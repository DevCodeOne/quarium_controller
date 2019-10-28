#pragma once

#include <chrono>

#include "logger.h"
#include "output_value.h"

namespace output_transitions {
    template<typename PeriodType = std::chrono::milliseconds>
    class instant {
       public:
        bool operator()(std::chrono::milliseconds delta, output_value &current_value,
                        const output_value &target_value) const;
    };

    template<typename PeriodType>
    bool instant<PeriodType>::operator()(std::chrono::milliseconds delta, output_value &current_value,
                                         const output_value &target_value) const {
        current_value = target_value;
        return true;
    }

    template<typename PeriodType = std::chrono::milliseconds>
    class linear_transition {
       public:
        linear_transition(uint32_t velocity, PeriodType period);

        bool operator()(std::chrono::milliseconds delta, output_value &current_value,
                        const output_value &target_value) const;

       private:
        PeriodType m_period = 0;
        uint32_t m_velocity = 0;

        mutable PeriodType time_diff_since_last_inc{0};
    };

    template<typename PeriodType>
    linear_transition<PeriodType>::linear_transition(uint32_t velocity, PeriodType period)
        : m_period(period), m_velocity(velocity) {}

    template<typename PeriodType>
    bool linear_transition<PeriodType>::operator()(std::chrono::milliseconds delta, output_value &current_value,
                                                   const output_value &target_value) const {
        if (current_value.current_type() != target_value.current_type()) {
            current_value = target_value;
            return true;
        }

        // TODO: incorperate time_diff into this, but then we would have to be able to do partial step (even one, where
        // step < 1 (double))
        time_diff_since_last_inc += delta;

        uint32_t step = m_velocity * (time_diff_since_last_inc / m_period);

        time_diff_since_last_inc = time_diff_since_last_inc % m_period;

        if (step > 0) {
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
                    current_value =
                        do_step(*current_value.get<unsigned int>(), *target_value.get<unsigned int>(), step);
                    break;
                // There is no sensible way to do this
                default:
                    logger::instance()->warn("Not an expected type for a linear_transition");
                    current_value = target_value;
                    break;
            }
        }

        return current_value == target_value;
    }
}  // namespace output_transitions
