#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include <cstdint>

#include "nlohmann/json.hpp"

using module_value_types = std::variant<int32_t, std::string, float>;

enum struct module_value_type { integer, floating, string };

std::optional<module_value_type> map_type_string_to_enum(const std::string &type);

// TODO handle string correctly
class module_value {
   public:
    static std::optional<module_value> deserialize(const nlohmann::json &description);

    template<typename T>
    module_value(const std::string &name, T min = std::numeric_limits<T>::lowest(),
                 T max = std::numeric_limits<T>::max(), T value = {});

    // TODO fix the following methods they will return wrong values if the type is incorrect
    template<typename T>
    bool value(const T &new_value);

    template<typename T>
    T value() const;
    template<typename T>
    T min() const;
    template<typename T>
    T max() const;

    module_value_type what_type() const;

    const std::string &name() const;

   private:
    module_value_types m_value = {};
    const module_value_types m_min = {};
    const module_value_types m_max = {};
    const std::string m_name;
};

template<typename T>
module_value::module_value(const std::string &name, T min, T max, T value)
    : m_name(name), m_value(value), m_min(min), m_max(max) {}

// TODO add type check for strings
template<typename T>
bool module_value::value(const T &new_value) {
    if (new_value < m_min || new_value > m_max) {
        return false;
    }

    m_value = new_value;
    return true;
}

template<typename T>
T module_value::value() const {
    if (std::holds_alternative<T>(m_value)) {
        return std::get<T>(m_value);
    }

    return {};
}

template<typename T>
T module_value::min() const {
    if (std::holds_alternative<T>(m_min)) {
        return std::get<T>(m_min);
    }

    return {};
}

template<typename T>
T module_value::max() const {
    if (std::holds_alternative<T>(m_max)) {
        return std::get<T>(m_max);
    }

    return {};
}

class module_interface_description {
   public:
    static std::optional<module_interface_description> deserialize(const nlohmann::json &description);
    ~module_interface_description() = default;

    const std::map<std::string, module_value> &values() const;
    const std::map<std::string, nlohmann::json> &other_values() const;

    std::optional<module_value> lookup_value(const std::string &id) const;
    std::optional<nlohmann::json> lookup_other_value(const std::string &id) const;

   private:
    module_interface_description(const std::map<std::string, module_value> &values,
                                 const std::map<std::string, nlohmann::json> &other_values = {});

    std::map<std::string, module_value> m_values;
    std::map<std::string, nlohmann::json> m_other_values;
};

// TODO define methods here to build a gui and handle all the stuff so that T only has to implement the module specific
// stuff
class module_interface {
   public:
    module_interface(const std::string &id, const module_interface_description &description);
    virtual ~module_interface() = default;

    virtual bool update_values() = 0;

    virtual const module_interface_description &description() const;
    virtual const std::string &id() const;

   private:
    std::string m_id;
    module_interface_description m_description;
};

std::shared_ptr<module_interface> create_module(const nlohmann::json &description);
