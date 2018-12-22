#include "modules/module_interface.h"
#include "modules/remote_function.h"

#include "logger.h"

// TODO add register function to register a module type
std::shared_ptr<module_interface> create_module(const nlohmann::json &description) {
    if (description.is_null() || !description.is_object()) {
        return nullptr;
    }

    auto module_type_entry = description["type"];

    if (module_type_entry.is_null() || !module_type_entry.is_string()) {
        return nullptr;
    }

    auto module_type = module_type_entry.get<std::string>();

    if (module_type == "remote_function") {
        return remote_function::deserialize(description);
    }

    return nullptr;
}

std::optional<module_value_type> map_type_string_to_enum(const std::string &type) {
    if (type == "int") {
        return module_value_type::integer;
    } else if (type == "float") {
        return module_value_type::floating;
    } else if (type == "string") {
        return module_value_type::string;
    }

    return {};
}

std::optional<module_value> module_value::deserialize(const nlohmann::json &description) {
    if (description.is_null() || !description.is_object()) {
        return {};
    }

    nlohmann::json name_entry = description["name"];
    nlohmann::json type_entry = description["type"];
    nlohmann::json range_entry = description["range"];

    if (name_entry.is_null() || !name_entry.is_string()) {
        logger::instance()->critical("Values description is invalid the name is not a valid string");
        return {};
    }

    if (type_entry.is_null() || !type_entry.is_string()) {
        logger::instance()->critical("Values description is invalid the provided type is not a valid string");
        return {};
    }

    try {
        auto name = name_entry.get<std::string>();
        auto type = type_entry.get<std::string>();

        auto value_type = map_type_string_to_enum(type);

        if (!value_type.has_value()) {
            logger::instance()->critical("Value type is not valid, valid values are : int, float, string");
            return {};
        }

        // TODO refactor the following
        if (!range_entry.is_null()) {
            if (!range_entry.is_array() || range_entry.size() != 2) {
                logger::instance()->critical("Range entry is not valid");
                return {};
            }

            nlohmann::json value1_entry = range_entry[0];
            nlohmann::json value2_entry = range_entry[1];

            auto sort_values = [](auto &left, auto &right) {
                if (left > right) {
                    std::swap(left, right);
                }
            };

            if (value1_entry.type() == value2_entry.type()) {
                if (value_type == module_value_type::integer &&
                    (value1_entry.type() == nlohmann::json::value_t::number_integer ||
                     value1_entry.type() == nlohmann::json::value_t::number_unsigned)) {
                    int left = value1_entry.get<int>();
                    int right = value2_entry.get<int>();

                    sort_values(left, right);

                    return module_value(name, left, right);
                } else if (value_type == module_value_type::floating &&
                           value1_entry.type() == nlohmann::json::value_t::number_float) {
                    float left = value1_entry.get<float>();
                    float right = value2_entry.get<float>();
                    sort_values(left, right);

                    return module_value(name, left, right);
                }
            } else {
                // TODO: There's no sense for a string to have a range look into more sensible approaches, maybe a list
                // of possible strings
                return {};
            }
        }

        switch (value_type.value()) {
            case module_value_type::integer:
                return module_value(name, std::numeric_limits<int>::lowest(), std::numeric_limits<int>::max());
            case module_value_type::floating:
                return module_value(name, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
            default:
                return {};
        }
    } catch (std::exception e) {
        logger::instance()->critical("An exception occured when trying to parse the values of a module {}", e.what());
        return {};
    }

    return {};
}

module_value_type module_value::what_type() const {
    if (std::holds_alternative<int>(m_value)) {
        return module_value_type::integer;
    } else if (std::holds_alternative<float>(m_value)) {
        return module_value_type::floating;
    }

    return module_value_type::string;
}

const std::string &module_value::name() const { return m_name; }

std::optional<module_interface_description> module_interface_description::deserialize(
    const nlohmann::json &description) {
    if (!description.is_object()) {
        logger::instance()->critical("The description of a module is not a valid description");
        return {};
    }

    if (description["values"].is_null() || !description["values"].is_array()) {
        logger::instance()->critical("No values section in the description of the module");
        return {};
    }

    std::map<std::string, module_value> values{};
    std::map<std::string, nlohmann::json> other_values{};

    for (auto &current_item : description["values"].items()) {
        auto &current_value_description_entry = current_item.value();

        if (!current_value_description_entry.is_object()) {
            logger::instance()->critical("A value entry in the values section of a module is invalid");
            continue;
        }

        auto value_description = module_value::deserialize(current_value_description_entry);

        if (value_description.has_value()) {
            values.emplace(std::make_pair(value_description.value().name(), value_description.value()));
        }
    }

    for (auto &current_item : description.items()) {
        if (current_item.key() == "values") {
            continue;
        }

        other_values.emplace(std::make_pair(current_item.key(), current_item.value()));
    }

    if (values.size() == 0) {
        return {};
    }

    return module_interface_description(values, other_values);
}

module_interface_description::module_interface_description(const std::map<std::string, module_value> &values,
                                                           const std::map<std::string, nlohmann::json> &other_values)
    : m_values(values), m_other_values(other_values) {}

const std::map<std::string, module_value> &module_interface_description::values() const { return m_values; }

const std::map<std::string, nlohmann::json> &module_interface_description::other_values() const {
    return m_other_values;
}

std::optional<module_value> module_interface_description::lookup_value(const std::string &id) const {
    if (auto result = m_values.find("id"); result != m_values.cend()) {
        return result->second;
    }

    return {};
}

std::optional<nlohmann::json> module_interface_description::lookup_other_value(const std::string &id) const {
    if (auto result = m_other_values.find("id"); result != m_other_values.cend()) {
        return std::optional<nlohmann::json>(result->second);
    }

    return {};
}

module_interface::module_interface(const std::string &id, const module_interface_description &description)
    : m_id(id), m_description(description) {}

const module_interface_description &module_interface::description() const { return m_description; }

const std::string &module_interface::id() const { return m_id; }

