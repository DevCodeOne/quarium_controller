#include "chrono_time.h"

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
