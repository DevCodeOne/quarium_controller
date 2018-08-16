#include "chrono_time.h"

std::time_t convert_to_time_t_localtime(const std::tm *date) {
    std::tm date_cpy;
    std::memcpy(&date_cpy, date, sizeof(std::tm));

    int32_t time_offset = timezone;
    if (date->tm_isdst > 0) {
        time_offset -= 60*60;
    }

    return std::mktime(&date_cpy) - time_offset;
}

weekday get_weekday(const days &days_since_epoch) {
    // The epoch started at a thursday so for monday to be the 0th day we have to subtract this
    return (weekday) (days_since_epoch.count() % 7 - (int) weekday::thursday);
}

std::string to_string(const weekday &day) {
    switch(day) {
        case weekday::monday:
            return "Monday";
        case weekday::tuesday:
            return "Tuesday";
        case weekday::wednesday:
            return "Wednesday";
        case weekday::thursday:
            return "Thursday";
        case weekday::friday:
            return "Friday";
        case weekday::saturday:
            return "Saturday";
        case weekday::sunday:
            return "Sunday";
        default:
            return "?";
    }
}
