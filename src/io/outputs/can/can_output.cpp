#include "io/outputs/can/can_output.h"

#include "io/outputs/output_transition.h"
#include "logger.h"

std::unique_ptr<can_output> can_output::create_for_interface(const nlohmann::json &description) {
    if (!description.is_object()) {
        return nullptr;
    }

    nlohmann::json can_device_entry = description["can_device"];
    nlohmann::json object_identifier_entry = description["object_identifier"];
    nlohmann::json default_entry = description["default"];
    nlohmann::json transition_entry = description["transition"];

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

    // TODO: if there is an error parsing this, return a nullptr, also add the period parameter
    if (!transition_entry.is_null() && transition_entry.is_object()) {
        uint32_t velocity = 1;
        // std::chrono::milliseconds period = std::chrono::seconds(1);
        if (!transition_entry["type"].is_null() && transition_entry["type"].is_string() &&
            transition_entry["type"].get<std::string>() == "linear") {
        }

        if (!transition_entry["velocity"].is_null() && transition_entry["velocity"].is_number_unsigned()) {
            velocity = transition_entry["velocity"].get<unsigned int>();
        }

        // if (!transition_entry["period"].is_null() && transition_entry["period"].is_string()) {
        // }

        return std::unique_ptr<can_output>(
            new can_output(can_device_instance, can_object_identifier(object_identifier), default_value,
                           output_transitions::linear_transition<>(velocity, std::chrono::milliseconds(500))));
    }

    return std::unique_ptr<can_output>(new can_output(can_device_instance, can_object_identifier(object_identifier),
                                                      default_value, output_transitions::instant<>{}));
}

template<typename TransitionStep>
can_output::can_output(std::shared_ptr<can> can_instance, can_object_identifier identifier,
                       const output_value &initial_value, TransitionStep transition)
    : m_object_identifier(identifier),
      m_can_instance(can_instance),
      m_value(initial_value),
      m_transitioner(initial_value) {
    m_transitioner.start_transition_thread(
        [this, transition](auto time_diff, auto &input, const auto &output) {
            bool result = transition(time_diff, input, output);
            result &= sync_values() == can_error_code::ok;
            return result;
        },
        std::chrono::milliseconds(500));
}

bool can_output::control_output(const output_value &value) {
    m_value = value;
    m_transitioner.target_value(value);

    return sync_values() == can_error_code::ok;
}

bool can_output::override_with(const output_value &value) {
    m_overriden_value = value;
    m_transitioner.target_value(value);

    return sync_values() == can_error_code::ok;
}

bool can_output::restore_control() {
    m_overriden_value.reset();
    m_transitioner.target_value(m_value);

    return sync_values() == can_error_code::ok;
}

std::optional<output_value> can_output::is_overriden() const { return m_overriden_value; }

output_value can_output::current_state() const { return m_transitioner.current_value(); }

can_error_code can_output::sync_values() { return update_value(); }

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

    // TODO: request data, so that one can confirm that the data has actually been written
    if (result != can_error_code::ok) {
        logger_instance->warn("Couldn't send data with the canbus");
    }

    return result;
}
