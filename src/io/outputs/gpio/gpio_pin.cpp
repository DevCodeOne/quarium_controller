#include "io/outputs/gpio/gpio_pin.h"

#include "config.h"
#include "io/outputs/gpio/gpio_chip.h"
#include "logger.h"

gpio_pin_id::gpio_pin_id(unsigned int id, std::shared_ptr<gpio_chip> chip) : m_gpiochip_instance(chip), m_id(id) {}

unsigned int gpio_pin_id::id() const { return m_id; }

std::shared_ptr<gpio_pin> gpio_pin_id::open_pin() {
    auto chip_instance = m_gpiochip_instance.lock();

    if (!chip_instance) {
        return nullptr;
    }

    return chip_instance->open_pin(*this);
}

std::optional<output_value> gpio_pin::is_overriden() const {
    if (m_overriden_value.has_value()) {
        return std::optional<output_value>{output_value(*m_overriden_value)};
    }

    return {};
}

output_value gpio_pin::current_state() const {
    switch_output controlled_state = m_controlled_value;
    if (m_overriden_value.has_value()) {
        controlled_state = *m_overriden_value;
    }

    return controlled_state;
}

gpio_pin::gpio_pin(std::weak_ptr<gpio_chip> chip_instance, gpio_pin_id id, gpiod::gpiod_line line)
    : m_id(id), m_line(std::move(line)), m_gpiochip_instance(chip_instance) {}

gpio_pin::gpio_pin(gpio_pin &&other)
    : m_id(std::move(other.m_id)),
      m_line(std::move(other.m_line)),
      m_controlled_value(std::move(other.m_controlled_value)),
      m_overriden_value(std::move(other.m_overriden_value)),
      m_gpiochip_instance(other.m_gpiochip_instance) {}

std::optional<gpio_pin> gpio_pin::open(std::shared_ptr<gpio_chip> chip_instance, gpio_pin_id id) {
    auto logger_instance = logger::instance();
    if (!chip_instance) {
        logger_instance->critical("Chip of the provided gpio_pin_id is not valid");
        return std::nullopt;
    }

    gpiod::gpiod_line line = chip_instance->m_chip.get_line(id.id());

    if (!line) {
        logger_instance->critical("Couldn't get line with id {}", id.id());
        return std::nullopt;
    }

    auto invert_signal_entry = config::instance()->find("invert_output");
    auto invert_signal = false;

    if (!invert_signal_entry.is_null() && invert_signal_entry.is_boolean()) {
        invert_signal = invert_signal_entry.get<bool>();
    }
    int result =
        line.request_output_flags("quarium_controller", invert_signal ? GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW : 0, 0);

    if (result == -1) {
        logger_instance->critical("Couldn't request line with id {}", id.id());
        return std::nullopt;
    }

    return gpio_pin(chip_instance, std::move(id), std::move(line));
}

bool gpio_pin::control_output(const output_value &value) {
    auto contained_value = value.get<switch_output>();

    if (!contained_value.has_value()) {
        return false;
    }
    m_controlled_value = *contained_value;

    auto chip_instance = m_gpiochip_instance.lock();
    if (!m_line || !chip_instance) {
        return false;
    }

    switch (*contained_value) {
        case switch_output::on:
        case switch_output::off:
            logger::instance()->info("Turning gpio {} of chip {} {}", m_id.id(), chip_instance->path_to_file().c_str(),
                                     *contained_value == switch_output::on ? "on" : "off");
            break;
        case switch_output::toggle:
            logger::instance()->info("Toggle gpio {} of chip {}", m_id.id(), chip_instance->path_to_file().c_str());
            break;
    }

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
    logger::instance()->info("Restore gpio {} to be controlled by the schedule again : it is now {}", m_id.id(),
                             m_controlled_value == switch_output::on ? "on" : "off");
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

    // TODO: make path configurable
    auto gpio_chip_instance = gpio_chip::instance();

    if (!gpio_chip_instance) {
        return nullptr;
    }

    gpio_pin_id pin_id(pin_number, gpio_chip_instance);

    auto created_pin = gpio_pin::open(gpio_chip_instance, pin_id);

    if (!created_pin.has_value()) {
        return nullptr;
    }

    // Setting the default value when creating the pin
    // TODO consider doing this in a different place but the description gets parsed here so this place is not entirely
    // wrong
    created_pin->control_output(default_value);
    return std::unique_ptr<output_interface>(new gpio_pin(std::move(*created_pin)));
}

// TODO test these
bool operator<(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs.id() < rhs.id(); }

bool operator>(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return !(lhs < rhs || lhs == rhs); }

bool operator==(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs.id() == rhs.id(); }

bool operator!=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return !(lhs == rhs); }

bool operator<=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs < rhs || lhs == rhs; }

bool operator>=(const gpio_pin_id &lhs, const gpio_pin_id &rhs) { return lhs > rhs || lhs == rhs; }
