#include <algorithm>
#include <fstream>
#include <optional>
#include <vector>

#include "chrono_time.h"
#include "config.h"
#include "logger.h"
#include "schedule.h"

bool schedule_action::add_action(json &schedule_action_description) {
    json id_entry = schedule_action_description["id"];
    json gpios_entry = schedule_action_description["gpios"];
    json gpio_actions_entry = schedule_action_description["gpio_actions"];

    if (id_entry.is_null() || gpios_entry.is_null() || gpio_actions_entry.is_null()) {
        logger::instance()->critical("A needed entry in a action was missing : {} {} {}",
                                     id_entry.is_null() ? "id" : "", gpios_entry.is_null() ? "gpios" : "",
                                     gpio_actions_entry.is_null() ? "gpio_actions" : "");
        if (!id_entry.is_null() && id_entry.is_string()) {
            logger::instance()->critical("The issue was in the action with the id {}", id_entry.get<std::string>());
        }
        return false;
    }

    schedule_action created_action;

    if (!id_entry.is_string()) {
        logger::instance()->critical("The id for an action is not a string");
    }

    const std::string id = id_entry.get<std::string>();
    if (is_valid_id(id)) {
        logger::instance()->critical("The id for an action {} is already in use");
        return false;
    }

    if (!gpios_entry.is_array()) {
        logger::instance()->critical("The entry gpios in action {} isn't an array", id);
        return false;
    }

    std::vector<gpio_pin> created_gpios;
    bool created_all_gpios_successfully =
        std::all_of(gpios_entry.begin(), gpios_entry.end(), [&created_gpios](auto &current_gpio_entry) {
            if (!current_gpio_entry.is_number_unsigned()) {
                return false;
            }
            created_gpios.emplace_back(gpio_pin{current_gpio_entry.template get<unsigned int>()});
            return true;
        });

    if (!created_all_gpios_successfully) {
        logger::instance()->critical("The entry gpios in action {} isn't valid", id);
        return false;
    }

    if (!gpio_actions_entry.is_array()) {
        logger::instance()->critical("The entry gpio_actions in action {} isn't an array", id);
        return false;
    }

    std::vector<gpio_pin::action> created_gpio_actions;
    bool created_all_gpio_actions_successfully = std::all_of(
        gpio_actions_entry.begin(), gpio_actions_entry.end(), [&created_gpio_actions](auto &current_gpio_entry) {
            if (!current_gpio_entry.is_boolean()) {
                return false;
            }
            created_gpio_actions.emplace_back(gpio_pin::action{current_gpio_entry.template get<bool>()});
            return true;
        });

    if (!created_all_gpio_actions_successfully) {
        logger::instance()->critical("The entry gpio_actions in action {} isn't valid", id);
    }

    if (created_gpios.size() != created_gpio_actions.size()) {
        logger::instance()->critical(
            "There are not the same number of entries in the gpio specific arrays in action {}", id);
        return false;
    }

    created_action.id(id);
    for (unsigned int i = 0; i < created_gpios.size(); ++i) {
        created_action.add_pin(std::make_pair(created_gpios[i], created_gpio_actions[i]));
    }

    std::lock_guard<std::mutex> list_guard{_list_mutex};

    _actions[id] = created_action;

    return true;
}

bool schedule_action::is_valid_id(const schedule_action_id &id) { return _actions.find(id) != _actions.cend(); }

bool schedule_action::execute_action(const schedule_action_id &id) {
    if (!is_valid_id(id)) {
        return false;
    }

    return _actions[id]();
}

schedule_action &schedule_action::id(const schedule_action_id &new_id) {
    m_id = new_id;
    return *this;
}

schedule_action &schedule_action::add_pin(const std::pair<gpio_pin, gpio_pin::action> &new_pin) {
    m_pins.emplace_back(new_pin);
    return *this;
}

bool schedule_action::operator()() { return false; }

const schedule_action_id &schedule_action::id() const { return m_id; }

const std::vector<std::pair<gpio_pin, gpio_pin::action>> &schedule_action::pins() const { return m_pins; }

std::optional<schedule_event> schedule_event::create_from_description(json &schedule_event_description) {
    json id_entry = schedule_event_description["id"];
    json day_entry = schedule_event_description["day"];
    json trigger_at_entry = schedule_event_description["trigger_at"];
    json actions_entry = schedule_event_description["actions"];

    schedule_event created_event;

    if (id_entry.is_null() || day_entry.is_null() || trigger_at_entry.is_null() || actions_entry.is_null()) {
        logger::instance()->critical("A needed entry in a event was missing : {} {} {} {}",
                                     id_entry.is_null() ? "id" : "", day_entry.is_null() ? "day_entry" : "",
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

const std::vector<schedule_action_id> &schedule_event::assigned_actions() const { return m_actions; }

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

    auto actions = schedule_file["actions"];
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
        days start_at{0};
        days end_at{0};

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

            created_schedule.start_at(*end_at);
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
            logger::instance()->critical("period_in_days is not a number");
            return {};
        }

        period = days{period_entry.get<unsigned int>()};
    }

    created_schedule.period(period).schedule_mode(schedule_mode);

    for (auto &current_event : created_events) {
        created_schedule.add_event(current_event);
    }

    if (!created_schedule) {
        return {};
    }

    ++_instance_count;
    return created_schedule;
}

schedule &schedule::start_at(const days &new_start) {
    m_start_at = new_start;
    return *this;
}

schedule &schedule::end_at(const days &new_end) {
    m_end_at = new_end;
    return *this;
}

schedule &schedule::period(const days &new_period) {
    m_period = new_period;
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

schedule &schedule::add_event(const schedule_event &event) {
    m_events.emplace_back(event);
    return *this;
}

std::optional<days> schedule::start_at() const { return m_start_at; }

std::optional<days> schedule::end_at() const { return m_end_at; }

days schedule::period() const { return m_period; }

const std::string &schedule::title() const { return m_title; }

schedule::mode schedule::schedule_mode() const { return m_mode; }

const std::vector<schedule_event> &schedule::events() const { return m_events; }

// TODO implement
schedule::operator bool() const { return true; }
