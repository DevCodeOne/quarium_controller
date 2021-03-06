#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <variant>

#include "io/outputs/output_interface.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

using output_id = std::string;

class outputs {
   public:
    static bool is_valid_id(const output_id &id);
    static bool control_output(const output_id &id, const output_value &action);
    static std::optional<output_value> is_overriden(const output_id &id);
    static bool override_with(const output_id &id, const output_value &action);
    static bool restore_control(const output_id &id);
    static std::optional<output_value> current_state(const output_id &id);
    static std::vector<output_id> get_ids();

   private:
    using outputs_map_type = std::map<output_id, std::unique_ptr<output_interface>>;

    static bool add_output(json &gpio_description);
    static outputs_map_type::iterator find_output(const output_id &id);

    static inline outputs_map_type _outputs;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};
