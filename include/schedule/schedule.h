#pragma once

#include <atomic>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "chrono_time.h"
#include "network/rest_resource.h"
#include "schedule_event.h"

using json = nlohmann::json;

class schedule final : public rest_resource<schedule, rest_resource_types::entry> {
   public:
    enum struct mode { repeating, single_iteration };

    static std::optional<schedule> create_from_file(const std::filesystem::path &schedule_file_path);
    static std::optional<schedule> deserialize(json &schedule_description);

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
    nlohmann::json serialize() const;

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
