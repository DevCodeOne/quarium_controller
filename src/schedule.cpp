#include <algorithm>
#include <fstream>
#include <optional>
#include <vector>

#include "chrono_time.h"
#include "config.h"
#include "logger.h"
#include "schedule.h"
#include "signal_handler.h"

bool schedule_gpio::add_gpio(json &gpio_description) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    json id_entry = gpio_description["id"];
    json pin_entry = gpio_description["pin"];

    if (id_entry.is_null() || pin_entry.is_null()) {
        logger::instance()->critical("A needed entry in a gpio entry was missing {} {} {}",
                                     id_entry.is_null() ? "id" : "", pin_entry.is_null() ? "pin" : "");
        if (!id_entry.is_null() && id_entry.is_string()) {
            logger::instance()->critical("The issue was in the gpio entry with the id {}", id_entry.get<std::string>());
        }
        return false;
    }

    if (!id_entry.is_string()) {
        logger::instance()->critical("The id for a gpio entry is not a string");
        return false;
    }

    std::string id = id_entry.get<std::string>();

    if (is_valid_id(id)) {
        logger::instance()->critical("The id {} for a gpio entry is already in use", id);
        return false;
    }

    if (!pin_entry.is_number_unsigned()) {
        logger::instance()->critical("The pin entry of the gpio entry {} is not an unsigned number", id);
        return false;
    }

    unsigned int pin = pin_entry.get<unsigned int>();

    _gpios.emplace_back(std::make_unique<schedule_gpio>(id, gpio_pin_id(pin, gpio_chip::default_gpio_dev_path)));
    return true;
}

bool schedule_gpio::is_valid_id(const schedule_gpio_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};
    return std::find_if(_gpios.cbegin(), _gpios.cend(),
                        [&id](const auto &current_action) { return current_action->id() == id; }) != _gpios.cend();
}

bool schedule_gpio::control_pin(const schedule_gpio_id &id, gpio_pin::action &action) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    if (!is_valid_id(id)) {
        return false;
    }

    auto gpio = std::find_if(_gpios.begin(), _gpios.end(),
                             [&id](const auto &current_action) { return current_action->id() == id; });

    auto chip = gpio_chip::instance();

    if (!chip) {
        return false;
    }

    return chip.value()->control_pin((*gpio)->m_pin_id, action);
}

std::vector<schedule_gpio_id> schedule_gpio::get_ids() {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    std::vector<schedule_gpio_id> ids(_gpios.size());

    for (auto &current_gpio : _gpios) {
        ids.emplace_back(current_gpio->id());
    }

    return ids;
}

schedule_gpio::schedule_gpio(schedule_gpio &&other)
    : m_id(std::move(other.m_id)), m_pin_id(std::move(other.m_pin_id)) {}

schedule_gpio::schedule_gpio(const schedule_gpio_id &id, const gpio_pin_id &pin_id) : m_id(id), m_pin_id(pin_id) {}

const schedule_gpio_id &schedule_gpio::id() const { return m_id; }

const gpio_pin_id &schedule_gpio::pin() const { return m_pin_id; }

bool schedule_action::add_action(json &schedule_action_description) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    json id_entry = schedule_action_description["id"];
    json gpios_entry = schedule_action_description["gpios"];
    json gpio_actions_entry = schedule_action_description["gpio_actions"];

    if (id_entry.is_null() || gpios_entry.is_null() || gpio_actions_entry.is_null()) {
        logger::instance()->critical("A needed entry in an action was missing : {} {} {}",
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
        logger::instance()->critical("The id for an action {} is already in use", id);
        return false;
    }

    if (!gpios_entry.is_array()) {
        logger::instance()->critical("The entry gpios in action {} isn't an array", id);
        return false;
    }

    std::vector<schedule_gpio_id> created_gpios;
    bool created_all_gpios_successfully =
        std::all_of(gpios_entry.begin(), gpios_entry.end(), [&created_gpios](auto &current_gpio_entry) {
            if (!current_gpio_entry.is_string()) {
                return false;
            }

            std::string id = current_gpio_entry.template get<std::string>();
            if (!schedule_gpio::is_valid_id(id)) {
                logger::instance()->critical("A gpio with the id {} doesn't exist", id);
                return false;
            }

            created_gpios.emplace_back(schedule_gpio_id(id));
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
            if (!current_gpio_entry.is_string()) {
                return false;
            }

            std::string value = current_gpio_entry.template get<std::string>();

            gpio_pin::action act;

            if (value == "on") {
                act = gpio_pin::action::on;
            } else if (value == "off") {
                act = gpio_pin::action::off;
            } else if (value == "toggle") {
                act = gpio_pin::action::toggle;
            } else {
                logger::instance()->critical("One entry in gpio_actions is not on, off or toggle");
            }

            created_gpio_actions.emplace_back(act);
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

    _actions.emplace_back(std::make_unique<schedule_action>(std::move(created_action)));

    return true;
}

bool schedule_action::is_valid_id(const schedule_action_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};
    return std::find_if(_actions.cbegin(), _actions.cend(),
                        [&id](const auto &current_action) { return current_action->id() == id; }) != _actions.cend();
}

bool schedule_action::execute_action(const schedule_action_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};
    if (!is_valid_id(id)) {
        return false;
    }

    auto action = std::find_if(_actions.begin(), _actions.end(),
                               [&id](const auto &current_action) { return current_action->id() == id; });

    return (**action)();
}

schedule_action::schedule_action(schedule_action &&other)
    : m_id(std::move(other.m_id)), m_pins(std::move(other.m_pins)) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    if (is_valid_id(m_id)) {
        std::string id = m_id;
        auto action = std::find_if(_actions.begin(), _actions.end(),
                                   [&id](const auto &current_action) { return current_action->id() == id; });

        *action = std::unique_ptr<schedule_action>(this);
    }
}

schedule_action &schedule_action::id(const schedule_action_id &new_id) {
    m_id = new_id;
    return *this;
}

schedule_action &schedule_action::add_pin(const std::pair<schedule_gpio_id, gpio_pin::action> &new_pin) {
    m_pins.emplace_back(new_pin);
    return *this;
}

bool schedule_action::operator()() {
    bool result = true;

    for (auto [current_pin, current_action] : m_pins) {
        result &= schedule_gpio::control_pin(current_pin, current_action);
    }
    return result;
}

const schedule_action_id &schedule_action::id() const { return m_id; }

const std::vector<std::pair<schedule_gpio_id, gpio_pin::action>> &schedule_action::pins() const { return m_pins; }

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

    // Check gpios by turning off all pins

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
