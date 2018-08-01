#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "chrono_time.h"
#include "gpio_handler.h"

using json = nlohmann::json;

using schedule_action_id = std::string;

class schedule_action {
   public:
    static bool add_action(json &schedule_action_description);
    static bool is_valid_id(const schedule_action_id &id);
    static bool execute_action(const schedule_action_id &id);

    schedule_action &id(const schedule_action_id &new_id);
    schedule_action &add_pin(const std::pair<gpio_pin, gpio_pin::action> &new_pin);

    const schedule_action_id &id() const;
    const std::vector<std::pair<gpio_pin, gpio_pin::action>> &pins() const;

    bool operator()();

   private:
    std::string m_id;
    std::vector<std::pair<gpio_pin, gpio_pin::action>> m_pins;

    static inline std::map<schedule_action_id, schedule_action> _actions;
    static inline std::mutex _list_mutex;
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
    const std::vector<schedule_action_id> &assigned_actions() const;

   private:
    std::string m_id;
    std::chrono::minutes m_trigger_time;
    days m_day;
    std::vector<schedule_action_id> m_actions;
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
    schedule &add_event(const schedule_event &event);

    std::optional<days> start_at() const;
    std::optional<days> end_at() const;
    days period() const;
    const std::string &title() const;
    const std::vector<schedule_event> &events() const;
    mode schedule_mode() const;

    explicit operator bool() const;

   private:
    static bool events_conflict(const std::vector<schedule_event> &events);

    std::optional<days> m_start_at;
    std::optional<days> m_end_at;
    days m_period;
    std::string m_title;
    mode m_mode = mode::repeating;
    std::vector<schedule_event> m_events;

    static inline std::atomic_int _instance_count = 0;
};

class schedule_handler {
   public:
    static std::shared_ptr<schedule_handler> instance();

    bool add_schedule(schedule sched);

   private:
    schedule_handler();

    bool is_conflicting_with_other_schedules();
    std::vector<schedule> m_active_schedules;
    std::vector<schedule> m_inactive_schedules;

    static inline std::mutex _instance_mutex;
    static inline std::shared_ptr<schedule_handler> _instance;
};
