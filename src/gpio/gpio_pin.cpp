#include "gpio/gpio_pin.h"
#include "config.h"
#include "gpio/gpio_chip.h"
#include "logger.h"

gpio_pin_id::gpio_pin_id(unsigned int id, std::shared_ptr<gpio_chip> chip) : m_chip(chip), m_id(id) {}

unsigned int gpio_pin_id::id() const { return m_id; }

std::shared_ptr<gpio_pin> gpio_pin_id::open_pin() {
    if (!m_chip) {
        return nullptr;
    }

    return m_chip->open_pin(*this);
}

const std::shared_ptr<gpio_chip> gpio_pin_id::chip() const { return m_chip; }

std::optional<gpio_pin::action> gpio_pin::is_overriden() const { return m_overriden_action; }

// TODO remove code duplication in gpio_pin::update_gpio
gpio_pin::action gpio_pin::current_state() const {
    action controlled_state = m_controlled_action;
    if (m_overriden_action.has_value()) {
        controlled_state = m_overriden_action.value();
    }

    return controlled_state;
}

gpio_pin::gpio_pin(gpio_pin_id id, gpiod::gpiod_line line) : m_id(id), m_line(std::move(line)) {}

gpio_pin::gpio_pin(gpio_pin &&other) : m_id(std::move(other.m_id)), m_line(std::move(other.m_line)) {}

std::optional<gpio_pin> gpio_pin::open(gpio_pin_id id) {
    auto chip = id.chip();

    if (!chip) {
        logger::instance()->critical("Chip of the provided gpio_pin_id is not valid");
        return {};
    }

    gpiod::gpiod_line line = chip->m_chip.get_line(id.id());

    if (!line) {
        logger::instance()->critical("Couldn't get line with id {}", id.id());
        return {};
    }

    auto invert_signal_entry = config::instance()->find("invert_output");
    auto invert_signal = false;

    if (!invert_signal_entry.is_null() && invert_signal_entry.is_boolean()) {
        invert_signal = invert_signal_entry.get<bool>();
    }
    int result =
        line.request_output_flags("quarium_controller", invert_signal ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0, 0);

    if (result == -1) {
        logger::instance()->critical("Couldn't request line with id {}", id.id());
        return {};
    }

    return gpio_pin(std::move(id), std::move(line));
}

bool gpio_pin::control(const action &act) {
    if (!m_line || !m_id.chip()) {
        return false;
    }

    switch (act) {
        case action::on:
        case action::off:
            logger::instance()->info("Turning gpio {} of chip {} {}", m_id.id(), m_id.chip()->path_to_file().c_str(),
                                     act == action::on ? "on" : "off");
            break;
        case action::toggle:
            logger::instance()->info("Toggle gpio {} of chip {}", m_id.id(), m_id.chip()->path_to_file().c_str());
            break;
    }

    m_controlled_action = act;
    return update_gpio();
}

bool gpio_pin::override_with(const action &act) {
    m_overriden_action = act;
    logger::instance()->info("Override gpio {} control to be {}", m_id.id(), act == action::on ? "on" : "off");
    return update_gpio();
}

bool gpio_pin::restore_control() {
    m_overriden_action = {};
    logger::instance()->info("Restore gpio {} to be controlled by the schedule again", m_id.id());
    return update_gpio();
}

bool gpio_pin::update_gpio() {
    action to_write = m_controlled_action;

    if (m_overriden_action.has_value()) {
        logger::instance()->info("Action is overriden");
        to_write = m_overriden_action.value();
    }

    int value = m_line.get_value();

    if (value == -1) {
        return false;
    }

    if ((action)value == to_write) {
        return true;
    }

    switch (to_write) {
        case action::on:
            return m_line.set_value(1) == 0 ? true : false;
        case action::off:
            return m_line.set_value(0) == 0 ? true : false;
        case action::toggle:
            return m_line.set_value(!value) == 0 ? true : false;
        default:
            logger::instance()->critical("Invalid action to write to gpio {}", gpio_id());
            break;
    }

    return false;
}

unsigned int gpio_pin::gpio_id() const { return m_id.id(); }

nlohmann::json gpio_pin::serialize() const {
    nlohmann::json serialized;

    // TODO action to string
    serialized["id"] = m_id.id();
    serialized["controlled_action"] = (int)m_controlled_action;
    if (m_overriden_action) {
        serialized["overriden_action"] = (int)m_overriden_action.value();
    }

    return std::move(serialized);
}

// TODO test these
bool operator<(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs.id() < rhs.id(); }

bool operator>(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return !(lhs < rhs || lhs == rhs); }

bool operator==(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs.id() == rhs.id(); }

bool operator!=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return !(lhs == rhs); }

bool operator<=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs < rhs || lhs == rhs; }

bool operator>=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs > rhs || lhs == rhs; }
