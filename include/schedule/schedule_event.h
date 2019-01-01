#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "chrono_time.h"
#include "network/rest_resource.h"
#include "schedule/schedule_action.h"

using json = nlohmann::json;

class schedule_event final : public rest_resource<schedule_event> {
   public:
    static std::optional<schedule_event> deserialize(json &schedule_event_description);

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

    nlohmann::json serialize() const;

   private:
    std::string m_id;
    std::chrono::minutes m_trigger_time;
    days m_day;
    std::vector<schedule_action_id> m_actions;
    mutable bool m_marker = false;
};

bool is_earlier(const schedule_event &lhs, const schedule_event &rhs);
