#include "schedule/schedule_handler.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#include <vector>

#include "chrono_time.h"
#include "config.h"
#include "logger.h"
#include "signal_handler.h"

// TODO: add mutexes here
std::shared_ptr<schedule_handler> schedule_handler::instance() { return singleton<schedule_handler>::instance(); }

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

    m_should_exit.store(true);
    m_event_thread.join();
    m_is_started = false;
}

bool schedule_handler::add_schedule(schedule sched) {
    auto logger_instance = logger::instance();
    if (!sched) {
        logger_instance->critical("Schedule is not valid");
        return false;
    }

    if (is_conflicting_with_other_schedules(sched)) {
        logger_instance->critical("Schedule {} is conflicting with other schedules", sched.title());
        return false;
    }

    auto current_day = duration_since_epoch<days>();

    if (!sched.end_at().has_value() && !sched.start_at().has_value()) {
        sched.start_at(current_day);
        sched.end_at(days(sched.start_at().value() + sched.period() - days(1)));
    } else if (!sched.end_at().has_value()) {
        sched.end_at(days(sched.start_at().value() + sched.period() - days(1)));
    }

    if ((sched.schedule_mode() == schedule::mode::repeating) &&
        (!sched.start_at().has_value() || !sched.end_at().has_value())) {
        logger_instance->critical("Schedule {} is invalid", sched.title());
        return false;
    }

    {
        auto lock = singleton<schedule_handler>::retrieve_instance_lock();
        logger_instance->info("Added schedule {} to the list of schedules", sched.title());
        m_inactive_schedules.emplace_back(std::move(sched));
    }

    return true;
}

