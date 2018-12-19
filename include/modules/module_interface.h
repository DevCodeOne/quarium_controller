#pragma once

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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

    template<typename T>
    bool value(const T &new_value);

    template<typename T>
    T value() const;
    template<typename T>
    T min() const;
    template<typename T>
    T max() const;

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
    return std::visit([](const auto &current_value) -> T { return static_cast<T>(current_value); }, m_value);
}

template<typename T>
T module_value::min() const {
    return std::visit([](const auto &current_value) -> T { return static_cast<T>(current_value); }, m_min);
}

template<typename T>
T module_value::max() const {
    return std::visit([](const auto &current_value) -> T { return static_cast<T>(current_value); }, m_max);
}

class module_interface_description {
   public:
    static std::optional<module_interface_description> deserialize(const nlohmann::json &description);
    ~module_interface_description() = default;

    const std::vector<std::pair<std::string, module_value>> &values() const;
    const std::vector<std::pair<std::string, nlohmann::json>> &other_values() const;

   private:
    module_interface_description(const std::vector<std::pair<std::string, module_value>> &values,
                                 const std::vector<std::pair<std::string, nlohmann::json>> &other_values = {});

    std::vector<std::pair<std::string, module_value>> m_values;
    std::vector<std::pair<std::string, nlohmann::json>> m_other_values;
};

// TODO define methods here to build a gui and handle all the stuff so that T only has to implement the module specific
// stuff
class module_interface {
   public:
    module_interface(const std::string &id, const module_interface_description &description);
    virtual ~module_interface() = default;

    virtual bool value(const std::string &id, const module_value_types &value);
    virtual bool update_values() = 0;

    virtual std::optional<module_value_types> value(const std::string &id) const;
    virtual const module_interface_description &description() const;
    virtual const std::string &id() const;

   private:
    std::string m_id;
    module_interface_description m_description;
    std::map<std::string, module_value_types> m_values;
};

std::shared_ptr<module_interface> create_module(const nlohmann::json &description);
