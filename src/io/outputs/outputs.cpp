#include "io/outputs/outputs.h"

#include "logger.h"

bool outputs::add_output(nlohmann::json &gpio_description) {
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

auto outputs::find_output(const output_id &id) -> outputs_map_type::iterator {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};
    return std::find_if(_outputs.begin(), _outputs.end(),
                        [&id](const auto &current_output) { return current_output.first == id; });
}

bool outputs::is_valid_id(const output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};
    return find_output(id) != _outputs.cend();
}

bool outputs::control_output(const output_id &id, const output_value &value) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = find_output(id);

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return false;
    }

    return ((*output).second)->control_output(value);
}

std::optional<output_value> outputs::is_overriden(const output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = find_output(id);

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return {};
    }

    return ((*output).second)->is_overriden();
}

bool outputs::override_with(const output_id &id, const output_value &value) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = find_output(id);

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return false;
    }

    return ((*output).second)->override_with(value);
}

bool outputs::restore_control(const output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = find_output(id);

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return false;
    }

    return ((*output).second)->restore_control();
}

std::optional<output_value> outputs::current_state(const output_id &id) {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    auto output = find_output(id);

    if (output == _outputs.cend() || (*output).second == nullptr) {
        return {};
    }

    return ((*output).second)->current_state();
}

std::vector<output_id> outputs::get_ids() {
    std::lock_guard<std::recursive_mutex> list_guard{_list_mutex};

    std::vector<output_id> ids;
    ids.reserve(_outputs.size());

    for (auto &[id, iterator] : _outputs) {
        ids.emplace_back(id);
    }

    return ids;
}
