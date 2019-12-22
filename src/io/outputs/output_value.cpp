#include "io/outputs/output_value.h"

#include <algorithm>
#include <map>

#include "logger.h"

std::ostream &operator<<(std::ostream &os, const switch_output &output) {
    switch (output) {
        case switch_output::on:
            return os << "on";
            break;
        case switch_output::off:
            return os << "off";
            break;
        case switch_output::toggle:
            return os << "toggle";
            break;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const tasmota_power_command &power_command) {
    switch (power_command) {
        case tasmota_power_command::on:
            return os << R"(Power%20On)";
            break;
        case tasmota_power_command::off:
            return os << R"(Power%20Off)";
            break;
        case tasmota_power_command::toggle:
            return os << R"(Power%20TOGGLE)";
            break;
    }
    return os;
}

bool operator==(const output_value &lhs, const output_value &rhs) {
    if (lhs.current_type() != rhs.current_type()) {
        return false;
    }

    return std::visit(
        [&rhs](const auto &current_value) {
            return current_value == *rhs.get<std::decay_t<decltype(current_value)>>();
        },
        lhs.m_value);
}

bool operator!=(const output_value &lhs, const output_value &rhs) { return !(lhs == rhs); }

std::optional<output_value> output_value::deserialize(const nlohmann::json &description_parameter,
                                                      std::optional<output_value_types> type) {
    auto logger_instance = logger::instance();
    nlohmann::json description = description_parameter;
    if (description.is_null()) {
        logger_instance->info("The description of one output_value was invalid");
        return {};
    }

    if (description.is_object()) {
        // Parse as an object
        // TODO implement
        auto type_entry = description["type"];
        auto default_entry = description["default"];
        auto range_entry = description["range"];

        if (type_entry.is_null() || !type_entry.is_string()) {
            logger_instance->info("The description of one output_value was invalid : the type entry was invalid");
            return {};
        }

        output_value_types type = output_value_types::switch_output;
        std::string type_as_string = type_entry.get<std::string>();

        std::optional<output_value::variant_type> default_value;
        std::optional<output_value::variant_type> range;

        if (type_as_string == "int") {
            type = output_value_types::number;
        } else if (type_as_string == "unsigned int") {
            type = output_value_types::number_unsigned;
        } else if (type_as_string == "switch_output") {
            type = output_value_types::switch_output;
        } else if (type_as_string == "tasmota_power_command") {
            type = output_value_types::tasmota_power_command;
        } else if (type_as_string == "string") {
            type = output_value_types::string;
        } else {
            logger_instance->critical("Invalid value type in value description");
            return {};
        }

        if (!default_entry.is_null()) {
            if (default_entry.is_number_unsigned()) {
                default_value = default_entry.get<unsigned int>();
            } else if (default_entry.is_number_integer()) {
                default_value = default_entry.get<int>();
            } else if (default_entry.is_string()) {
                const std::map<std::string, switch_output> switch_output_values = {
                    {"on", switch_output::on}, {"off", switch_output::off}, {"toggle", switch_output::toggle}};

                if (auto result = switch_output_values.find(default_entry.get<std::string>());
                    result != switch_output_values.cend()) {
                    default_value = result->second;
                }
            } else {
                logger_instance->info(
                    "The description of one output_value was invalid : the default entry in the description is "
                    "invalid");
                return {};
            }
        }

        if (!default_value.has_value()) {
            if (type == output_value_types::number) {
                default_value = 0;
            } else if (type == output_value_types::number_unsigned) {
                default_value = 0u;
            } else if (type == output_value_types::switch_output) {
                default_value = switch_output::off;
            } else if (type == output_value_types::tasmota_power_command) {
                default_value = tasmota_power_command::off;
            } else if (type == output_value_types::string) {
                default_value = "";
            } else {
                return {};
            }
        }

        if (!default_value.has_value()) {
            return {};
        }

        if (range_entry.is_null()) {
            if (type == output_value_types::number || type == output_value_types::number_unsigned) {
                logger_instance->warn(
                    "The description of the value doesn't specify a valid range this is probably not what you want");
            }
            return output_value(*default_value);
        }

        if (!range_entry.is_array() || range_entry.size() != 2) {
            logger_instance->info(
                "The description of one output_value was invalid : the range entry in the description is "
                "invalid");
            return {};
        }

        auto first_entry = range_entry[0];
        auto second_entry = range_entry[1];

        if (first_entry.is_number() != second_entry.is_number()) {
            return {};
        }

        std::optional<output_value::variant_type> lower_range_entry;
        std::optional<output_value::variant_type> higher_range_entry;
        if (type == output_value_types::number && first_entry.is_number()) {
            auto left = std::min(first_entry.get<int>(), second_entry.get<int>());
            auto right = std::max(first_entry.get<int>(), second_entry.get<int>());

            lower_range_entry = left;
            higher_range_entry = right;
        } else if (type == output_value_types::number_unsigned && first_entry.is_number()) {
            auto left = std::min(first_entry.get<unsigned int>(), second_entry.get<unsigned int>());
            auto right = std::max(first_entry.get<unsigned int>(), second_entry.get<unsigned int>());

            lower_range_entry = left;
            lower_range_entry = left;
        }

        return output_value(*default_value, lower_range_entry, higher_range_entry);
    } else {
        // Is a singular value
        // TODO: parse this with the individual types as helpers
        if (description.is_string()) {
            auto value = description.get<std::string>();

            if (type.has_value() && *type == output_value_types::switch_output) {
                const std::map<std::string, switch_output> switch_output_values = {
                    {"on", switch_output::on}, {"off", switch_output::off}, {"toggle", switch_output::toggle}};

                if (auto result = switch_output_values.find(value); result != switch_output_values.cend()) {
                    return output_value{result->second};
                }
            } else if (type.has_value() && *type == output_value_types::tasmota_power_command) {
                const std::map<std::string, tasmota_power_command> tasmota_power_command_output_values = {
                    {"on", tasmota_power_command::on},
                    {"off", tasmota_power_command::off},
                    {"toggle", tasmota_power_command::toggle}};

                if (auto result = tasmota_power_command_output_values.find(value);
                    result != tasmota_power_command_output_values.cend()) {
                    return output_value{result->second};
                }
            } else if (type.has_value() && *type == output_value_types::string) {
                return output_value{description.dump()};
            }
        } else if (description.is_number_unsigned() &&
                   ((!type.has_value()) || (type.has_value() && *type == output_value_types::number_unsigned))) {
            return output_value(description.get<unsigned int>());
        } else if (description.is_number() &&
                   ((!type.has_value()) || (type.has_value() && *type == output_value_types::number))) {
            return output_value(description.get<int>());
        }

        // Fall back to plain string
        return output_value(description.dump());
    }

    return {};
}

output_value_types output_value::current_type() const {
    if (holds_type<switch_output>()) {
        return output_value_types::switch_output;
    } else if (holds_type<tasmota_power_command>()) {
        return output_value_types::tasmota_power_command;
    } else if (holds_type<int>()) {
        return output_value_types::number;
    } else if (holds_type<unsigned int>()) {
        return output_value_types::number_unsigned;
    } else if (holds_type<std::string>()) {
        return output_value_types::string;
    }
    // Should never actually happen
    return output_value_types::number;
}
