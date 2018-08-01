#pragma once

#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "chrono_time.h"
#include "gpio_handler.h"

using json = nlohmann::json;

enum schedule_action_id : int32_t {};

class schedule_action {
   public:
    static bool add_action(const std::string &title);
    static bool remove_action(const std::string &title);

    schedule_action &title(const std::string &new_title);
    schedule_action &add_actions(std::pair<gpio_pin, gpio_pin::action> pins);

    std::string title() const;
    std::vector<std::pair<gpio_pin, gpio_pin::action>> pins() const;

    bool operator()();

   private:
    std::string m_title;
    std::vector<std::pair<gpio_pin, gpio_pin::action>> m_pins;

    static inline std::vector<std::shared_ptr<schedule_action>> _actions;
    static inline std::mutex _list_mutex;
};

class schedule_event {
   public:
    std::optional<schedule_event> load_schedule_event(json &schedule_event_description);
   private:
    std::string m_title;
    std::chrono::minutes m_trigger_time;
    std::vector<schedule_action> m_actions;
};

class schedule {
   public:
    std::optional<schedule> load_schedule(json &schedule_description);

    schedule &start_at(const days &new_start);
    schedule &end_at(const days &new_end);
    schedule &title(const std::string &new_title);
    schedule &repeating(const bool &new_repeating);

    days start_at() const;
    days end_at() const;
    std::string title(const std::string &new_title) const;
    bool repeating(const bool &new_repeating) const;

   private:
    schedule();

    days m_start_at;
    days m_end_at;
    std::string m_title;
    bool m_repeating;
    std::vector<schedule_event> m_events;
};

std::optional<std::vector<schedule>> load_schedules(const std::filesystem::path &schedule_file_path);

class schedule_handler {
   public:
    static std::shared_ptr<schedule_handler> instance();

    bool add_schedule(schedule sched);

   private:
    schedule_handler();
    bool is_conflicting_with_other_schedules();

    static inline std::mutex _instance_mutex;
    static inline std::shared_ptr<schedule_handler> _instance;
};
