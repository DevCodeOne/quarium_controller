#include "schedule/schedule.h"

#include "config.h"
#include "logger.h"

std::optional<schedule> schedule::create_from_file(const std::filesystem::path &schedule_file_path) {
    std::ifstream file_as_stream(schedule_file_path);

    if (!file_as_stream) {
        logger::instance()->critical("Couldn't open schedule file {}", schedule_file_path.c_str());
        return {};
    }

    nlohmann::json schedule_file;

    try {
        schedule_file = nlohmann::json::parse(file_as_stream);
    } catch (std::exception &e) {
        logger::instance()->critical("Schedule file {} contains errors : {}",
                                     std::filesystem::canonical(schedule_file_path).c_str(), e.what());
        return {};
    }

    auto outputs = schedule_file["outputs"];

    if (outputs.is_null()) {
        logger::instance()->critical("No outputs are defined in the file {}", schedule_file_path.c_str());
        return {};
    }

    bool successfully_parsed_all_gpios =
        std::all_of(outputs.begin(), outputs.end(),
                    [](auto &current_output_description) { return outputs::add_output(current_output_description); });

    if (!successfully_parsed_all_gpios) {
        logger::instance()->critical("One or more descriptions of outputs contain errors");
        return {};
    }

    auto actions = schedule_file["actions"];

    if (actions.is_null()) {
        logger::instance()->critical("No actions are defined in the file {}", schedule_file_path.c_str());
        return {};
    }

    bool successfully_parsed_all_actions = std::all_of(
        actions.begin(), actions.end(),
        [](auto &current_action_description) { return schedule_action::add_action(current_action_description); });

    if (!successfully_parsed_all_actions) {
        logger::instance()->critical("Description of one or more actions contain errors");
        return {};
    }

    auto schedule = schedule_file["schedule"];

    return deserialize(schedule);
}

std::optional<schedule> schedule::deserialize(const nlohmann::json &schedule_description) {
    auto logger_instance = logger::instance();

    auto event_description_array_entry = schedule_description.find("events");
    auto title_entry = schedule_description.find("title");
    auto start_date_entry = schedule_description.find("start_at");
    auto end_date_entry = schedule_description.find("end_at");
    auto repeating_entry = schedule_description.find("repeating");
    auto period_entry = schedule_description.find("period_in_days");

    if (config::instance() == nullptr) {
        logger_instance->critical("No config found");
        return {};
    }

    std::string date_format = config::instance()->find("date_format").get<std::string>();

    schedule created_schedule;

    if (title_entry == schedule_description.cend()) {
        using namespace std::literals;

        logger_instance->warn("Schedule has no title");
        logger_instance->warn("Schedule has no title");
        created_schedule.title("Schedule"s + std::to_string(_instance_count));
    } else if (title_entry->is_string()) {
        created_schedule.title(title_entry->get<std::string>());
    } else {
        logger_instance->critical("Title of the schedule is not a string");
        return {};
    }

    if (event_description_array_entry == schedule_description.cend()) {
        logger_instance->critical("Schedule has no events array");
        return {};
    }

    if (!event_description_array_entry->is_array()) {
        logger_instance->critical("Schedule has events entry but it is not an array");
        return {};
    }

    std::vector<schedule_event> created_events;

    bool successfully_created_all_events =
        std::all_of(event_description_array_entry->begin(), event_description_array_entry->end(),
                    [&created_events](auto &current_event_description) {
                        auto created_schedule_event = schedule_event::deserialize(current_event_description);

                        if (!created_schedule_event) {
                            return false;
                        }

                        created_events.emplace_back(std::move(*created_schedule_event));
                        return true;
                    });

    if (!successfully_created_all_events) {
        logger_instance->critical("Description of one or more events contains errors");
        return {};
    }

    days period{0};
    schedule::mode schedule_mode = schedule::mode::repeating;

    if (start_date_entry == schedule_description.cend() && end_date_entry == schedule_description.cend() &&
        period_entry == schedule_description.cend()) {
        logger_instance->warn(
            "No date information for schedule is defined, all settings will be set to their default values");
    } else {
        // TODO remove code duplication
        if (start_date_entry != schedule_description.cend()) {
            if (!start_date_entry->is_string()) {
                logger_instance->critical("start_at is not a string");
                return {};
            }
            auto start_at =
                convert_date_to_duration_since_epoch<days>(start_date_entry->get<std::string>(), date_format);

            if (!start_at) {
                logger_instance->critical(
                    "start_at date doesn't conform to the specified date format, leading zeros for numbers smaller "
                    "than 10 are required");
                return {};
            }

            created_schedule.start_at(*start_at);
        } else {
            logger_instance->warn("No start date given for schedule");
        }

        if (end_date_entry != schedule_description.cend()) {
            if (!end_date_entry->is_string()) {
                logger_instance->critical("end_at is not a string");
                return {};
            }

            auto end_at = convert_date_to_duration_since_epoch<days>(end_date_entry->get<std::string>(), date_format);

            if (!end_at) {
                logger_instance->critical(
                    "end_at date doesn't conform to the specified date format, leading zeros for numbers smaller "
                    "than 10 are required");
                return {};
            }

            created_schedule.end_at(*end_at);
        }
    }

    if (repeating_entry != schedule_description.cend()) {
        if (!repeating_entry->is_boolean()) {
            logger_instance->critical("repeating is not a boolean");
            return {};
        }

        schedule_mode = repeating_entry->get<bool>() ? schedule::mode::repeating : schedule::mode::single_iteration;
    } else {
        logger_instance->warn("No value given for repeating, it will be set on the default value");
    }

    if (period_entry != schedule_description.cend()) {
        if (!period_entry->is_number_unsigned()) {
            logger_instance->critical("period_in_days is not an unsigned number");
            return {};
        }

        period = days{period_entry->get<unsigned int>()};
    }

    created_schedule.period(period).schedule_mode(schedule_mode);

    bool successfully_added_all_events =
        std::all_of(created_events.cbegin(), created_events.cend(),
                    [&created_schedule](auto &current_event) { return created_schedule.add_event(current_event); });

    if (!successfully_added_all_events) {
        logger_instance->critical("Couldn't add all events to schedule");
        return {};
    }

    if (!created_schedule) {
        return {};
    }

    ++_instance_count;
    return created_schedule;
}

// TODO test serialize method
nlohmann::json schedule::serialize() const {
    nlohmann::json serialized;

    for (auto &current_event : m_events) {
        serialized["events"].push_back(current_event.serialize());
    }

    serialized["title"] = m_title;
    serialized["period_in_days"] = (unsigned int)m_period.count();
    serialized["repeating"] = m_mode == mode::repeating;

    if (m_start_at) {
        serialized["start_at"] = m_start_at.value().count();
    }

    if (m_end_at) {
        serialized["end_at"] = m_end_at.value().count();
    }

    return std::move(serialized);
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
                    [event](auto &current_event) { return event.name() == current_event.name(); })) {
        logger::instance()->critical("Duplicate event name {}", event.name());
        return false;
    }
    // TODO: consider inserting at the right place to prevent having to sort again
    m_events.emplace_back(event);
    std::sort(m_events.begin(), m_events.end(), is_earlier);
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
