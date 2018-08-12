#include "schedule/schedule_gpio.h"
#include "logger.h"

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

    std::vector<schedule_gpio_id> ids;
    ids.reserve(_gpios.size());

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


