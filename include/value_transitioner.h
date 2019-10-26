#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <shared_mutex>
#include <thread>

template<typename ValueType>
class value_transitioner final {
   public:
    using value_type_t = ValueType;

    template<typename Callable>
    value_transitioner(Callable transition_step, value_type_t current_value);
    value_transitioner(value_transitioner &other);
    value_transitioner(const value_transitioner &&other);
    ~value_transitioner();

    value_transitioner &operator=(value_transitioner &other);
    value_transitioner &operator=(const value_transitioner &&other);

    value_transitioner &target_value(value_type_t new_target);

   private:
    template<typename Callable>
    void do_transitions(Callable transition_step);

    mutable std::mutex m_intance_mutex;
    std::thread m_transition_thread;
    std::condition_variable m_wake_up;
    std::atomic_bool m_exit_thread;

    value_type_t m_target_value;
    value_type_t m_current_value;
};

template<typename ValueType>
template<typename Callable>
value_transitioner<ValueType>::value_transitioner(Callable transition_step, value_type_t current_value)
    : m_current_value(current_value), m_target_value(current_value), m_exit_thread(false) {
    m_transition_thread = std::thread([this, transition_step]() { do_transitions(transition_step); });
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
    std::lock_guard<std::mutex> instance_guard{m_intance_mutex};

    m_target_value = target_value;
    return *this;
}

template<typename ValueType>
template<typename Callable>
void value_transitioner<ValueType>::do_transitions(Callable transition_step) {
    using namespace std::chrono;

    milliseconds then = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    milliseconds delta_time{0};

    while (!m_exit_thread.load()) {
        std::unique_lock<std::mutex> instance_guard{m_intance_mutex};

        delta_time = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()) - then;

        // Maybe check return value, to check if the transition is already done
        transition_step(delta_time, m_current_value, m_target_value);

        then = duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
        m_wake_up.wait_for(instance_guard, std::chrono::milliseconds(100));
    }
}
