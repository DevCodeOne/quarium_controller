#pragma once

#include <optional>
#include <string>
#include <chrono>
#include <vector>

#include "nlohmann/json.hpp"

#include "../chrono_time.h"
#include "schedule_action.h"

using json = nlohmann::json;

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


