#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <sstream>
#include <variant>

#include "nlohmann/json.hpp"

enum struct switch_output { off = 0, on = 1, toggle = 2 };
std::ostream &operator<<(std::ostream &os, const switch_output &output);

enum struct tasmota_power_command { off = 0, on = 1, toggle = 2 };
std::ostream &operator<<(std::ostream &os, const tasmota_power_command &power_command);

enum struct output_value_types {
    number,
    number_unsigned,
    switch_output,
    value_collection,
    tasmota_power_command,
    string
};

namespace Detail {
    template<class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };
    template<class... Ts>
    overloaded(Ts...)->overloaded<Ts...>;
}  // namespace Detail

class output_value {
   public:
    using variant_type = std::variant<switch_output, tasmota_power_command, int, unsigned int, std::string>;

    static std::optional<output_value> deserialize(
        const nlohmann::json &description,
        std::optional<output_value_types> type = std::optional<output_value_types>{});

    template<typename T>
    std::optional<T> serialize() const;

    template<typename T, std::enable_if_t<!std::is_same_v<T, output_value>, int> = 0>
    output_value(T value);
    template<typename T, std::enable_if_t<!std::is_same_v<T, output_value>, int> = 0>
    output_value(T value, std::optional<T> min, std::optional<T> max);
    output_value(const output_value &other) = default;
    output_value(output_value &&other) = default;
    ~output_value() = default;

    output_value &operator=(const output_value &other) = default;
    output_value &operator=(output_value &&other) = default;

    template<typename T>
    std::optional<T> get() const;
    template<typename T>
    std::optional<T> min() const;
    template<typename T>
    std::optional<T> max() const;

    template<typename T>
    bool holds_type() const;

    output_value_types current_type() const;

    template<typename T>
    bool set(const T &value);

   private:
    variant_type m_value;
    // TODO: use variant_type plus std::monostate instead of an optional value for min and max value
    std::optional<variant_type> m_min;
    std::optional<variant_type> m_max;

    friend bool operator==(const output_value &lhs, const output_value &rhs);
};

template<>
inline std::optional<std::string> output_value::serialize() const {
    auto serialize_value = [](auto current_value) -> std::string {
        std::ostringstream os("");
        os << current_value;
        return os.str();
    };

    return std::visit(serialize_value, m_value);
}

template<typename T>
std::optional<T> output_value::get() const {
    if (std::holds_alternative<T>(m_value)) {
        return std::get<T>(m_value);
    }

    return {};
}

template<typename T>
std::optional<T> output_value::min() const {
    if (m_min.has_value() && std::holds_alternative<T>(*m_min)) {
        return std::get<T>(*m_min);
    }

    return {};
}

template<typename T>
std::optional<T> output_value::max() const {
    if (m_max.has_value() && std::holds_alternative<T>(*m_max)) {
        return std::get<T>(*m_max);
    }

    return {};
}

template<typename T>
bool output_value::set(const T &value) {
    if (!holds_type<T>()) {
        return false;
    }

    if (m_min.has_value()) {
        auto result = std::visit(
            Detail::overloaded{[&value](auto &min_value) -> bool { return value >= static_cast<T>(min_value); },
                               [&value](std::string &) -> bool { return false; }},
            *m_min);

        if (!result) {
            return false;
        }
    }

    if (m_max.has_value()) {
        auto result = std::visit(
            Detail::overloaded{[&value](auto &max_value) -> bool { return value <= static_cast<T>(max_value); },
                               [&value](std::string &) -> bool { return false; }},
            *m_max);

        if (!result) {
            return false;
        }
    }

    m_value = value;
    return true;
}

template<typename T, std::enable_if_t<!std::is_same_v<T, output_value>, int>>
output_value::output_value(T value) : m_value(value) {}

template<typename T, std::enable_if_t<!std::is_same_v<T, output_value>, int>>
output_value::output_value(T value, std::optional<T> min, std::optional<T> max) : m_value(value) {
    if (min) {
        m_min.emplace(variant_type{*min});
    }

    if (max) {
        m_max.emplace(variant_type{*max});
    }
}

template<typename T>
bool output_value::holds_type() const {
    return std::holds_alternative<T>(m_value);
}

bool operator==(const output_value &lhs, const output_value &rhs);
bool operator!=(const output_value &lhs, const output_value &rhs);
