#pragma once

#include <cstdint>
#include <optional>
#include <variant>

#include "nlohmann/json.hpp"

enum struct switch_output { off = 0, on = 1, toggle = 2 };

enum struct output_value_types { number, number_unsigned, switch_output };

class output_value {
   public:
    using variant_type = std::variant<switch_output, int, unsigned int>;

    static std::optional<output_value> deserialize(const nlohmann::json &description);

    template<typename T>
    output_value(T value, std::optional<T> min = {}, std::optional<T> max = {});

    template<typename T>
    std::optional<T> get() const;
    template<typename T>
    std::optional<T> min() const;
    template<typename T>
    std::optional<T> max() const;

    template<typename T>
    bool holds_type() const;

    std::size_t type_index() const;

   private:
    variant_type m_value;
    std::optional<variant_type> m_min;
    std::optional<variant_type> m_max;
};

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
output_value::output_value(T value, std::optional<T> min, std::optional<T> max) : m_value(value) {
    if (min.has_value()) {
        m_min = *min;
    }

    if (max.has_value()) {
        m_max = *max;
    }
}

template<typename T>
bool output_value::holds_type() const {
    return std::holds_alternative<T>(m_value);
}
