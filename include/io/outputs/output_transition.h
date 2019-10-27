#pragma once

#include <chrono>

#include "logger.h"
#include "output_value.h"

namespace output_transitions {
    static inline bool instant(std::chrono::milliseconds delta, output_value &current_value,
                               const output_value &target_value) {
        current_value = target_value;
        return true;
    }

    template<uint32_t velocity, typename PeriodType = std::chrono::milliseconds, uint32_t Period = 100>
    bool linear_transition(std::chrono::milliseconds delta, output_value &current_value,
                           const output_value &target_value) {
        if (current_value.current_type() != target_value.current_type()) {
            current_value = target_value;
            return true;
        }

        uint32_t step = velocity;

        bool finished_transition = false;

        auto do_step = [](auto current_value, auto target_value, auto step) {
            if (std::abs((int32_t)current_value - (int32_t)target_value) < step) {
                return target_value;
            }

            if (current_value < target_value) {
                return decltype(current_value)(current_value + step);
            }

            return decltype(current_value)(current_value - step);
        };
        switch (current_value.current_type()) {
            case output_value_types::number:
                current_value = do_step(*current_value.get<int>(), *target_value.get<int>(), step);
                break;
            case output_value_types::number_unsigned:
                current_value = do_step(*current_value.get<unsigned int>(), *target_value.get<unsigned int>(), step);
                break;
            // There is no sensible way to do this
            default:
                logger::instance()->warn("Not an expected type for a linear_transition");
                current_value = target_value;
                break;
        }

        return current_value == target_value;
    }

}  // namespace output_transitions
