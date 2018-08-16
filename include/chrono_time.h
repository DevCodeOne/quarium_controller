#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

extern long timezone;

using days = std::chrono::duration<int32_t, std::ratio<24 * std::chrono::hours::duration::period::num>>;

enum struct weekday : uint8_t { monday, tuesday, wednesday, thursday, friday, saturday, sunday };

weekday get_weekday(const days &days_since_epoch);
std::string to_string(const weekday &day);

std::time_t convert_to_time_t_localtime(const std::tm *date);

// TODO update when C++20 comes around
template<typename T>
T duration_since_epoch() {
    std::time_t current_time_utc = std::time(nullptr);
    std::tm *current_date = localtime(&current_time_utc);

    return std::chrono::duration_cast<T>(std::chrono::seconds(convert_to_time_t_localtime(current_date)));
}

template<typename T>
std::optional<T> convert_date_to_duration_since_epoch(const std::string &date, const std::string &date_format) {
    std::istringstream date_stream{date};

    std::tm date_tm;
    std::memset(&date_tm, 0, sizeof(std::tm));
    date_stream >> std::get_time(&date_tm, date_format.c_str());

    if (date_stream.fail()) {
        return {};
    }

    return std::chrono::duration_cast<T>(std::chrono::seconds(convert_to_time_t_localtime(&date_tm)));
}
