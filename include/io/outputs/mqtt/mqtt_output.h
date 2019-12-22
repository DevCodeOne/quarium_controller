#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "io/interfaces/mqtt/mqtt.h"
#include "io/outputs/output_interface.h"
#include "nlohmann/json.hpp"
#include "value_transitioner.h"

struct mqtt_topic final : public std::filesystem::path {};

class mqtt_output final : public output_interface {
   public:
    mqtt_output(const mqtt_output &output) = delete;
    mqtt_output(mqtt_output &output) = delete;
    virtual ~mqtt_output() = default;

    mqtt_output &operator=(const mqtt_output &other) = delete;
    mqtt_output &operator=(mqtt_output &&other) = delete;

    virtual bool control_output(const output_value &value) override;
    virtual bool override_with(const output_value &value) override;
    virtual bool restore_control() override;
    virtual std::optional<output_value> is_overriden() const override;
    virtual output_value current_state() const override;

    static std::unique_ptr<mqtt_output> create_for_interface(const nlohmann::json &description);

   private:
    bool sync_values();
    bool update_value();

    template<typename Callable>
    mqtt_output(std::shared_ptr<mqtt> mqtt_instance, mqtt_topic topic, const output_value &initial_value,
                Callable transition_step);

    std::shared_ptr<mqtt> m_mqtt_instance;
    mqtt_topic m_topic;
    output_value m_value;
    std::optional<output_value> m_overriden_value;
    value_transitioner<output_value> m_transitioner;
};

template<typename Callable>
mqtt_output::mqtt_output(std::shared_ptr<mqtt> mqtt_instance, mqtt_topic topic, const output_value &initial_value,
                         Callable transition_step)
    : m_mqtt_instance(mqtt_instance), m_topic(topic), m_value(initial_value), m_transitioner(initial_value) {
    m_transitioner.start_transition_thread(
        [this, transition_step](auto time_diff, auto &input, const auto &output) -> transition_state {
            auto result = transition_step(time_diff, input, output);

            if (result == transition_state::value_did_change || result == transition_state::finished_transition) {
                sync_values();
            }

            return result;
        },
        /* TODO: set this value based on the period value defined in the TransitionStep instance */
        std::chrono::milliseconds(100));
}
