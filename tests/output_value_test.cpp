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
