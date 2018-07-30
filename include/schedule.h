#pragma once

#include <chrono>
#include <filesystem>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "gpio_handler.h"

using days = std::chrono::duration<int, std::ratio<24 * std::chrono::hours::duration::period::num>>;

enum schedule_action_id : int {};

class schedule_action {
   public:
    bool operator()();

   private:
    std::string m_title;
    std::vector<std::pair<gpio_pin, gpio_pin::action>> m_pins;

    static inline std::vector<schedule_action> _actions;
    static inline std::mutex _list_mutex;
};

class schedule_event {
   public:
   private:
    std::string m_title;
    std::chrono::minutes m_trigger_time;
    std::vector<schedule_action> m_actions;
};

class schedule {
   public:
   private:
    days m_schedule_start;
    days m_schedule_end;
    std::string m_title;
    bool m_repeating;
    std::vector<schedule_event> m_events;
};

std::optional<std::vector<schedule>> load_schedules(const std::filesystem::path &schedule_file_path);
