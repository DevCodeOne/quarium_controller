#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

enum struct transition_state { value_did_change, value_did_not_change, finished_transition };

// TODO: make at least moveable
template<typename ValueType>
class value_transitioner final {
   public:
    using value_type_t = ValueType;

    value_transitioner(value_type_t current_value);
    value_transitioner(const value_transitioner &other) = delete;
    value_transitioner(value_transitioner &&other) = delete;
    ~value_transitioner();

    value_transitioner &operator=(const value_transitioner &other) = delete;
    value_transitioner &operator=(value_transitioner &&other) = delete;

    template<typename Callable>
    value_transitioner &start_transition_thread(Callable callable,
                                                std::chrono::milliseconds period = std::chrono::milliseconds(100));

    value_transitioner &target_value(value_type_t new_target);
    value_type_t current_value() const;

   private:
    template<typename Callable>
    void do_transitions(Callable transition_step);

    mutable std::mutex m_instance_mutex;
    mutable std::recursive_mutex m_value_mutex;
    std::thread m_transition_thread;
    std::condition_variable m_wake_up;
    std::chrono::milliseconds m_period = std::chrono::milliseconds(100);
    std::atomic_bool m_exit_thread;
    std::atomic_bool m_is_running;

    value_type_t m_target_value;
    value_type_t m_current_value;
};

template<typename ValueType>
value_transitioner<ValueType>::value_transitioner(value_type_t current_value)
    : m_current_value(current_value), m_target_value(current_value), m_exit_thread(false) {}

template<typename ValueType>
template<typename Callable>
auto value_transitioner<ValueType>::start_transition_thread(Callable callable, std::chrono::milliseconds period)
    -> value_transitioner & {
    std::lock_guard<std::mutex> instance_guard{m_instance_mutex};
    m_period = period;

    m_transition_thread = std::thread([this, callable]() { do_transitions(callable); });
    return *this;
}

template<typename ValueType>
value_transitioner<ValueType>::~value_transitioner() {
    m_exit_thread.store(true);
    m_wake_up.notify_all();

    if (m_transition_thread.joinable()) {
        m_transition_thread.join();
    }
}

template<typename ValueType>
auto value_transitioner<ValueType>::target_value(value_type_t target_value) -> value_transitioner<value_type_t> & {
    {
        std::lock_guard<std::mutex> instance_guard{m_instance_mutex};
        m_target_value = target_value;
    }
    m_wake_up.notify_all();
    return *this;
}

template<typename ValueType>
auto value_transitioner<ValueType>::current_value() const -> value_type_t {
    std::lock_guard<std::recursive_mutex> value_guard{m_value_mutex};

    return m_current_value;
}

template<typename ValueType>
template<typename Callable>
void value_transitioner<ValueType>::do_transitions(Callable transition_step) {
    using namespace std::chrono;

    milliseconds then = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    milliseconds delta_time{0};

    while (!m_exit_thread.load()) {
        std::unique_lock<std::mutex> instance_guard{m_instance_mutex};

        delta_time = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()) - then;

        transition_state current_state;
        auto time_transition_step_call = steady_clock::now();

        // Maybe check return value, to check if the transition is already done
        {
            std::lock_guard<std::recursive_mutex> value_guard{m_value_mutex};
            current_state = transition_step(delta_time, m_current_value, m_target_value);
        }

        if (current_state != transition_state::finished_transition) {
            then = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
            m_wake_up.wait_until(instance_guard, time_transition_step_call + m_period);
        } else {
            m_wake_up.wait(instance_guard);
            then = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
        }
    }
}
