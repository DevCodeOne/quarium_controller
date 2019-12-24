#include "schedule/schedule_event.h"

#include "logger.h"

std::optional<schedule_event> schedule_event::deserialize(const json &description) {
    auto logger_instance = logger::instance();

    auto name_entry = description.find("name");
    auto day_entry = description.find("day");
    auto trigger_at_entry = description.find("trigger_at");
    auto actions_entry = description.find("actions");

    schedule_event created_event;

    if (name_entry == description.cend() || day_entry == description.cend() || trigger_at_entry == description.cend() ||
        actions_entry == description.cend()) {
        logger_instance->critical(
            "A needed entry in a event was missing : {} {} {} {}", name_entry == description.cend() ? "id" : "",
            day_entry == description.cend() ? "day" : "", trigger_at_entry == description.cend() ? "trigger_at" : "",
            actions_entry == description.cend() ? "actions_entry" : "");
        if (name_entry != description.cend() && name_entry->is_string()) {
            logger_instance->critical("The issue was in the event with the id {}", name_entry->get<std::string>());
        }
        return {};
    }

    if (!name_entry->is_string()) {
        logger_instance->critical("The id of an event is not a string");
        return {};
    }

    created_event.name(name_entry->get<std::string>());

    if (!day_entry->is_number_unsigned()) {
        logger_instance->critical("The day offset of the entry is not an unsigned number");
        return {};
    }

    created_event.day(days{day_entry->get<unsigned int>()});

    if (!trigger_at_entry->is_string()) {
        logger_instance->critical("The trigger_at entry is not a string");
        return {};
    }

    std::istringstream trigger_at_stream{trigger_at_entry->get<std::string>()};
    std::tm trigger_at{};

    trigger_at_stream >> std::get_time(&trigger_at, "%H:%M");

    if (trigger_at_stream.fail()) {
        logger_instance->critical("The trigger_at entry is not a valid time (HH:MM, e.g. 13:37)");
        return {};
    }

    created_event.trigger_time(
        std::chrono::minutes(trigger_at.tm_min) +
        std::chrono::duration_cast<std::chrono::minutes>(std::chrono::hours(trigger_at.tm_hour)));

    if (!actions_entry->is_array()) {
        logger_instance->critical("The actions entry is not an array");
        return {};
    }

    for (auto &current_action_id : (*actions_entry)) {
        if (!current_action_id.is_string()) {
            logger_instance->critical("The entry in the actions array is not a string");
            return {};
        }

        schedule_action_id id = current_action_id.get<std::string>();

        if (!schedule_action::is_valid_id(id)) {
            logger_instance->critical("The provided action id is not valid");
            return {};
        }

        created_event.add_action(id);
    }

    return created_event;
}

schedule_event &schedule_event::name(const std::string &new_name) {
    m_name = new_name;
    return *this;
}

schedule_event &schedule_event::trigger_time(const std::chrono::minutes new_trigger_time) {
    m_trigger_time = new_trigger_time;
    return *this;
}

schedule_event &schedule_event::day(const days &day) {
    m_day = day;
    return *this;
}

schedule_event &schedule_event::add_action(const schedule_action_id &action_id) {
    m_actions.emplace_back(action_id);
    return *this;
}

const std::string &schedule_event::name() const { return m_name; }

std::chrono::minutes schedule_event::trigger_time() const { return m_trigger_time; }

days schedule_event::day() const { return m_day; }

const std::vector<schedule_action_id> &schedule_event::actions() const { return m_actions; }

bool schedule_event::is_processed() const { return m_marker; }

void schedule_event::unmark_as_processed() const { m_marker = false; }

void schedule_event::mark_as_processed() const { m_marker = true; }

bool is_earlier(const schedule_event &lhs, const schedule_event &rhs) {
    return (lhs.day() < rhs.day()) || (lhs.day() == rhs.day() && lhs.trigger_time() < rhs.trigger_time());
}

// TODO Test serialize method
nlohmann::json schedule_event::serialize() const {
    nlohmann::json serialized;

    serialized["name"] = m_name;
    serialized["day"] = m_day.count();
    serialized["trigger_at"] = m_trigger_time.count();

    for (auto &current_action_id : m_actions) {
        serialized["actions"].push_back(current_action_id);
    }

    return std::move(serialized);
}
