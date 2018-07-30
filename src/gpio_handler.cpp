#include "gpio_handler.h"

std::optional<gpio_chip> gpio_chip::open(const std::filesystem::path &gpio_chip_path) {
    if (!reserve_gpiochip(gpio_chip_path)) {
        return {};
    }

    return gpio_chip(gpio_chip_path);
}

bool gpio_chip::reserve_gpiochip(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::mutex> _access_map_guard{_instance_mutex};

    auto result = _gpiochip_access_map.find(gpio_chip_path);

    if (result != _gpiochip_access_map.cend() && result->second) {
        return false;
    }

    _gpiochip_access_map[gpio_chip_path] = true;

    return true;
}

bool gpio_chip::free_gpiochip(const std::filesystem::path &gpio_chip_path) {
    std::lock_guard<std::mutex> _access_map_guard{_instance_mutex};

    auto result = _gpiochip_access_map.find(gpio_chip_path);

    if (result == _gpiochip_access_map.cend() || !result->second) {
        return false;
    }

    _gpiochip_access_map[gpio_chip_path] = false;

    return true;
}

gpio_chip::gpio_chip(const std::filesystem::path &gpio_dev) : m_gpiochip_path(gpio_dev) {}

gpio_chip::gpio_chip(gpio_chip &&other) : m_gpiochip_path(std::move(other.m_gpiochip_path)) {
    other.m_gpiochip_path = "";
}

gpio_chip::~gpio_chip() { free_gpiochip(m_gpiochip_path); }
