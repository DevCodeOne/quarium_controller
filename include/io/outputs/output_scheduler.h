#pragma once

#include <algorithm>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>

#include "io/outputs/output_value.h"
#include "io/outputs/outputs.h"

class output_control_job final {
   public:
    output_control_job() = default;
    output_control_job(const std::vector<std::pair<output_id, output_value>> &output_control);

    output_control_job &add_output_control(std::pair<output_id, output_value> ouput_control);
    output_control_job &add_output_controls(const std::vector<std::pair<output_id, output_value>> &output_control);

    size_t number_of_occured_errors() const;
    std::vector<output_id> occured_errors() const;
    std::vector<std::pair<output_id, output_value>> output_controls() const;

   private:
    std::vector<std::pair<output_id, output_value>> m_output_controls;
    std::vector<output_id> m_failures;

    mutable std::shared_mutex m_instance_mutex;
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

    std::vector<std::shared_ptr<output_control_job>> m_outputs_to_set;
};
