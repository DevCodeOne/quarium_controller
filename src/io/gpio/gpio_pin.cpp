#include "io/gpio/gpio_pin.h"
#include "config.h"
#include "io/gpio/gpio_chip.h"
#include "logger.h"

gpio_pin_id::gpio_pin_id(unsigned int id, std::shared_ptr<gpio_chip> chip)
    : m_gpiochip_path(chip->path_to_file()), m_id(id) {}

unsigned int gpio_pin_id::id() const { return m_id; }

std::shared_ptr<gpio_pin> gpio_pin_id::open_pin() {
    auto chip_instance = gpio_chip::instance(m_gpiochip_path);
    if (!chip_instance) {
        return nullptr;
    }

    return chip_instance->open_pin(*this);
}

const std::filesystem::path &gpio_pin_id::gpio_chip_path() const { return m_gpiochip_path; }

std::optional<output_value> gpio_pin::is_overriden() const { return m_overriden_value; }

// TODO remove code duplication in gpio_pin::update_gpio
output_value gpio_pin::current_state() const {
    switch_output controlled_state = m_controlled_value;
    if (m_overriden_value.has_value()) {
        controlled_state = *m_overriden_value;
    }

    return controlled_state;
}

gpio_pin::gpio_pin(gpio_pin_id id, gpiod::gpiod_line line) : m_id(id), m_line(std::move(line)) {}

gpio_pin::gpio_pin(gpio_pin &&other) : m_id(std::move(other.m_id)), m_line(std::move(other.m_line)) {}

std::optional<gpio_pin> gpio_pin::open(gpio_pin_id id) {
    auto chip_instance = gpio_chip::instance(id.gpio_chip_path());

    if (!chip_instance) {
        logger::instance()->critical("Chip of the provided gpio_pin_id is not valid");
        return {};
    }

    gpiod::gpiod_line line = chip_instance->m_chip.get_line(id.id());

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

bool gpio_pin::control_output(const output_value &value) {
    auto chip_instance = gpio_chip::instance(m_id.gpio_chip_path());
    if (!m_line || !chip_instance) {
        return false;
    }

    auto contained_value = value.get<switch_output>();

    if (!contained_value.has_value()) {
        return false;
    }

    switch (*contained_value) {
        case switch_output::on:
        case switch_output::off:
            logger::instance()->info("Turning gpio {} of chip {} {}", m_id.id(), m_id.gpio_chip_path().c_str(),
                                     *contained_value == switch_output::on ? "on" : "off");
            break;
        case switch_output::toggle:
            logger::instance()->info("Toggle gpio {} of chip {}", m_id.id(), m_id.gpio_chip_path().c_str());
            break;
    }

    m_controlled_value = *contained_value;
    return update_gpio();
}

bool gpio_pin::override_with(const output_value &value) {
    auto contained_value = value.get<switch_output>();

    if (!contained_value.has_value()) {
        return false;
    }

    m_overriden_value = *contained_value;
    logger::instance()->info("Override gpio {} control to be {}", m_id.id(),
                             *m_overriden_value == switch_output::on ? "on" : "off");
    return update_gpio();
}

bool gpio_pin::restore_control() {
    m_overriden_value = {};
    logger::instance()->info("Restore gpio {} to be controlled by the schedule again", m_id.id());
    return update_gpio();
}

bool gpio_pin::update_gpio() {
    switch_output to_write = m_controlled_value;

    if (m_overriden_value.has_value()) {
        logger::instance()->info("Action is overriden");
        to_write = *m_overriden_value;
    }

    int value = m_line.get_value();

    if (value == -1) {
        return false;
    }

    if ((switch_output)value == to_write) {
        return true;
    }

    switch (to_write) {
        case switch_output::on:
            return m_line.set_value(1) == 0 ? true : false;
        case switch_output::off:
            return m_line.set_value(0) == 0 ? true : false;
        case switch_output::toggle:
            return m_line.set_value(!value) == 0 ? true : false;
        default:
            logger::instance()->critical("Invalid switch_output to write to gpio {}", gpio_id());
            break;
    }

    return false;
}

unsigned int gpio_pin::gpio_id() const { return m_id.id(); }

nlohmann::json gpio_pin::serialize() const {
    nlohmann::json serialized;

    // TODO action to string
    serialized["id"] = m_id.id();
    serialized["controlled_action"] = (int)m_controlled_value;
    if (m_overriden_value) {
        serialized["overriden_action"] = (int)*m_overriden_value;
    }

    return std::move(serialized);
}

std::unique_ptr<output_interface> gpio_pin::create_for_interface(const nlohmann::json &description) {
    if (!description.is_object()) {
        return nullptr;
    }

    nlohmann::json pin_entry = description["pin"];
    nlohmann::json default_entry = description["default"];

    if (pin_entry.is_null() || !pin_entry.is_number_unsigned()) {
        return nullptr;
    }

    if (default_entry.is_null()) {
        default_entry = "on";
    }

    if (!default_entry.is_string()) {
        return nullptr;
    }

    auto pin_number = pin_entry.get<unsigned int>();
    auto default_as_string = default_entry.get<std::string>();
    switch_output default_value = switch_output::off;

    if (default_as_string == "on") {
        default_value = switch_output::on;
    }

    auto gpio_chip_instance = gpio_chip::instance();

    if (!gpio_chip_instance) {
        return nullptr;
    }

    gpio_pin_id pin_id(pin_number, gpio_chip_instance);

    auto created_pin = gpio_pin::open(pin_id);

    if (!created_pin.has_value()) {
        return nullptr;
    }

    return std::unique_ptr<output_interface>(new gpio_pin(std::move(*created_pin)));
}

// TODO test these
bool operator<(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs.id() < rhs.id(); }

bool operator>(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return !(lhs < rhs || lhs == rhs); }

bool operator==(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs.id() == rhs.id(); }

bool operator!=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return !(lhs == rhs); }

bool operator<=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs < rhs || lhs == rhs; }

bool operator>=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs > rhs || lhs == rhs; }
