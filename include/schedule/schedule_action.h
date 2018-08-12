#pragma once

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "schedule_gpio.h"

using json = nlohmann::json;

using schedule_action_id = std::string;

class schedule_action {
   public:
    static bool is_valid_id(const schedule_action_id &id);
    static bool execute_action(const schedule_action_id &id);

    schedule_action() = default;
    schedule_action(const schedule_action &other) = delete;
    schedule_action(schedule_action &&other);

    schedule_action &id(const schedule_action_id &new_id);
    schedule_action &add_pin(const std::pair<schedule_gpio_id, gpio_pin::action> &new_pin);

    const schedule_action_id &id() const;
    const std::vector<std::pair<schedule_gpio_id, gpio_pin::action>> &pins() const;

    bool operator()();

   private:
    static bool add_action(json &schedule_action_description);

    schedule_action_id m_id;
    std::vector<std::pair<schedule_gpio_id, gpio_pin::action>> m_pins;

    static inline std::vector<std::unique_ptr<schedule_action>> _actions;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};
