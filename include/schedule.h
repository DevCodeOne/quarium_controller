#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <thread>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "chrono_time.h"
#include "gpio_handler.h"

using json = nlohmann::json;

using schedule_action_id = std::string;
using schedule_gpio_id = std::string;

class schedule_gpio {
   public:
    static bool is_valid_id(const schedule_gpio_id &id);
    static bool control_pin(const schedule_gpio_id &id, gpio_pin::action &action);

    schedule_gpio(const schedule_gpio_id &id, const gpio_pin_id &pin_id);
    schedule_gpio(const schedule_gpio &other) = delete;
    schedule_gpio(schedule_gpio &&other);

    const schedule_gpio_id &id() const;
    const gpio_pin_id &pin() const;

   private:
    static bool add_gpio(json &gpio_description);

    schedule_gpio_id m_id;
    gpio_pin_id m_pin_id;

    static inline std::vector<std::unique_ptr<schedule_gpio>> _gpios;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};

class schedule_action {
   public:
    static bool is_valid_id(const schedule_action_id &id);
    static bool execute_action(const schedule_action_id &id);

    schedule_action() = default;
    schedule_action(const schedule_action &other) = delete;
    schedule_action(schedule_action &&other);

    schedule_action &id(const schedule_action_id &new_id);
    schedule_action &add_pin(const std::pair<schedule_gpio_id, gpio_pin::action> &new_pin);

    const schedule_action_id &id() const;
    const std::vector<std::pair<schedule_gpio_id, gpio_pin::action>> &pins() const;

    bool operator()();

   private:
    static bool add_action(json &schedule_action_description);

    schedule_action_id m_id;
    std::vector<std::pair<schedule_gpio_id, gpio_pin::action>> m_pins;

    static inline std::vector<std::unique_ptr<schedule_action>> _actions;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};

class schedule_event {
   public:
    static std::optional<schedule_event> create_from_description(json &schedule_event_description);

    schedule_event &id(const std::string &new_id);
    schedule_event &trigger_time(const std::chrono::minutes new_trigger_time);
    schedule_event &day(const days &schedule_start_offset);
    schedule_event &add_action(const schedule_action_id &action_id);

    const std::string &id() const;
    std::chrono::minutes trigger_time() const;
    days day() const;
    const std::vector<schedule_action_id> &actions() const;
    bool is_marked() const;
    void unmark() const;
    void mark() const;

   private:
    std::string m_id;
    std::chrono::minutes m_trigger_time;
    days m_day;
    std::vector<schedule_action_id> m_actions;
    mutable bool m_marker = false;
};

class schedule {
   public:
    enum struct mode { repeating, single_iteration };

    static std::optional<schedule> create_from_file(const std::filesystem::path &schedule_file_path);
    static std::optional<schedule> create_from_description(json &schedule_description);

    schedule &start_at(const days &new_start);
    schedule &end_at(const days &new_end);
    schedule &period(const days &period);
    schedule &title(const std::string &new_title);
    schedule &schedule_mode(const mode &new_mode);
    bool add_event(const schedule_event &event);

    std::optional<days> start_at() const;
    std::optional<days> end_at() const;
    days period() const;
    const std::string &title() const;
    const std::vector<schedule_event> &events() const;
    mode schedule_mode() const;

    explicit operator bool() const;

   private:
    static bool events_conflict(const std::vector<schedule_event> &events);

    void recalculate_period();

    std::optional<days> m_start_at{};
    std::optional<days> m_end_at{};
    days m_period{0};
    std::string m_title{""};
    mode m_mode{mode::repeating};
    std::vector<schedule_event> m_events;

    static inline std::atomic_int _instance_count = 0;
};

class schedule_handler {
   public:
    static std::shared_ptr<schedule_handler> instance();

    bool add_schedule(schedule sched);
    void start_event_handler();
    void stop_event_handler();

   private:
    schedule_handler() = default;
    static void event_handler();
    static days days_since_epoch();
    bool is_conflicting_with_other_schedules(const schedule &sched);

    std::vector<schedule> m_active_schedules;
    std::vector<schedule> m_inactive_schedules;
    std::recursive_mutex m_schedules_list_mutex;
    std::thread m_event_thread;
    bool m_is_started = false;
    std::atomic_bool m_should_exit = false;

    static inline std::mutex _instance_mutex;
    static inline std::shared_ptr<schedule_handler> _instance;
};
