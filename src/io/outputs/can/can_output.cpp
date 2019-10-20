#include "io/outputs/can/can_output.h"
#include "logger.h"

std::unique_ptr<can_output> can_output::create_for_interface(const nlohmann::json &description) {
    if (!description.is_object()) {
        return nullptr;
    }

    nlohmann::json can_device_entry = description["can_device"];
    nlohmann::json object_identifier_entry = description["object_identifier"];
    nlohmann::json default_entry = description["default"];

    if (can_device_entry.is_null() || !can_device_entry.is_string()) {
        return nullptr;
    }

    if (object_identifier_entry.is_null() || !object_identifier_entry.is_number_unsigned()) {
        return nullptr;
    }

    // If there is a default entry, it should be otherwise, otherwise default should be 0
    if (!default_entry.is_null() && !default_entry.is_number_unsigned()) {
        return nullptr;
    }

    auto can_device = can_device_entry.get<std::string>();
    auto object_identifier = object_identifier_entry.get<unsigned int>();
    auto default_value = default_entry.is_null() ? default_entry.get<unsigned int>() : 0u;

    auto can_device_instance = can::instance(can_device);

    if (!can_device_instance) {
        return nullptr;
    }

    return std::unique_ptr<can_output>(
        new can_output(can_device_instance, can_object_identifier(object_identifier), default_value));
}

can_output::can_output(std::shared_ptr<can> can_instance, can_object_identifier identifier,
                       const output_value &initial_value)
    : m_object_identifier(identifier), m_can_instance(can_instance), m_value(initial_value) {}

bool can_output::control_output(const output_value &value) {
    m_value = value;
    return update_value() == can_error_code::ok;
}

bool can_output::override_with(const output_value &value) {
    m_overriden_value = value;
    return update_value() == can_error_code::ok;
}

bool can_output::restore_control() {
    m_overriden_value.reset();
    return update_value() == can_error_code::ok;
}

std::optional<output_value> can_output::is_overriden() const { return m_overriden_value; }

output_value can_output::current_state() const {
    if (m_overriden_value.has_value()) {
        return *m_overriden_value;
    }

    return m_value;
}

can_error_code can_output::update_value() {
    auto logger_instance = logger::instance();
    output_value value = current_state();
    uint32_t data = 0;

    switch (value.current_type()) {
        case output_value_types::number:
            data = static_cast<uint32_t>(*value.get<int>());
            break;
        case output_value_types::number_unsigned:
            data = static_cast<uint32_t>(*value.get<unsigned int>());
            break;
        case output_value_types::switch_output:
            data = static_cast<uint32_t>(*value.get<switch_output>());
            break;
        default:
            return can_error_code::send_error;
    }

    auto result = m_can_instance->send(m_object_identifier, data);

    if (result != can_error_code::ok) {
        logger_instance->warn("Couldn't send data with the canbus");
    }

    return result;
}