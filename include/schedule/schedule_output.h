#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <variant>

#include "nlohmann/json.hpp"

#include "schedule/output_interface.h"

using json = nlohmann::json;

class schedule_output {
   public:
    static bool is_valid_id(const schedule_output_id &id);
    static bool control_pin(const schedule_output_id &id, const output_value &action);
    static std::optional<output_value> is_overriden(const schedule_output_id &id);
    static bool override_with(const schedule_output_id &id, const output_value &action);
    static bool restore_control(const schedule_output_id &id);
    static std::optional<output_value> current_state(const schedule_output_id &id);
    static std::vector<schedule_output_id> get_ids();

   private:
    static bool add_gpio(json &gpio_description);

    static inline std::map<schedule_output_id, std::unique_ptr<output_interface>> _outputs;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};
