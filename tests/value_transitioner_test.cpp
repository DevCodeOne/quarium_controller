#define CATCH_CONFIG_MAIN
#include "value_transitioner.h"

#include <iostream>

#include "catch2/catch.hpp"

TEST_CASE("value_transitioner basic test") {
    int value = 0;
    int index = 0;

    value_transitioner<int> transitioner(value);
    auto update_local_values = [&value, &transitioner, &index]() {
        value = transitioner.current_value();
        std::cout << value << std::endl;
        ++index;
    };
    auto transition_step = [&update_local_values](auto time_diff, const auto &current_value, const auto &target_value) {
        std::cout << "Running" << std::endl;
        auto result = current_value;

        int distance = target_value - current_value;

        if (std::abs(distance) > 10) {
            int one_step = (int)(distance / std::abs(distance) * 10);
            result += one_step;
        } else {
            result = target_value;
        }

        update_local_values();

        return transition_state::value_did_change;
    };

    transitioner.start_transition_thread(transition_step);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int target = 100;

    transitioner.target_value(target);

    // TODO: improve this test has race condition, though not a very critical one
    std::this_thread::sleep_for(std::chrono::milliseconds((10 + 1) * 100));
    REQUIRE(value == target);
    REQUIRE(index > 10);
}
