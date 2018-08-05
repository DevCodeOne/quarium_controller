#include "gpio_handler.h"
#include "logger.h"

gpio_pin_id::gpio_pin_id(unsigned int id, std::filesystem::path gpio_chip_path)
    : m_gpio_chip_path(gpio_chip_path), m_id(id) {}

unsigned int gpio_pin_id::id() const { return m_id; }

const std::filesystem::path &gpio_pin_id::gpio_chip_path() const { return m_gpio_chip_path; }

gpio_pin::gpio_pin(gpio_pin_id id, gpiod_line *line) : m_id(id) {
    m_line = std::shared_ptr<gpiod_line>(line, [](gpiod_line *line) { gpiod_line_release(line); });
}

gpio_pin::gpio_pin(gpio_pin &&other) : m_id(std::move(other.m_id)), m_line(other.m_line) { other.m_line = nullptr; }

std::optional<gpio_pin> gpio_pin::open(gpio_pin_id id) {
    auto chip = gpio_chip::instance(id.gpio_chip_path());

    if (chip.has_value() == false || chip.value() == nullptr) {
        return {};
    }

    gpiod_line *line = gpiod_chip_get_line(chip.value()->m_chip, id.id());

    if (line == nullptr) {
        return {};
    }

    bool result = gpiod_line_request_output(line, "quarium_controller", 0);

    if (!result) {
        return {};
    }

    return gpio_pin(std::move(id), line);
}

bool gpio_pin::control(const action &act) {
    if (m_line == nullptr) {
        return false;
    }

    switch (act) {
        case action::off:
            logger::instance()->info("Turning gpio {} of chip {} on", m_id.id(), m_id.gpio_chip_path().c_str());
            break;
        case action::on:
            logger::instance()->info("Turning gpio {} of chip {} on", m_id.id(), m_id.gpio_chip_path().c_str());
            break;
        case action::toggle:
            logger::instance()->info("Turning gpio {} of chip {} on", m_id.id(), m_id.gpio_chip_path().c_str());
            break;
    }

    return true;
}

bool operator<(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs.id() < rhs.id(); }

std::optional<std::shared_ptr<gpio_chip>> gpio_chip::instance(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    if (auto result = _gpiochip_access_map.find(gpio_chip_path); result != _gpiochip_access_map.cend()) {
        return result->second;
    }

    if (!reserve_gpiochip(gpio_chip_path)) {
        return {};
    }

    return _gpiochip_access_map[gpio_chip_path];
}

std::optional<gpio_chip> gpio_chip::open(const std::filesystem::path &gpio_chip_path) {
    gpiod_chip *opened_chip = gpiod_chip_open(gpio_chip_path.c_str());

    if (!opened_chip) {
        logger::instance()->info("Couldn't open gpiochip");
        return {};
    }

    return gpio_chip(gpio_chip_path, opened_chip);
}

bool gpio_chip::reserve_gpiochip(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    auto result = _gpiochip_access_map.find(gpio_chip_path);

    if (result != _gpiochip_access_map.cend()) {
        return false;
    }

    auto created_gpio_chip = gpio_chip::open(gpio_chip_path);

    if (!created_gpio_chip) {
        return false;
    }

    _gpiochip_access_map.emplace(gpio_chip_path, std::make_shared<gpio_chip>(std::move(created_gpio_chip.value())));
    return true;
}

bool gpio_chip::free_gpiochip(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    auto result = _gpiochip_access_map.find(gpio_chip_path);

    if (result == _gpiochip_access_map.cend()) {
        return false;
    }

    _gpiochip_access_map.erase(gpio_chip_path);

    return true;
}

gpio_chip::gpio_chip(const std::filesystem::path &gpio_dev, gpiod_chip *chip)
    : m_gpiochip_path(gpio_dev), m_chip(chip) {}

gpio_chip::gpio_chip(gpio_chip &&other)
    : m_gpiochip_path(std::move(other.m_gpiochip_path)), m_chip(other.m_chip), m_reserved_pins(other.m_reserved_pins) {
    other.m_gpiochip_path = "";
    other.m_chip = nullptr;
    other.m_reserved_pins = decltype(m_reserved_pins)();
}

gpio_chip::~gpio_chip() {
    gpiod_chip_close(m_chip);
    free_gpiochip(m_gpiochip_path);
}

bool gpio_chip::control_pin(const gpio_pin_id &id, const gpio_pin::action &action) {
    if (id.gpio_chip_path() != m_gpiochip_path) {
        logger::instance()->warn("Wrong gpio_chip path");
        return false;
    }

    if (auto result = m_reserved_pins.find(id); result != m_reserved_pins.cend()) {
        return result->second->control(action);
    }

    auto created_pin = gpio_pin::open(id);

    if (!created_pin) {
        return false;
    }

    if (auto result =
            m_reserved_pins.emplace(std::make_pair(id, std::make_shared<gpio_pin>(std::move(created_pin.value()))));
        result.second) {
        logger::instance()->warn("Reserved new pin {}", id.id());
        return result.first->second->control(action);
    }

    return false;
}
