#pragma once

#include <chrono>
#include <cstdint>
#include <string>

using days = std::chrono::duration<int32_t, std::ratio<24 * std::chrono::hours::duration::period::num>>;

enum struct weekday : uint8_t { monday, tuesday, wednesday, thursday, friday, saturday, sunday };

weekday get_weekday(const days &days_since_epoch);
std::string to_string(const weekday &day);
