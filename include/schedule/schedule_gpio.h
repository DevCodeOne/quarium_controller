#pragma once

#include <mutex>
#include <vector>
#include <string>

#include "nlohmann/json.hpp"

#include "gpio_handler.h"

using schedule_gpio_id = std::string;

using json = nlohmann::json;

class schedule_gpio {
   public:
    static bool is_valid_id(const schedule_gpio_id &id);
    static bool control_pin(const schedule_gpio_id &id, const gpio_pin::action &action);
    static std::vector<schedule_gpio_id> get_ids();

    schedule_gpio(const schedule_gpio_id &id, const gpio_pin_id &pin_id);
    schedule_gpio(const schedule_gpio &other) = delete;
    schedule_gpio(schedule_gpio &&other);

    const schedule_gpio_id &id() const;
    const gpio_pin_id &pin() const;

   private:
    static bool add_gpio(json &gpio_description);

    schedule_gpio_id m_id;
    gpio_pin_id m_pin_id;

    static inline std::vector<std::unique_ptr<schedule_gpio>> _gpios;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};


