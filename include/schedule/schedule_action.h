#pragma once

#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "io/outputs/output_value.h"
#include "io/outputs/outputs.h"

using json = nlohmann::json;

using schedule_action_id = std::string;

class schedule_action {
   public:
    static bool is_valid_id(const schedule_action_id &id);
    static std::vector<schedule_action_id> execute_actions(const std::vector<schedule_action_id> &ids);

    schedule_action() = default;
    schedule_action(const schedule_action &other) = delete;
    schedule_action(schedule_action &&other);

    schedule_action &id(const schedule_action_id &new_id);
    schedule_action &attach_output(const std::pair<output_id, output_value> &new_output);

    const schedule_action_id &id() const;
    const std::vector<std::pair<output_id, output_value>> &outputs() const;

   private:
    static bool add_action(json &schedule_action_description);

    schedule_action_id m_id;
    std::vector<std::pair<output_id, output_value>> m_outputs;

    static inline std::vector<std::unique_ptr<schedule_action>> _actions;
    static inline std::recursive_mutex _instance_mutex;

    friend class schedule;
};
