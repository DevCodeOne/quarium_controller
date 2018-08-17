#include "schedule/schedule.h"
#include "logger.h"
#include "config.h"

std::optional<schedule> schedule::create_from_file(const std::filesystem::path &schedule_file_path) {
    using namespace nlohmann;

    std::ifstream file_as_stream(schedule_file_path);

    if (!file_as_stream) {
        logger::instance()->critical("Couldn't open schedule file {}", schedule_file_path.c_str());
        return {};
    }

    json schedule_file;
    bool is_valid = true;

    try {
        schedule_file = json::parse(file_as_stream);
    } catch (std::exception &e) {
        logger::instance()->critical("Schedule file {} contains errors",
                                     std::filesystem::canonical(schedule_file_path).c_str());
        logger::instance()->critical(e.what());
        is_valid = false;
    }

    if (!is_valid) {
        return {};
    }

    auto gpios = schedule_file["gpios"];

    if (gpios.is_null()) {
        logger::instance()->warn("No gpios are defined in the file {}", schedule_file_path.c_str());
    }

    bool successfully_parsed_all_gpios = std::all_of(gpios.begin(), gpios.end(), [](auto &current_action_description) {
        return schedule_gpio::add_gpio(current_action_description);
    });

    if (!successfully_parsed_all_gpios) {
        logger::instance()->critical("Description of one or more gpios contain errors");
        return {};
    }

    auto actions = schedule_file["actions"];

    if (actions.is_null()) {
        logger::instance()->warn("No actions are defined in the file {}", schedule_file_path.c_str());
    }

    bool successfully_parsed_all_actions = std::all_of(
        actions.begin(), actions.end(),
        [](auto &current_action_description) { return schedule_action::add_action(current_action_description); });

    if (!successfully_parsed_all_actions) {
        logger::instance()->critical("Description of one or more actions contain errors");
        return {};
    }

    auto schedule = schedule_file["schedule"];

    return create_from_description(schedule);
}

std::optional<schedule> schedule::create_from_description(json &schedule_description) {
    json event_description_array_entry = schedule_description["events"];
    json title_entry = schedule_description["title"];
    json start_date_entry = schedule_description["start_at"];
    json end_date_entry = schedule_description["end_at"];
    json repeating_entry = schedule_description["repeating"];
    json period_entry = schedule_description["period_in_days"];
    std::string date_format = config::instance()->find("date_format").get<std::string>();

    schedule created_schedule;

    if (title_entry.is_null()) {
        using namespace std::literals;
        logger::instance()->warn("Schedule has no title");
        created_schedule.title("Schedule"s + std::to_string(_instance_count));
    } else if (title_entry.is_string()) {
        created_schedule.title(title_entry.get<std::string>());
    } else {
        logger::instance()->critical("Title of the schedule is not a string");
        return {};
    }

    if (event_description_array_entry.is_null()) {
        logger::instance()->critical("Schedule has no events array");
        return {};
    }

    if (!event_description_array_entry.is_array()) {
        logger::instance()->critical("Schedule has events entry but it is not an array");
        return {};
    }

    std::vector<schedule_event> created_events;

    bool successfully_created_all_events = std::all_of(
        event_description_array_entry.begin(), event_description_array_entry.end(),
        [&created_events](auto &current_event_description) {
            auto created_schedule_event = schedule_event::create_from_description(current_event_description);

            if (!created_schedule_event) {
                return false;
            }

            created_events.emplace_back(std::move(*created_schedule_event));
            return true;
        });

    if (!successfully_created_all_events) {
        logger::instance()->critical("Description of one or more events contains errors");
        return {};
    }

    days period{0};
    schedule::mode schedule_mode = schedule::mode::repeating;

    if (start_date_entry.is_null() && end_date_entry.is_null() && period_entry.is_null()) {
        logger::instance()->warn(
            "No date information for schedule is defined, all settings will be set to their default values");
    } else {
        // TODO remove code duplication
        if (!start_date_entry.is_null()) {
            if (!start_date_entry.is_string()) {
                logger::instance()->critical("start_at is not a string");
                return {};
            }
            auto start_at =
                convert_date_to_duration_since_epoch<days>(start_date_entry.get<std::string>(), date_format);

            if (!start_at) {
                logger::instance()->critical(
                    "start_at date doesn't conform to the specified date format, leading zeros for numbers smaller "
                    "than 10 are required");
                return {};
            }

            created_schedule.start_at(*start_at);
        } else {
            logger::instance()->warn("No start date given for schedule");
        }

        if (!end_date_entry.is_null()) {
            if (!end_date_entry.is_string()) {
                logger::instance()->critical("end_at is not a string");
                return {};
            }

            auto end_at = convert_date_to_duration_since_epoch<days>(end_date_entry.get<std::string>(), date_format);

            if (!end_at) {
                logger::instance()->critical(
                    "end_at date doesn't conform to the specified date format, leading zeros for numbers smaller "
                    "than 10 are required");
                return {};
            }

            created_schedule.end_at(*end_at);
        }
    }

    if (!repeating_entry.is_null()) {
        if (!repeating_entry.is_boolean()) {
            logger::instance()->critical("repeating is not a boolean");
            return {};
        }

        schedule_mode = repeating_entry.get<bool>() ? schedule::mode::repeating : schedule::mode::single_iteration;
    } else {
        logger::instance()->warn("No value given for repeating, it will be set on the default value");
    }

    if (!period_entry.is_null()) {
        if (!period_entry.is_number_unsigned()) {
            logger::instance()->critical("period_in_days is not an unsigned number");
            return {};
        }

        period = days{period_entry.get<unsigned int>()};
    }

    created_schedule.period(period).schedule_mode(schedule_mode);

    bool successfully_added_all_events =
        std::all_of(created_events.cbegin(), created_events.cend(),
                    [&created_schedule](auto &current_event) { return created_schedule.add_event(current_event); });

    if (!successfully_added_all_events) {
        logger::instance()->critical("Couldn't add all events to schedule");
        return {};
    }

    if (!created_schedule) {
        return {};
    }

    ++_instance_count;
    return created_schedule;
}

