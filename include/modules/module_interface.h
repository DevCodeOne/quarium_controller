#pragma once

#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <optional>

#include <cstdint>

using module_value_types = std::variant<int32_t, std::string, float>;

template<typename T>
class module_value_description;

template<>
class module_value_description<int32_t> {};

template<>
class module_value_description<std::string> {};

template<>
class module_value_description<float> {};

using module_value_description_variant =
    std::variant<module_value_description<int32_t>, module_value_description<std::string>,
                 module_value_description<float>>;

class module_interface_description {
   public:
    module_interface_description(
        std::vector<std::pair<std::string, module_value_description_variant>> values_description);
    ~module_interface_description() = default;

    const std::vector<std::pair<std::string, module_value_description_variant>> &values() const;

   private:
    std::vector<std::pair<std::string, module_value_description_variant>> m_values;
};

// TODO define methods here to build a gui and handle all the stuff so that T only has to implement the module specific
// stuff
class module_interface {
   public:
    module_interface() = default;
    virtual ~module_interface() = default;

    virtual bool value(const std::string &id, const module_value_types &value) = 0;
    virtual bool update_values() = 0;

    virtual std::optional<module_value_types> value(const std::string &id) const = 0;
    virtual const module_interface_description &description() const = 0;

   private:
};
