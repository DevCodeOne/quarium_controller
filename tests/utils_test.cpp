#define CATCH_CONFIG_MAIN
#include "utils.h"

#include <array>

#include "catch2/catch.hpp"

TEST_CASE("Utils unit tests") {
    using namespace std::literals;
    using namespace std::chrono;

    std::array<std::string_view, 4> valid_values{"100ms"sv, "100s"sv, "100m"sv, "100h"sv};
    auto valid_value_units = std::make_tuple(100ms, 100s, 100min, 100h);
    auto invalid_value = "100"sv;
    auto another_invalid_value = "10.0ms"sv;
    auto also_invalid_value = "10.0ms00"sv;

    REQUIRE(parse_duration<std::chrono::milliseconds>(std::get<0>(valid_values)).has_value());
    REQUIRE(*parse_duration<std::chrono::milliseconds>(std::get<0>(valid_values)) == std::get<0>(valid_value_units));

    REQUIRE(parse_duration<std::chrono::seconds>(std::get<1>(valid_values)).has_value());
    REQUIRE(*parse_duration<std::chrono::seconds>(std::get<1>(valid_values)) == std::get<1>(valid_value_units));

    REQUIRE(parse_duration<std::chrono::seconds>(std::get<2>(valid_values)).has_value());
    REQUIRE(*parse_duration<std::chrono::seconds>(std::get<2>(valid_values)) == std::get<2>(valid_value_units));

    REQUIRE(parse_duration<std::chrono::seconds>(std::get<3>(valid_values)).has_value());
    REQUIRE(*parse_duration<std::chrono::seconds>(std::get<3>(valid_values)) == std::get<3>(valid_value_units));

    REQUIRE(!parse_duration<std::chrono::milliseconds>(invalid_value).has_value());
    REQUIRE(!parse_duration<std::chrono::milliseconds>(another_invalid_value).has_value());
    REQUIRE(!parse_duration<std::chrono::milliseconds>(also_invalid_value).has_value());
}
