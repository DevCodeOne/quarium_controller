#include "schedule/schedule_event.h"
#include "logger.h"

std::optional<schedule_event> schedule_event::create_from_description(json &schedule_event_description) {
    json id_entry = schedule_event_description["id"];
    json day_entry = schedule_event_description["day"];
    json trigger_at_entry = schedule_event_description["trigger_at"];
    json actions_entry = schedule_event_description["actions"];

    schedule_event created_event;

    if (id_entry.is_null() || day_entry.is_null() || trigger_at_entry.is_null() || actions_entry.is_null()) {
        logger::instance()->critical("A needed entry in a event was missing : {} {} {} {}",
                                     id_entry.is_null() ? "id" : "", day_entry.is_null() ? "day" : "",
                                     trigger_at_entry.is_null() ? "trigger_at" : "",
                                     actions_entry.is_null() ? "actions_entry" : "");
        if (!id_entry.is_null() && id_entry.is_string()) {
            logger::instance()->critical("The issue was in the event with the id {}", id_entry.get<std::string>());
        }
        return {};
    }

    if (!id_entry.is_string()) {
        logger::instance()->critical("The id of an event is not a string");
        return {};
    }

    created_event.id(id_entry.get<std::string>());

    if (!day_entry.is_number_unsigned()) {
        logger::instance()->critical("The day offset of the entry is not an unsigned number");
        return {};
    }

    created_event.day(days{day_entry.get<unsigned int>()});

    if (!trigger_at_entry.is_string()) {
        logger::instance()->critical("The trigger_at entry is not a string");
        return {};
    }

    std::istringstream trigger_at_stream{trigger_at_entry.get<std::string>()};
    std::tm trigger_at{};

    trigger_at_stream >> std::get_time(&trigger_at, "%H:%M");

    if (trigger_at_stream.fail()) {
        logger::instance()->critical("The trigger_at entry is not a valid time (HH:MM, e.g. 13:37)");
        return {};
    }

    created_event.trigger_time(
        std::chrono::minutes(trigger_at.tm_min) +
        std::chrono::duration_cast<std::chrono::minutes>(std::chrono::hours(trigger_at.tm_hour)));

    if (!actions_entry.is_array()) {
        logger::instance()->critical("The actions entry is not an array");
        return {};
    }

    for (auto &current_action_id : actions_entry) {
        if (!current_action_id.is_string()) {
            logger::instance()->critical("The entry in the actions array is not a string");
            return {};
        }

        schedule_action_id id = current_action_id.get<std::string>();

        if (!schedule_action::is_valid_id(id)) {
            logger::instance()->critical("The provided action id is not valid");
            return {};
        }

        created_event.add_action(id);
    }

    return created_event;
}

schedule_event &schedule_event::id(const std::string &new_id) {
    m_id = new_id;
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

const std::string &schedule_event::id() const { return m_id; }

std::chrono::minutes schedule_event::trigger_time() const { return m_trigger_time; }

days schedule_event::day() const { return m_day; }

const std::vector<schedule_action_id> &schedule_event::actions() const { return m_actions; }

bool schedule_event::is_marked() const { return m_marker; }

void schedule_event::unmark() const { m_marker = false; }

void schedule_event::mark() const { m_marker = true; }
