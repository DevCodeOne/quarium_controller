#include "io/outputs/output_scheduler.h"

#include <algorithm>
#include <type_traits>

batch_output_control &batch_output_control::add_output_control(std::pair<output_id, output_value> output_control) {
    m_output_controls.emplace_back(std::move(output_control));
    return *this;
}

batch_output_control &batch_output_control::add_output_controls(
    const std::vector<std::pair<output_id, output_value>> &output_control) {
    m_output_controls.reserve(m_output_controls.size() + output_control.size() + 1);
    std::copy(output_control.cbegin(), output_control.cend(), std::back_inserter(m_output_controls));
    return *this;
}

batch_output_control &batch_output_control::optimize_outputs(bool value) {
    m_optimize_outputs = value;
    return *this;
}

size_t batch_output_control::number_of_occured_errors() const { return m_failures.size(); }

bool batch_output_control::optimize_outputs() const { return m_optimize_outputs; }

std::vector<output_id> batch_output_control::occured_errors() const { return m_failures; }

std::vector<std::pair<output_id, output_value>> batch_output_control::output_controls() const {
    return m_output_controls;
}

std::future<batch_output_control_result> output_scheduler::execute_batch_output_control(
    const batch_output_control &job) {
    batch_output_control job_copy(job);
    return std::async(std::launch::async, do_output_job, std::move(job_copy));
}

batch_output_control_result output_scheduler::do_output_job(batch_output_control job) {
    bool job_result = true;

    std::vector<std::pair<output_id, output_value>> output_controls = job.output_controls();
    std::vector<std::pair<std::pair<output_id, output_value>, output_control_result>> control_results;

    control_results.reserve(output_controls.size());

    if (job.optimize_outputs()) {
        std::vector<std::pair<output_id, output_value>> optimized_outputs;

        // Remove all duplicates from the job, the last value is set and overrides all other values
        std::copy_if(output_controls.crbegin(), output_controls.crend(), std::back_inserter(optimized_outputs),
                     [&optimized_outputs, &control_results](const auto &element_to_copy) -> bool {
                         bool is_already_set = std::any_of(optimized_outputs.cbegin(), optimized_outputs.cend(),
                                                           [&element_to_copy](const auto &current_element) {
                                                               return current_element.first == element_to_copy.first;
                                                           });
                         if (is_already_set) {
                             control_results.emplace_back(element_to_copy, output_control_result::skipped);
                         }

                         return !is_already_set;
                     });

        output_controls = std::move(optimized_outputs);
    }

    for (const auto &[output_id, output_value] : output_controls) {
        bool current_result = outputs::control_output(output_id, output_value);

        if (!current_result) {
            control_results.emplace_back(std::make_pair(output_id, output_value), output_control_result::failure);
        }
    }

    return batch_output_control_result{
        .batch_result = job_result, .output_controls = std::move(output_controls), .control_results = control_results};
}
