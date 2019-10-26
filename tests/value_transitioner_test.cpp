#define CATCH_CONFIG_MAIN
#include "value_transitioner.h"

#include <iostream>

#include "catch2/catch.hpp"

TEST_CASE("value_transitioner basic test") {
    int value = 0;

    auto transition_step = [](auto time_diff, auto &current_value, const auto &target_value) -> bool {
        if (*current_value == *target_value) {
            return false;
        }

        int distance = *target_value - *current_value;

        if (std::abs(distance) > 10) {
            int one_step = (int)(distance / std::abs(distance) * 10);
            *current_value += one_step;
        } else {
            *current_value = *target_value;
        }

        return true;
    };

    value_transitioner<int *> transitioner(transition_step, &value);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int target = 100;

    transitioner.target_value(&target);

    // TODO: improve this test
    std::this_thread::sleep_for(std::chrono::milliseconds((10 + 1) * 100));
    REQUIRE(value == target);
}
