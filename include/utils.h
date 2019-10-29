#pragma once

#include <charconv>
#include <chrono>
#include <optional>
#include <string_view>

template<typename TargetPeriod>
std::optional<TargetPeriod> parse_duration(std::string_view str) {
    auto numbers_end = str.find_first_not_of("0123456789");

    if (numbers_end == 0 || numbers_end == std::string_view::npos) {
        return {};
    }

    auto number_str = str.substr(0, numbers_end);
    auto unit = str.substr(numbers_end);
    uint32_t numbers = 0;
    auto conversion_result = std::from_chars(str.cbegin(), str.cend(), numbers);

    if (conversion_result.ec != std::errc()) {
        return {};
    }

    std::optional<TargetPeriod> result{};

    if (unit.compare("ms") == 0) {
        result = std::chrono::duration_cast<TargetPeriod>(std::chrono::milliseconds(numbers));
    } else if (unit.compare("s") == 0) {
        result = std::chrono::duration_cast<TargetPeriod>(std::chrono::seconds(numbers));
    } else if (unit.compare("m") == 0) {
        result = std::chrono::duration_cast<TargetPeriod>(std::chrono::minutes(numbers));
    } else if (unit.compare("h") == 0) {
        result = std::chrono::duration_cast<TargetPeriod>(std::chrono::hours(numbers));
    }

    return result;
}
