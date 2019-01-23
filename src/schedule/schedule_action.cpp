#include <map>

#include "logger.h"
#include "schedule/schedule_action.h"

bool schedule_action::add_action(json &schedule_action_description) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    json id_entry = schedule_action_description["id"];
    json output_entry = schedule_action_description["outputs"];
    json output_actions_entry = schedule_action_description["output_actions"];

    if (id_entry.is_null() || output_entry.is_null() || output_actions_entry.is_null()) {
        logger::instance()->critical("A needed entry in an action was missing : {} {} {}",
                                     id_entry.is_null() ? "id" : "", output_entry.is_null() ? "outputs" : "",
                                     output_actions_entry.is_null() ? "output_actions" : "");
        if (!id_entry.is_null() && id_entry.is_string()) {
            logger::instance()->critical("The issue was in the action with the id {}", id_entry.get<std::string>());
        }
        return false;
    }

    schedule_action created_action;

    if (!id_entry.is_string()) {
        logger::instance()->critical("The id for an action is not a string");
    }

    const std::string id = id_entry.get<std::string>();

    if (is_valid_id(id)) {
        logger::instance()->critical("The id for an action {} is already in use", id);
        return false;
    }

    if (!output_entry.is_array()) {
        logger::instance()->critical("The entry outputs in action {} isn't an array", id);
        return false;
    }

    std::vector<output_id> created_outputs;
    bool created_all_outputs_successfully =
        std::all_of(output_entry.begin(), output_entry.end(), [&created_outputs](auto &current_output_entry) {
            if (!current_output_entry.is_string()) {
                return false;
            }

            std::string id = current_output_entry.template get<std::string>();
            if (!outputs::is_valid_id(id)) {
                logger::instance()->critical("An output with the id {} doesn't exist", id);
                return false;
            }

            created_outputs.emplace_back(output_id(id));
            return true;
        });

    if (!created_all_outputs_successfully) {
        logger::instance()->critical("The entry outputs in action {} isn't valid", id);
        return false;
    }

    if (!output_actions_entry.is_array()) {
        logger::instance()->critical("The entry output_actions in action {} isn't an array", id);
        return false;
    }

    std::vector<output_value> created_output_actions;
    bool created_all_output_actions_successfully =
        std::all_of(output_actions_entry.begin(), output_actions_entry.end(),
                    [&created_outputs, &created_output_actions](auto &current_output_entry) {
                        std::optional<output_value> value;
                        if (created_outputs.size() < created_output_actions.size()) {
                            return false;
                        }

                        auto current_value = outputs::current_state(created_outputs[created_output_actions.size()]);
                        if (current_value.has_value()) {
                            value = output_value::deserialize(current_output_entry, current_value->current_type());
                        } else {
                            value = output_value::deserialize(current_output_entry);
                        }

                        if (!value.has_value()) {
                            return false;
                        }

                        created_output_actions.emplace_back(*value);
                        return true;
                    });

    if (!created_all_output_actions_successfully) {
        logger::instance()->critical("The entry output_actions in action {} isn't valid", id);
    }

    if (created_outputs.size() != created_output_actions.size()) {
        logger::instance()->critical(
            "There are not the same number of entries in the output specific arrays in action {}", id);
        return false;
    }

    created_action.id(id);
    for (unsigned int i = 0; i < created_outputs.size(); ++i) {
        created_action.add_pin(std::make_pair(created_outputs[i], created_output_actions[i]));
    }

    _actions.emplace_back(std::make_unique<schedule_action>(std::move(created_action)));

    return true;
}

bool schedule_action::is_valid_id(const schedule_action_id &id) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};
    return std::find_if(_actions.cbegin(), _actions.cend(),
                        [&id](const auto &current_action) { return current_action->id() == id; }) != _actions.cend();
}

bool schedule_action::execute_action(const schedule_action_id &id) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};
    if (!is_valid_id(id)) {
        return false;
    }

    auto action = std::find_if(_actions.begin(), _actions.end(),
                               [&id](const auto &current_action) { return current_action->id() == id; });

    return (**action)();
}

bool schedule_action::execute_actions(const std::vector<schedule_action_id> &ids) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    std::map<output_id, output_value> actions;
    bool result = true;

    for (auto current_id = ids.rbegin(); current_id != ids.rend(); ++current_id) {
        if (!is_valid_id(*current_id)) {
            logger::instance()->warn("{} is not a valid id for an action", *current_id);
            return false;
        }

        auto action = std::find_if(_actions.begin(), _actions.end(),
                                   [&current_id](const auto &action) { return action->id() == *current_id; });

        if ((*action) == nullptr) {
            continue;
        }

        // Do this manually to prevent setting the same pins multiple times
        for (auto &[current_pin, current_action] : (*action)->m_outputs) {
            auto result = actions.emplace(current_pin, current_action);

            if (!result.second) {
                logger::instance()->info("Skip setting {}", current_pin);
            }
        }
    }

    for (auto &[current_pin_id, current_action] : actions) {
        result &= outputs::control_output(current_pin_id, current_action);
    }

    return result;
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

schedule_action &schedule_action::add_pin(const std::pair<output_id, output_value> &new_pin) {
    m_outputs.emplace_back(new_pin);
    return *this;
}

bool schedule_action::operator()() {
    bool result = true;

    for (auto [current_pin, current_action] : m_outputs) {
        result &= outputs::control_output(current_pin, current_action);
    }
    return result;
}

const schedule_action_id &schedule_action::id() const { return m_id; }

const std::vector<std::pair<output_id, output_value>> &schedule_action::pins() const { return m_outputs; }
