#include <algorithm>
#include <fstream>
#include <optional>
#include <vector>

#include "chrono_time.h"
#include "config.h"
#include "logger.h"
#include "schedule/schedule_handler.h"
#include "signal_handler.h"

std::shared_ptr<schedule_handler> schedule_handler::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = std::shared_ptr<schedule_handler>(new schedule_handler);
    }

    return _instance;
}

void schedule_handler::start_event_handler() {
    if (m_is_started) {
        return;
    }

    m_is_started = true;
    m_event_thread = std::thread(schedule_handler::event_handler);
}

void schedule_handler::stop_event_handler() {
    if (!m_is_started) {
        return;
    }

    m_should_exit = true;
    m_event_thread.join();
    m_is_started = false;
}

bool schedule_handler::add_schedule(schedule sched) {
    if (!sched) {
        logger::instance()->critical("Schedule is not valid");
        return false;
    }

    if (is_conflicting_with_other_schedules(sched)) {
        logger::instance()->critical("Schedule {} is conflicting with other schedules", sched.title());
        return false;
    }

    auto current_day = days_since_epoch();

    if (!sched.end_at().has_value() && !sched.start_at().has_value()) {
        sched.start_at(current_day);
        sched.end_at(days(sched.start_at().value() + sched.period() - days(1)));
    } else if (!sched.end_at().has_value()) {
        sched.end_at(days(sched.start_at().value() + sched.period() - days(1)));
    }

    if (!sched.start_at().has_value() || !sched.end_at().has_value()) {
        logger::instance()->critical("Schedule {} is invalid", sched.title());
        return false;
    }

    if (sched.start_at().value() <= current_day && sched.end_at().value() >= current_day) {
        std::lock_guard<std::recursive_mutex> instance_guard(m_schedules_list_mutex);
        logger::instance()->info("Added schedule {} to active schedules", sched.title());
        m_active_schedules.emplace_back(std::move(sched));
    } else {
        std::lock_guard<std::recursive_mutex> instance_guard(m_schedules_list_mutex);
        logger::instance()->info("Added schedule {} to inactive schedules", sched.title());
        m_inactive_schedules.emplace_back(std::move(sched));
    }

    return true;
}

void schedule_handler::event_handler() {
    signal_handler::disable_for_current_thread();

    auto handler_instance = instance();

    while (!handler_instance->m_should_exit) {
        {
            std::lock_guard<std::recursive_mutex> instance_guard(handler_instance->m_schedules_list_mutex);

            auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            auto *tm = std::localtime(&time);
            auto minutes_since_today =
                std::chrono::duration_cast<std::chrono::minutes>(std::chrono::hours(tm->tm_hour)) +
                std::chrono::minutes(tm->tm_min);
            days current_day = days_since_epoch();

            // TODO move old schedules to inactive restart periodical schedules
            for (auto &current_schedule : handler_instance->m_active_schedules) {
                logger::instance()->info("Checking events of schedule {}", current_schedule.title());

                for (auto &current_event : current_schedule.events()) {
                    if (current_event.trigger_time() != minutes_since_today) {
                        current_event.unmark();
                        continue;
                    }

                    if (current_event.is_marked()) {
                        continue;
                    }

                    for (auto &current_action_id : current_event.actions()) {
                        logger::instance()->info("Executing action {} of event {}", current_action_id,
                                                 current_event.id());
                        bool result = schedule_action::execute_action(current_action_id);

                        if (!result) {
                            logger::instance()->warn("Something went wrong when executing action {} in event {}",
                                                     current_action_id, current_event.id());
                        }
                    }

                    current_event.mark();
                }
            }

            std::vector<schedule> to_be_added_back;

            auto activate_schedules =
                std::remove_if(handler_instance->m_inactive_schedules.begin(),
                               handler_instance->m_inactive_schedules.end(), [](const auto &current_schedule) {
                                   return current_schedule.schedule_mode() == schedule::mode::repeating;
                               });

            for (auto current_schedule = activate_schedules;
                 current_schedule != handler_instance->m_inactive_schedules.cend(); ++current_schedule) {
                schedule created_schedule(*current_schedule);

                created_schedule.start_at(current_day);
                created_schedule.end_at(created_schedule.start_at().value() + created_schedule.period() - days(1));
                to_be_added_back.emplace_back(std::move(created_schedule));
            }

            if (activate_schedules != handler_instance->m_inactive_schedules.cend()) {
                handler_instance->m_inactive_schedules.erase(activate_schedules);
            }

            auto to_be_removed = std::remove_if(
                handler_instance->m_active_schedules.begin(), handler_instance->m_active_schedules.end(),
                [current_day](const auto &current_schedule) { return current_schedule.end_at() < current_day; });

            if (to_be_removed != handler_instance->m_active_schedules.cend()) {
                for (auto current_schedule = to_be_removed;
                     current_schedule != handler_instance->m_active_schedules.cend(); ++current_schedule) {
                    if (current_schedule->schedule_mode() == schedule::mode::repeating) {
                        schedule created_schedule(*current_schedule);

                        created_schedule.start_at(current_day);
                        created_schedule.end_at(created_schedule.start_at().value() + created_schedule.period());
                        to_be_added_back.emplace_back(std::move(created_schedule));
                    } else {
                        handler_instance->m_inactive_schedules.emplace_back(std::move(*current_schedule));
                    }
                }
                handler_instance->m_active_schedules.erase(to_be_removed);
            }

            for (auto current_schedule : to_be_added_back) {
                bool result = handler_instance->add_schedule(current_schedule);

                if (result) {
                    logger::instance()->info("Added schedule {} back to active schedules", current_schedule.title());
                } else {
                    logger::instance()->info("Couldn't add schedule {} back to active schedules",
                                             current_schedule.title());
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

days schedule_handler::days_since_epoch() {
    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto *tm = std::localtime(&time);
    return std::chrono::duration_cast<days>(std::chrono::seconds(std::mktime(tm)));
}

// TODO implement remove code duplication
bool schedule_handler::is_conflicting_with_other_schedules(const schedule &sched) {
    auto is_conflicting = [&sched](const auto &current_sched) { return sched.title() == current_sched.title(); };

    auto result = std::find_if(m_active_schedules.cbegin(), m_active_schedules.cend(), is_conflicting);

    if (result != m_active_schedules.cend()) {
        return true;
    }

    result = std::find_if(m_inactive_schedules.cbegin(), m_inactive_schedules.cend(), is_conflicting);

    if (result != m_inactive_schedules.cend()) {
        return true;
    }

    return false;
}
