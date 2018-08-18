#include "schedule/schedule_gpio.h"
#include "logger.h"

bool schedule_gpio::add_gpio(json &gpio_description) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    json id_entry = gpio_description["id"];
    json pin_entry = gpio_description["pin"];
    json default_entry = gpio_description["default"];

    if (id_entry.is_null() || pin_entry.is_null() || default_entry.is_null()) {
        logger::instance()->critical("A needed entry in a gpio entry was missing {} {} {}",
                                     id_entry.is_null() ? "id" : "", pin_entry.is_null() ? "pin" : "",
                                     default_entry.is_null() ? "default_entry" : "");
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

    if (!default_entry.is_string()) {
        logger::instance()->critical("The default value for the gpio entry with the id {} is not a string", id);
        return false;
    }

    std::string default_value = default_entry.get<std::string>();
    gpio_pin::action default_state = gpio_pin::action::off;

    if (default_value == "on") {
        default_state = gpio_pin::action::on;
    } else if (default_value == "off") {
        default_state = gpio_pin::action::off;
    } else {
        logger::instance()->critical(
            "The default value for the gpio entry with the id {} is not valid it has to be \"on\" or \"off\"", id);
        return false;
    }

    auto pin_id = gpio_pin_id(pin, gpio_chip::default_gpio_dev_path);

    auto chip = gpio_chip::instance();

    // TODO add back later so the user knows that this won't work
    if (!chip) {
        logger::instance()->critical("The gpiochip of the gpio with the id {} is not accessable", id);
        // return false;
    }

    if (chip && chip.value()->control_pin(pin_id, default_state) == false) {
        logger::instance()->critical("The gpio {} on gpiochip {} is not accessable", id,
                                     pin_id.gpio_chip_path().c_str());
        // return false;
    }

    _gpios.emplace_back(std::make_unique<schedule_gpio>(id, pin_id, default_state));
    return true;
}

bool schedule_gpio::is_valid_id(const schedule_gpio_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};
    return std::find_if(_gpios.cbegin(), _gpios.cend(),
                        [&id](const auto &current_action) { return current_action->id() == id; }) != _gpios.cend();
}

bool schedule_gpio::control_pin(const schedule_gpio_id &id, const gpio_pin::action &action) {
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

std::optional<gpio_pin::action> schedule_gpio::is_overriden(const schedule_gpio_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    if (!is_valid_id(id)) {
        return {};
    }

    auto gpio = std::find_if(_gpios.begin(), _gpios.end(),
                             [&id](const auto &current_action) { return current_action->id() == id; });

    auto chip = gpio_chip::instance();

    if (!chip) {
        return {};
    }

    auto pin = chip.value()->access_pin((*gpio)->pin());

    if (!pin) {
        return {};
    }

    return pin->is_overriden();
}

bool schedule_gpio::override_with(const schedule_gpio_id &id, const gpio_pin::action &action) {
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

    auto pin = chip.value()->access_pin((*gpio)->pin());

    if (!pin) {
        return false;
    }

    return pin->override_with(action);
}

bool schedule_gpio::restore_control(const schedule_gpio_id &id) {
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

    auto pin = chip.value()->access_pin((*gpio)->pin());

    if (!pin) {
        return false;
    }

    return pin->restore_control();
}

std::vector<schedule_gpio_id> schedule_gpio::get_ids() {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    std::vector<schedule_gpio_id> ids;
    ids.reserve(_gpios.size());

    for (auto &current_gpio : _gpios) {
        ids.emplace_back(current_gpio->id());
    }

    return ids;
}

schedule_gpio::schedule_gpio(schedule_gpio &&other)
    : m_id(std::move(other.m_id)),
      m_pin_id(std::move(other.m_pin_id)),
      m_default_state(std::move(other.m_default_state)) {}

schedule_gpio::schedule_gpio(const schedule_gpio_id &id, const gpio_pin_id &pin_id,
                             const gpio_pin::action &default_state)
    : m_id(id), m_pin_id(pin_id), m_default_state(default_state) {}

schedule_gpio &schedule_gpio::default_state(gpio_pin::action &new_default_state) {
    m_default_state = new_default_state;
    return *this;
}

const schedule_gpio_id &schedule_gpio::id() const { return m_id; }

const gpio_pin_id &schedule_gpio::pin() const { return m_pin_id; }

const gpio_pin::action &schedule_gpio::default_state() const { return m_default_state; }
