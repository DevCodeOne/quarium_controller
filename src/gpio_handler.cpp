#include "gpio_handler.h"

gpio_pin_id::gpio_pin_id(unsigned int id, std::filesystem::path gpio_chip_path)
    : m_gpio_chip_path(gpio_chip_path), m_id(id) {}

gpio_pin::gpio_pin(gpio_pin_id id) : m_id(id) {}

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

gpio_chip::gpio_chip(gpio_chip &&other) : m_gpiochip_path(std::move(other.m_gpiochip_path)), m_chip(other.m_chip) {
    other.m_gpiochip_path = "";
    other.m_chip = nullptr;
}

gpio_chip::~gpio_chip() {
    gpiod_chip_close(m_chip);
    free_gpiochip(m_gpiochip_path);
}

bool gpio_chip::control_pin(const gpio_pin_id &id, const gpio_pin::action &action) {
}
