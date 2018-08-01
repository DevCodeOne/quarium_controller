#pragma once

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

using days = std::chrono::duration<int32_t, std::ratio<24 * std::chrono::hours::duration::period::num>>;

enum struct weekday : uint8_t { monday, tuesday, wednesday, thursday, friday, saturday, sunday };

weekday get_weekday(const days &days_since_epoch);
std::string to_string(const weekday &day);

template<typename T>
std::optional<T> convert_date_to_duration_since_epoch(const std::string &date, const std::string &date_format) {
    std::istringstream date_stream{date};

    std::tm date_tm;
    date_stream >> std::get_time(&date_tm, date_format.c_str());

    if (date_stream.fail()) {
        return {};
    }

    return std::chrono::duration_cast<T>(std::chrono::seconds{std::mktime(&date_tm)});
}
