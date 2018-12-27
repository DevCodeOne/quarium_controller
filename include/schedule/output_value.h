#pragma once

#include <variant>

enum struct switch_output { off = 0, on = 1, toggle = 2 };

// variant with all possible values, for now implicit convertible
class output_value {
   public:
    output_value(const switch_output &value);

    template<typename T>
    std::optional<T> get() const;

   private:
    std::variant<switch_output> m_value;
};

