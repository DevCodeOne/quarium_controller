#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "chrono_time.h"
#include "schedule/schedule.h"

TEST_CASE("Correct time") {
    constexpr const char date_format[] = "%d.%m.%Y %H:%M";
    auto first_day_test = convert_date_to_duration_since_epoch<days>("01.01.1970 00:00", date_format);
    REQUIRE(first_day_test.has_value());
    REQUIRE(first_day_test->count() == 0);

    auto second_day_test = convert_date_to_duration_since_epoch<days>("02.01.1970 00:00", date_format);
    REQUIRE(second_day_test.has_value());
    REQUIRE(second_day_test->count() == 1);

    auto third_day_test = convert_date_to_duration_since_epoch<days>("03.01.1970 00:00", date_format);
    REQUIRE(third_day_test.has_value());
    REQUIRE(third_day_test->count() == 2);

    auto tenth_day_test = convert_date_to_duration_since_epoch<days>("10.01.1970 00:00", date_format);
    REQUIRE(tenth_day_test.has_value());
    REQUIRE(tenth_day_test->count() == 9);

    auto first_minute_test = convert_date_to_duration_since_epoch<std::chrono::minutes>("01.01.1970 00:00", date_format);
    REQUIRE(first_minute_test.has_value());
    REQUIRE(first_minute_test->count() == 0);

    auto tenth_minute_test = convert_date_to_duration_since_epoch<std::chrono::minutes>("01.01.1970 00:10", date_format);
    REQUIRE(tenth_minute_test.has_value());
    REQUIRE(tenth_minute_test->count() == 10);


}
