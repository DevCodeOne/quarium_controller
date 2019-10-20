#pragma once

#include <memory>
#include <mutex>
#include <stack>
#include <utility>

#include "io/outputs/output_value.h"
#include "io/outputs/outputs.h"

class output_control_job final {
   public:
    output_control_job() = default;

    output_control_job &add_output_control(std::pair<output_id, output_value> ouput_control);
    output_control_job &add_output_controls(std::stack<std::pair<output_id, output_value>> output_control);
    std::pair<output_id, output_value> current() const;
    std::pair<output_id, output_value> next() const;

   private:
    std::stack<std::pair<output_id, output_value>> m_output_controls;
};

class output_scheduler final {
   public:
    static std::shared_ptr<output_scheduler> instance();

    // Since this is asynchronous use shared_ptr so the output_control_job is not invalidated before the operation is
    // done
    output_scheduler add_output_control_job(std::shared_ptr<output_control_job> job);

   private:
    output_scheduler();

    static inline std::shared_ptr<output_scheduler> _instance = nullptr;
    static inline std::mutex _instance_mutex{};

    std::stack<std::shared_ptr<output_control_job>> m_outputs_to_set;
};
