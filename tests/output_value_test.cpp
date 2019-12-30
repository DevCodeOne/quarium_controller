#define CATCH_CONFIG_MAIN
#include "io/outputs/output_value.h"

#include <algorithm>
#include <string>

#include "catch2/catch.hpp"

TEST_CASE("Basic output_value tests") {
    using namespace std::literals;

    output_value value(0, std::make_optional<int>(20), std::optional<int>{});
    output_value value4(""s);

    REQUIRE(value.get<int>().has_value());
    REQUIRE(value.get<int>().value_or(20) != 20);

    REQUIRE(value.min<int>().has_value());
    REQUIRE(value.min<int>().value_or(-1) == 20);

    REQUIRE(!value.max<int>().has_value());
}

TEST_CASE("Basic deserializing tests") {
    auto created_value = output_value::deserialize(R"("20")", output_value_types::number);
    auto created_value2 = output_value::deserialize(R"(20)", output_value_types::number_unsigned);

    REQUIRE(created_value.has_value());
    REQUIRE(created_value2.has_value());
    REQUIRE(created_value->current_type() == output_value_types::number);
    REQUIRE(created_value2->current_type() == output_value_types::number_unsigned);
}

TEST_CASE("Basic mutator test") {
    output_value value(0, std::make_optional<int>(0), std::make_optional<int>(20));

    REQUIRE(value.get<int>().has_value());
    REQUIRE(value.get<int>().value() == 0);

    REQUIRE(value.min<int>().has_value());
    REQUIRE(value.min<int>().value() == 0);

    REQUIRE(value.max<int>().has_value());
    REQUIRE(value.max<int>().value() == 20);

    REQUIRE(value.set(10) == true);
    REQUIRE(value.get<int>() == 10);

    REQUIRE(value.set(21) == false);
    REQUIRE(value.get<int>() == 10);

    REQUIRE(value.set(0) == true);
    REQUIRE(value.get<int>() == 0);

    REQUIRE(value.set(-1) == false);
    REQUIRE(value.get<int>() == 0);

    REQUIRE(value.set(0.5f) == false);
    REQUIRE(value.get<int>() == 0);
}
