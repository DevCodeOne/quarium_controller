#include "schedule/schedule_action.h"

#include <future>

#include "io/outputs/output_scheduler.h"
#include "logger.h"

bool schedule_action::add_action(json &schedule_action_description) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};
    auto logger_instance = logger::instance();

    json id_entry = schedule_action_description["id"];
    json output_entry = schedule_action_description["outputs"];
    json output_actions_entry = schedule_action_description["output_actions"];

    if (id_entry.is_null() || output_entry.is_null() || output_actions_entry.is_null()) {
        logger_instance->critical("A needed entry in an action was missing : {} {} {}", id_entry.is_null() ? "id" : "",
                                  output_entry.is_null() ? "outputs" : "",
                                  output_actions_entry.is_null() ? "output_actions" : "");
        if (!id_entry.is_null() && id_entry.is_string()) {
            logger_instance->critical("The issue was in the action with the id {}", id_entry.get<std::string>());
        }
        return false;
    }

    schedule_action created_action;

    if (!id_entry.is_string()) {
        logger_instance->critical("The id for an action is not a string");
    }

    const std::string id = id_entry.get<std::string>();

    if (is_valid_id(id)) {
        logger_instance->critical("The id for an action {} is already in use", id);
        return false;
    }

    if (!output_entry.is_array()) {
        logger_instance->critical("The entry outputs in action {} isn't an array", id);
        return false;
    }

    std::vector<std::pair<output_id, output_value>> created_outputs;
    bool created_all_outputs_successfully = std::all_of(
        output_entry.begin(), output_entry.end(), [&created_outputs, &logger_instance](auto &current_output_entry) {
            if (!current_output_entry.is_string()) {
                return false;
            }

            std::string id = current_output_entry.template get<std::string>();
            if (!outputs::is_valid_id(id)) {
                logger_instance->critical("An output with the id {} doesn't exist", id);
                return false;
            }

            created_outputs.emplace_back(output_id(id), output_value(0));
            return true;
        });

    if (!created_all_outputs_successfully) {
        logger_instance->critical("The entry outputs in action {} isn't valid", id);
        return false;
    }

    if (!output_actions_entry.is_array()) {
        logger_instance->critical("The entry output_actions in action {} isn't an array", id);
        return false;
    }

    if (output_actions_entry.size() != output_entry.size()) {
        logger_instance->critical(
            "The entry output_actions doesn't have the same number of elements as outputs in action {}", id,
            output_entry.size(), output_actions_entry.size());
        return false;
    }

    bool created_all_output_actions_successfully = true;

    for (size_t i = 0; i < output_actions_entry.size(); ++i) {
        std::optional<output_value> value;

        auto current_value = outputs::current_state(created_outputs[i].first);
        if (current_value.has_value()) {
            value = output_value::deserialize(output_actions_entry[i], current_value->current_type());
        } else {
            logger_instance->warn(
                "Don't have valid type information from the default value, trying to figure out the type");
            value = output_value::deserialize(output_actions_entry[i]);
        }

        if (!value.has_value()) {
            created_all_output_actions_successfully = false;
            break;
        }

        created_outputs[i].second = *value;
    }

    if (!created_all_output_actions_successfully) {
        logger_instance->critical("The entry output_actions in action {} isn't valid", id);
    }

    created_action.id(id);
    for (const auto &current_output_value_pair : created_outputs) {
        created_action.attach_output(current_output_value_pair);
    }

    _actions.emplace_back(std::make_unique<schedule_action>(std::move(created_action)));

    return true;
}

bool schedule_action::is_valid_id(const schedule_action_id &id) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};
    return std::find_if(_actions.cbegin(), _actions.cend(),
                        [&id](const auto &current_action) { return current_action->id() == id; }) != _actions.cend();
}