void schedule_handler::event_handler() {
    signal_handler::disable_for_current_thread();
    std::vector<schedule_action_id> actions_to_execute;
    std::vector<bool> execution_results;
    std::multimap<schedule_action_id,
                  std::decay_t<decltype(std::declval<rest_resource_id<schedule_event>>().as_number())>>
        actions_to_event_mapping;
    auto logger_instance = logger::instance();

    auto handler_instance = instance();

    while (!handler_instance->m_should_exit.load()) {
        {
            auto lock = singleton<schedule_handler>::retrieve_instance_lock();

            auto minutes_since_today = duration_since_epoch<std::chrono::minutes>() - duration_since_epoch<days>();
            auto current_day = duration_since_epoch<days>();

            for (const auto &current_schedule : handler_instance->m_active_schedules) {
                actions_to_execute.clear();
                execution_results.clear();
                actions_to_event_mapping.clear();

                logger_instance->info("Checking events of schedule {}", current_schedule.title());

                if (!current_schedule.start_at().has_value()) {
                    continue;
                }

                auto day_in_schedule = current_day - current_schedule.start_at().value();

                // TODO: when the clock is set to an earlier timepoint this code won't behave correctly
                for (const auto &current_event : current_schedule.events()) {
                    if ((current_event.day() > day_in_schedule) ||
                        (current_event.trigger_time() > minutes_since_today) || current_event.is_processed()) {
                        continue;
                    }

                    logger_instance->info("Scheduling actions of : {} id : {} for execution", current_event.name(),
                                          current_event.id().as_number());

                    for (auto &current_action_id : current_event.actions()) {
                        actions_to_execute.emplace_back(current_action_id);
                        actions_to_event_mapping.insert({current_action_id, current_event.id().as_number()});
                    }

                    current_event.mark_as_processed();
                }

                // Actions are in order, because the events are in order (sorted by event time on a per day basis)
                auto failed_actions = schedule_action::execute_actions(actions_to_execute);

                if (failed_actions.size() > 0) {
                    logger_instance->warn("Some actions couldn't be executed");
                    // Find the corresponding events to the failed actions and remove their processed mark
                    std::vector<std::decay_t<decltype(std::declval<rest_resource_id<schedule_event>>().as_number())>>
                        failed_events;
                    for (const auto &current_failed_action_id : failed_actions) {
                        auto failed_events_of_action = actions_to_event_mapping.equal_range(current_failed_action_id);

                        for (auto it = failed_events_of_action.first; it != failed_events_of_action.second; ++it) {
                            failed_events.emplace_back(it->second);
                        }
                    }

                    for (auto const &current_event : current_schedule.events()) {
                        auto current_event_id = current_event.id().as_number();
                        if (std::any_of(
                                failed_events.cbegin(), failed_events.cend(),
                                [current_event_id](auto &current_id) { return current_id == current_event_id; })) {
                            // Some actions or all the actions of this event were unsuccesfull unmark as processed, so
                            // the are actions can be tried again
                            current_event.unmark_as_processed();
                            logger_instance->critical("One or more actions of the event {} resulted in errors",
                                                      current_event.name());
                        }
                    }

                    logger_instance->critical("Failed to execute some actions of schedule {}",
                                              current_schedule.title());
                }
            }

            std::vector<schedule> to_be_added_back;

            auto activate_schedules = std::remove_if(
                handler_instance->m_inactive_schedules.begin(), handler_instance->m_inactive_schedules.end(),
                [&current_day](const auto &current_schedule) {
                    return current_schedule.schedule_mode() == schedule::mode::repeating ||
                           current_schedule.end_at() >= current_day;
                });

            for (auto current_schedule = activate_schedules;
                 current_schedule != handler_instance->m_inactive_schedules.cend(); ++current_schedule) {
                auto created_schedule(*current_schedule);

                for (const auto &current_event : created_schedule.events()) {
                    current_event.unmark_as_processed();
                }

                if (current_schedule->end_at() < current_day &&
                    created_schedule.schedule_mode() == schedule::mode::repeating) {
                    created_schedule.start_at(current_day);
                    created_schedule.end_at(created_schedule.start_at().value() + created_schedule.period() - days(1));
                }

                logger_instance->info("Added Schedule {} to the list of active schedules", created_schedule.title());
                handler_instance->m_active_schedules.emplace_back(std::move(created_schedule));
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
                    logger_instance->info("Remove schedule {} from the list of active schedules",
                                          current_schedule->title());
                    handler_instance->m_inactive_schedules.emplace_back(std::move(*current_schedule));
                }
                handler_instance->m_active_schedules.erase(to_be_removed);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

bool schedule_handler::is_conflicting_with_other_schedules(const schedule &sched) {
    // TODO: maybe add a check if outputs overlapp ?
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

// http::response<http::dynamic_body> schedule_handler::handle_request(const http::request<http::dynamic_body> &request)
// {
//     http::response<http::dynamic_body> response;
//
//     std::string resource_path_string(request.target().to_string());
//     std::regex list_schedules_regex(R"(/api/v0/schedules/?)", std::regex_constants::extended);
//     std::regex schedule_id_regex(R"([1-9][0-9]*|[0-9])", std::regex_constants::extended);
//
//     if (std::regex_match(resource_path_string, list_schedules_regex) && request.method() == http::verb::get) {
//         auto lock = singleton<schedule_handler>::retrieve_instance_lock();
//         auto instance = schedule_handler::instance();
//
//         nlohmann::json schedules;
//
//         auto add_serialized_schedule = [&schedules](const auto &current_schedule) {
//             json pair;
//             pair["id"] = current_schedule.id().as_number();
//             pair["title"] = current_schedule.title();
//             schedules.push_back(std::move(pair));
//         };
//
//         std::for_each(instance->m_active_schedules.cbegin(), instance->m_active_schedules.cend(),
//                       add_serialized_schedule);
//         std::for_each(instance->m_inactive_schedules.cbegin(), instance->m_inactive_schedules.cend(),
//                       add_serialized_schedule);
//
//         boost::beast::ostream(response.body()) << schedules.dump();
//         return std::move(response);
//     }
//
//     std::smatch match;
//     std::regex_search(resource_path_string, match, list_schedules_regex);
//     std::string id = match.suffix();
//
//     if (std::regex_match(id, schedule_id_regex)) {
//         uint32_t id_as_number;
//         auto conversion_result = std::from_chars<uint32_t>(id.data(), id.data() + id.size(), id_as_number);
//
//         if (conversion_result.ec == std::errc::result_out_of_range) {
//             boost::beast::ostream(response.body()) << "ID is not a number";
//             return std::move(response);
//         }
//
//         auto lock = singleton<schedule_handler>::retrieve_instance_lock();
//         auto instance = schedule_handler::instance();
//
//         nlohmann::json specific_schedule;
//
//         auto result = std::find_if(
//             instance->m_active_schedules.cbegin(), instance->m_active_schedules.cend(),
//             [id_as_number](const auto &current_schedule) { return current_schedule.id().as_number() == id_as_number;
//             });
//
//         if (result == instance->m_active_schedules.cend()) {
//             result = std::find_if(instance->m_inactive_schedules.cbegin(), instance->m_inactive_schedules.cend(),
//                                   [id_as_number](const auto &current_schedule) {
//                                       return current_schedule.id().as_number() == id_as_number;
//                                   });
//         }
//
//         if (result != instance->m_inactive_schedules.cend()) {
//             boost::beast::ostream(response.body()) << result->serialize().dump();
//
//             return std::move(response);
//         }
//     }
//
//     boost::beast::ostream(response.body()) << "Couldn't find resource " << resource_path_string;
//     return std::move(response);
// }
