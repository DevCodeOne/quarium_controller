#include <algorithm>
#include <map>

#include "io/output_value.h"
#include "logger.h"

std::optional<output_value> output_value::deserialize(const nlohmann::json &description_parameter) {
    nlohmann::json description = description_parameter;
    if (description.is_null()) {
        logger::instance()->info("The description of one output_value was invalid");
        return {};
    }

    if (description.is_object()) {
        // Parse as an object
        // TODO implement
        auto type_entry = description["type"];
        auto default_entry = description["default"];
        auto range_entry = description["range"];

        if (type_entry.is_null() || !type_entry.is_string()) {
            logger::instance()->info("The description of one output_value was invalid : the type entry was invalid");
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
                logger::instance()->info(
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
            } else {
                return {};
            }
        }

        if (!default_value.has_value()) {
            return {};
        }

        if (range_entry.is_null()) {
            logger::instance()->warn(
                "The description of the value doesn't specify a valid range this is probably not what you want");
            return output_value(*default_value);
        }

        if (!range_entry.is_array() || range_entry.size() != 2) {
            logger::instance()->info(
                "The description of one output_value was invalid : the range entry in the description is "
                "invalid");
            return {};
        }

        auto first_entry = range_entry[0];
        auto second_entry = range_entry[1];

        if (first_entry.is_number() != second_entry.is_number()) {
            return false;
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

            const std::map<std::string, switch_output> switch_output_values = {
                {"on", switch_output::on}, {"off", switch_output::off}, {"toggle", switch_output::toggle}};

            if (auto result = switch_output_values.find(value); result != switch_output_values.cend()) {
                return output_value{result->second};
            }
        }
    }

    return {};
}
