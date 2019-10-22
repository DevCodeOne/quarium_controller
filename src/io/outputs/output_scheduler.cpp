#include "io/outputs/output_scheduler.h"

output_control_job &output_control_job::add_output_control(std::pair<output_id, output_value> output_control) {
    std::unique_lock<std::shared_mutex> _{m_instance_mutex};

    m_output_controls.emplace_back(std::move(output_control));
    return *this;
}

output_control_job &output_control_job::add_output_controls(
    const std::vector<std::pair<output_id, output_value>> &output_control) {
    std::unique_lock<std::shared_mutex> instance_guard{m_instance_mutex};

    m_output_controls.reserve(m_output_controls.size() + output_control.size() + 1);
    std::copy(output_control.cbegin(), output_control.cend(), std::back_inserter(m_output_controls));
    return *this;
}

size_t output_control_job::number_of_occured_errors() const {
    std::shared_lock<std::shared_mutex> instance_guard{m_instance_mutex};
    return m_failures.size();
}

std::vector<output_id> output_control_job::occured_errors() const {
    std::shared_lock<std::shared_mutex> instance_guard{m_instance_mutex};
    return m_failures;
}

std::vector<std::pair<output_id, output_value>> output_control_job::output_controls() const {
    std::shared_lock<std::shared_mutex> instance_guard{m_instance_mutex};
    return m_output_controls;
}