schedule &schedule::start_at(const days &new_start) {
    m_start_at = new_start;
    recalculate_period();

    return *this;
}

schedule &schedule::end_at(const days &new_end) {
    m_end_at = new_end;
    recalculate_period();

    return *this;
}

schedule &schedule::period(const days &new_period) {
    recalculate_period();

    m_period = std::max(new_period, m_period);
    return *this;
}

schedule &schedule::title(const std::string &new_title) {
    m_title = new_title;
    return *this;
}

schedule &schedule::schedule_mode(const mode &new_mode) {
    m_mode = new_mode;
    return *this;
}

bool schedule::add_event(const schedule_event &event) {
    if (std::any_of(m_events.cbegin(), m_events.cend(),
                    [event](auto &current_event) { return event.id() == current_event.id(); })) {
        logger::instance()->critical("Duplicate event id {}", event.id());
        return false;
    }
    m_events.emplace_back(event);
    recalculate_period();
    return true;
}

void schedule::recalculate_period() {
    if (m_start_at.has_value() && m_end_at.has_value()) {
        if (m_end_at.value() > m_start_at.value()) {
            auto calculated_period = days(m_end_at.value() - m_start_at.value());

            m_period = std::max(calculated_period, m_period);
        }
    }

    auto max_day = std::max_element(m_events.cbegin(), m_events.cend(),
                                    [](const auto &lhs, const auto &rhs) { return lhs.day() < rhs.day(); });

    if (max_day == m_events.cend()) {
        return;
    }

    m_period = std::max(days(max_day->day() + days(1)), m_period);
}

std::optional<days> schedule::start_at() const { return m_start_at; }

std::optional<days> schedule::end_at() const { return m_end_at; }

days schedule::period() const { return m_period; }

const std::string &schedule::title() const { return m_title; }

schedule::mode schedule::schedule_mode() const { return m_mode; }

const std::vector<schedule_event> &schedule::events() const { return m_events; }

// TODO log issues with schedule
schedule::operator bool() const {
    if (m_start_at.has_value() && !m_end_at.has_value()) {
        return true;
    }

    if (!m_start_at.has_value() && !m_end_at.has_value()) {
        return true;
    }

    if (m_period.count() == 0) {
        return false;
    }

    if (m_end_at.value() < m_start_at.value()) {
        return false;
    }

    if (m_end_at.value() - m_start_at.value() + days(1) < m_period) {
        return false;
    }

    if (m_events.size() == 0) {
        return false;
    }

    return true;
}
