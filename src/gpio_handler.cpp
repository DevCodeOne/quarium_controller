#include "gpio_handler.h"
#include "logger.h"

gpio_pin_id::gpio_pin_id(unsigned int id, std::filesystem::path gpio_chip_path)
    : m_gpio_chip_path(gpio_chip_path), m_id(id) {}

unsigned int gpio_pin_id::id() const { return m_id; }

std::optional<gpio_pin::action> gpio_pin::is_overriden() const { return m_overriden_action; }

const std::filesystem::path &gpio_pin_id::gpio_chip_path() const { return m_gpio_chip_path; }

gpio_pin::gpio_pin(gpio_pin_id id, gpiod_line *line) : m_id(id) {
    m_line = std::shared_ptr<gpiod_line>(line, [](gpiod_line *line) { gpiod_line_release(line); });
}

gpio_pin::gpio_pin(gpio_pin &&other) : m_id(std::move(other.m_id)), m_line(other.m_line) { other.m_line = nullptr; }

std::optional<gpio_pin> gpio_pin::open(gpio_pin_id id) {
    auto chip = gpio_chip::instance(id.gpio_chip_path());

    if (chip.has_value() == false || chip.value() == nullptr) {
        logger::instance()->critical("Couldn't get an instance of gpiochip", id.gpio_chip_path().c_str());
        return {};
    }

    gpiod_line *line = gpiod_chip_get_line(chip.value()->m_chip, id.id());

    if (line == nullptr) {
        logger::instance()->critical("Couldn't get line with id {}", id.id());
        return {};
    }

    int result = gpiod_line_request_output(line, "quarium_controller", 0);

    if (result == -1) {
        logger::instance()->critical("Couldn't request line with id {}", id.id());
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
            logger::instance()->info("Turning gpio {} of chip {} off", m_id.id(), m_id.gpio_chip_path().c_str());
            break;
        case action::toggle:
            logger::instance()->info("Toggle gpio {} of chip {}", m_id.id(), m_id.gpio_chip_path().c_str());
            break;
    }

    m_controlled_action = act;
    return update_gpio();
}

bool gpio_pin::override_with(const action &act) {
    m_overriden_action = act;
    logger::instance()->info("Override gpio {} control to be {}", m_id.id(), act == action::on ? "off" : "on");
    m_overriden_action = act;
    return update_gpio();
}

bool gpio_pin::restore_control() {
    m_overriden_action = {};
    return update_gpio();
}

bool gpio_pin::update_gpio() {
    action to_write = m_controlled_action;

    if (m_overriden_action.has_value()) {
        logger::instance()->info("Action is overriden");
        to_write = m_overriden_action.value();
    }

    int value = gpiod_line_get_value(m_line.get());

    if (value == -1) {
        return false;
    }

    if ((action) value == to_write) {
        return true;
    }

    switch (to_write) {
        case action::on:
            return gpiod_line_set_value(m_line.get(), 1) == 0 ? true : false;
        case action::off:
            return gpiod_line_set_value(m_line.get(), 0) == 0 ? true : false;
        case action::toggle:
            return gpiod_line_set_value(m_line.get(), value) == 0 ? true : false;
        default:
            logger::instance()->critical("Invalid action to write to gpio {}", gpio_id());
            break;
    }

    return false;
}

unsigned int gpio_pin::gpio_id() const { return m_id.id(); }

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
        logger::instance()->critical("Couldn't open gpiochip {}", gpio_chip_path.c_str());
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
    : m_gpiochip_path(std::move(other.m_gpiochip_path)),
      m_chip(other.m_chip),
      m_reserved_pins(std::move(other.m_reserved_pins)) {
    other.m_gpiochip_path = "";
    other.m_chip = nullptr;
}

gpio_chip::~gpio_chip() {
    if (m_chip) {
        // Possible problem in libgpiod where the chip might include the pins as resources, so the pins have to be
        // destroyed before the chip
        m_reserved_pins.clear();
        gpiod_chip_close(m_chip);
    }
}

bool gpio_chip::control_pin(const gpio_pin_id &id, const gpio_pin::action &action) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    auto pin = access_pin(id);

    if (!pin) {
        return false;
    }

    return pin->control(action);
}

std::shared_ptr<gpio_pin> gpio_chip::access_pin(const gpio_pin_id &id) {
    std::lock_guard<std::recursive_mutex> _access_map_guard{_instance_mutex};

    if (id.gpio_chip_path() != m_gpiochip_path) {
        logger::instance()->warn("Wrong gpio_chip path");
        return nullptr;
    }

    if (auto result = m_reserved_pins.find(id); result != m_reserved_pins.cend()) {
        return result->second;
    }

    auto created_pin = gpio_pin::open(id);

    if (!created_pin) {
        logger::instance()->critical("Couldn't open pin with id {}", id.id());
        return nullptr;
    }

    if (auto result =
            m_reserved_pins.emplace(std::make_pair(id, std::make_shared<gpio_pin>(std::move(created_pin.value()))));
        result.second) {
        logger::instance()->warn("Reserved new pin {}", id.id());
        return result.first->second;
    } else {
        logger::instance()->warn("Couldn't create pin with id {}", id.id());
    }

    return nullptr;
}
