#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "ring_buffer.h"

TEST_CASE("ring_buffer testing") {
    std::initializer_list<int> initial_values{10, 42};
    ring_buffer<int, 10> buffer(initial_values);

    REQUIRE(buffer.capacity() == 10);
    REQUIRE(buffer.size() == 2);
    REQUIRE(buffer.at(0).value() == 10);
    REQUIRE(buffer.at(1).value() == 42);
    REQUIRE(buffer.at(2).has_value() == false);

    for (int i = 0; i < 8; i++) {
        buffer.put(i);
    }

    REQUIRE(buffer.size() == 10);
    REQUIRE(buffer.at(10).has_value() == false);
    REQUIRE(buffer.at(9).has_value());

    for (int i = 0; i < 8; i++) {
        REQUIRE(buffer.at(i + initial_values.size()).has_value());
        REQUIRE(buffer.at(i + initial_values.size()).value() == i);
    }

    buffer.put(123);
    buffer.put(815);

    REQUIRE(buffer.at(8).has_value());
    REQUIRE(buffer.at(8).value() == 123);

    REQUIRE(buffer.at(9).has_value());
    REQUIRE(buffer.at(9).value() == 815);

    buffer.remove_last_element();

    REQUIRE(buffer.at(9).has_value() == false);
    REQUIRE(buffer.at(8).has_value());
    REQUIRE(buffer.at(8).value() == 123);

    REQUIRE(buffer.at(0).has_value());
    REQUIRE(buffer.at(0).value() == 0);

    ring_buffer<int, 5> buffer_two;

    REQUIRE(buffer_two.size() == 0);
    REQUIRE(buffer_two.at(0).has_value() == false);

    buffer_two.put(42);

    REQUIRE(buffer_two.retrieve_last_element().has_value());
    REQUIRE(buffer_two.retrieve_last_element().value() == 42);

    buffer_two.put(25);

    REQUIRE(buffer_two.retrieve_last_element().has_value());
    REQUIRE(buffer_two.retrieve_last_element().value() == 25);

    buffer_two.remove_last_element();

    REQUIRE(buffer_two.retrieve_last_element().has_value());
    REQUIRE(buffer_two.retrieve_last_element().value() == 42);

    buffer_two.remove_last_element();

    REQUIRE(buffer_two.retrieve_last_element().has_value() == false);
}
