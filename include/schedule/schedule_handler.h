#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "nlohmann/json.hpp"

#include "../chrono_time.h"
#include "gpio/gpio_handler.h"
#include "schedule.h"

using json = nlohmann::json;

class schedule_handler {
   public:
    static std::shared_ptr<schedule_handler> instance();

    bool add_schedule(schedule sched);
    void start_event_handler();
    void stop_event_handler();

   private:
    schedule_handler() = default;
    static void event_handler();
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
