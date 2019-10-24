#pragma once

#include <algorithm>
#include <future>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <vector>

#include "io/outputs/output_value.h"
#include "io/outputs/outputs.h"

class batch_output_control final {
   public:
    batch_output_control() = default;
    batch_output_control(const std::vector<std::pair<output_id, output_value>> &output_control);

    batch_output_control &add_output_control(std::pair<output_id, output_value> output_control);
    batch_output_control &add_output_controls(const std::vector<std::pair<output_id, output_value>> &output_control);
    batch_output_control &optimize_outputs(bool value);

    size_t number_of_occured_errors() const;
    bool optimize_outputs() const;
    std::vector<output_id> occured_errors() const;
    std::vector<std::pair<output_id, output_value>> output_controls() const;

   private:
    std::vector<std::pair<output_id, output_value>> m_output_controls;
    std::vector<output_id> m_failures;
    bool m_optimize_outputs = false;
};

enum struct output_control_result : uint8_t { success = 0, failure = 1, skipped = 2 };

struct batch_output_control_result {
    using output_control_type = std::pair<output_id, output_value>;

    const bool batch_result = false;
    const std::vector<output_control_type> output_controls;
    const std::vector<std::pair<output_control_type, output_control_result>> control_results;
};

class output_scheduler final {
   public:
    // static std::shared_ptr<output_scheduler> instance();

    // Since this is asynchronous use shared_ptr so the batch_output_control is not invalidated before the operation is
    // done
    static std::future<batch_output_control_result> execute_batch_output_control(const batch_output_control &job);

   private:
    output_scheduler() = delete;

    static batch_output_control_result do_output_job(batch_output_control job);

    // static inline std::shared_ptr<output_scheduler> _instance = nullptr;
    // static inline std::mutex _instance_mutex{};

    // std::vector<std::shared_ptr<batch_output_control>> m_outputs_to_set;
};
