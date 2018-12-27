#include "schedule/schedule_output.h"
#include "logger.h"

bool schedule_output::add_output(json &gpio_description) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    json id_entry = gpio_description["id"];
    json type_entry = gpio_description["type"];
    json description_entry = gpio_description["description"];

    if (id_entry.is_null() || type_entry.is_null() || description_entry.is_null()) {
        logger::instance()->critical("A needed entry in a output entry was missing {} {} {}",
                                     id_entry.is_null() ? "id" : "", type_entry.is_null() ? "type" : "",
                                     description_entry.is_null() ? "description" : "");
        if (!id_entry.is_null() && id_entry.is_string()) {
            logger::instance()->critical("The issue was in the gpio entry with the id {}", id_entry.get<std::string>());
        }
        return false;
    }

    if (!id_entry.is_string()) {
        logger::instance()->critical("The id for a gpio entry is not a string");
        return false;
    }

    std::string id = id_entry.get<std::string>();

    if (is_valid_id(id)) {
        logger::instance()->critical("The id {} for a gpio entry is already in use", id);
        return false;
    }

    if (!type_entry.is_string()) {
        logger::instance()->critical("The type entry of the output entry {} is not a string", id);
        return false;
    }

    if (!description_entry.is_object()) {
        logger::instance()->critical("The description entry for the output entry with the id {} is not an object", id);
        return false;
    }

    auto created_output = output_factory::deserialize(type_entry.get<std::string>(), description_entry);

    if (!created_output) {
        logger::instance()->critical("The output with the id {} couldn't be created", id);
        return false;
    }

    _outputs.emplace(std::make_pair(id, std::move(created_output)));
    return true;
}

bool schedule_output::is_valid_id(const schedule_output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};
    return std::find_if(_outputs.cbegin(), _outputs.cend(),
                        [&id](const auto &current_output) { return current_output.first == id; }) != _outputs.cend();
}

bool schedule_output::control_pin(const schedule_output_id &id, const output_value &value) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = std::find_if(_outputs.begin(), _outputs.end(),
                               [&id](const auto &current_output) { return current_output.first == id; });

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return false;
    }

    return ((*output).second)->control_output(value);
}

std::optional<output_value> schedule_output::is_overriden(const schedule_output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = std::find_if(_outputs.begin(), _outputs.end(),
                               [&id](const auto &current_output) { return current_output.first == id; });

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return {};
    }

    return ((*output).second)->is_overriden();
}

bool schedule_output::override_with(const schedule_output_id &id, const output_value &value) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = std::find_if(_outputs.begin(), _outputs.end(),
                               [&id](const auto &current_output) { return current_output.first == id; });

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return false;
    }

    return ((*output).second)->override_with(value);
}

bool schedule_output::restore_control(const schedule_output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = std::find_if(_outputs.begin(), _outputs.end(),
                               [&id](const auto &current_output) { return current_output.first == id; });

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return false;
    }

    return ((*output).second)->restore_control();
}

std::optional<output_value> schedule_output::current_state(const schedule_output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = std::find_if(_outputs.cbegin(), _outputs.cend(),
                               [&id](const auto &current_output) { return current_output.first == id; });

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return {};
    }

    return ((*output).second)->current_state();
}

std::vector<schedule_output_id> schedule_output::get_ids() {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    std::vector<schedule_output_id> ids;
    ids.reserve(_outputs.size());

    for (auto &[id, iterator] : _outputs) {
        ids.emplace_back(id);
    }

    return ids;
}
