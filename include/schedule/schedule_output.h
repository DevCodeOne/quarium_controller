#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "gpio/gpio_chip.h"

using schedule_output_id = std::string;

using json = nlohmann::json;

class schedule_output {
   public:
    static bool is_valid_id(const schedule_output_id &id);
    static bool control_pin(const schedule_output_id &id, const gpio_pin::action &action);
    static std::optional<gpio_pin::action> is_overriden(const schedule_output_id &id);
    static bool override_with(const schedule_output_id &id, const gpio_pin::action &action);
    static bool restore_control(const schedule_output_id &id);
    static std::optional<gpio_pin::action> current_state(const schedule_output_id &id);
    static std::vector<schedule_output_id> get_ids();

    schedule_output(const schedule_output_id &id, std::shared_ptr<gpio_pin> pin, const gpio_pin::action &default_state);
    schedule_output(const schedule_output &other) = delete;
    schedule_output(schedule_output &&other);

    schedule_output &default_state(gpio_pin::action &new_default_value);

    const schedule_output_id &id() const;
    const gpio_pin::action &default_state() const;

   private:
    static bool add_gpio(json &gpio_description);

    std::shared_ptr<gpio_pin> pin();

    schedule_output_id m_id = "";
    std::shared_ptr<gpio_pin> m_pin = nullptr;
    gpio_pin::action m_default_state = gpio_pin::action::off;

    static inline std::vector<std::unique_ptr<schedule_output>> _gpios;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};