// Actions will be executed in order, actions that will cancel each other will be optimized -> first action turns on
// second action turns off, then only the second one will be executed
std::vector<schedule_action_id> schedule_action::execute_actions(const std::vector<schedule_action_id> &ids) {
    struct schedule_action_execution_order {
        const std::vector<schedule_action_id>::const_reverse_iterator m_action_id;
        const output_value m_value;
    };

    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    // Mark all actions as failed and remove if the action was successfull
    std::vector<schedule_action_id> failed_actions;
    failed_actions.reserve(ids.size());
    std::copy(ids.cbegin(), ids.cend(), std::back_inserter(failed_actions));

    if (ids.size() == 0) {
        return failed_actions;
    }

    auto logger_instance = logger::instance();

    batch_output_control control_job;
    control_job.optimize_outputs(true);

    for (const auto &current_id : ids) {
        if (!is_valid_id(current_id)) {
            logger_instance->warn("{} is not a valid id for an action", current_id);
            return failed_actions;
        }

        const auto action = std::find_if(_actions.cbegin(), _actions.cend(),
                                         [&current_id](const auto &action) { return action->id() == current_id; });

        if ((*action) == nullptr) {
            continue;
        }

        control_job.add_output_controls((*action)->m_outputs);
    }

    auto output_control_future = output_scheduler::execute_batch_output_control(control_job);

    // TODO: add real timeout, don't wait for the result an undefined amount of time
    while (!output_control_future.valid()) {
        output_control_future.wait_for(std::chrono::seconds(10));
    }

    auto output_control_results = output_control_future.get();

    for (const auto &[output_control, control_result] : output_control_results.control_results) {
        if (control_result == output_control_result::skipped) {
            logger_instance->info("Skipped setting output {}", output_control.first);
        } else if (control_result == output_control_result::failure) {
            logger_instance->info("Failed setting output {}", output_control.first);
        }
    }

    // TODO: remove ids from failed_actions, when no output could be found, which failed
    auto new_failure_end =
        std::remove_if(failed_actions.begin(), failed_actions.end(), [&output_control_results](const auto &current_id) {
            auto current_action =
                std::find_if(_actions.cbegin(), _actions.cend(),
                             [&current_id](const auto &current_action) { return current_action->id() == current_id; });

            // Is not part of the valid actions, shouldn't happen
            if (current_action == _actions.cend()) {
                return false;
            }

            const auto &action_outputs = (*current_action)->m_outputs;

            // Check if every output of the action was successfully controlled or skipped
            return std::all_of(action_outputs.cbegin(), action_outputs.cend(),
                               [&output_control_results](const auto &current_controlled_output) {
                                   return std::all_of(
                                       output_control_results.control_results.cbegin(),
                                       output_control_results.control_results.cend(),
                                       [&current_controlled_output](const auto &current_result) {
                                           return current_controlled_output.first != current_result.first.first ||
                                                  current_result.second != output_control_result::failure;
                                       });
                               });
        });

    if (new_failure_end != failed_actions.cend()) {
        failed_actions.erase(new_failure_end, failed_actions.end());
    }

    return failed_actions;
}

schedule_action::schedule_action(schedule_action &&other)
    : m_id(std::move(other.m_id)), m_outputs(std::move(other.m_outputs)) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    if (is_valid_id(m_id)) {
        std::string id = m_id;
        auto action = std::find_if(_actions.begin(), _actions.end(),
                                   [&id](const auto &current_action) { return current_action->id() == id; });

        *action = std::unique_ptr<schedule_action>(this);
    }
}

schedule_action &schedule_action::id(const schedule_action_id &new_id) {
    m_id = new_id;
    return *this;
}

schedule_action &schedule_action::attach_output(const std::pair<output_id, output_value> &new_output) {
    m_outputs.emplace_back(new_output);
    return *this;
}

const schedule_action_id &schedule_action::id() const { return m_id; }

const std::vector<std::pair<output_id, output_value>> &schedule_action::outputs() const { return m_outputs; }
