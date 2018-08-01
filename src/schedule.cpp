#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

#include "logger.h"
#include "schedule.h"

std::optional<std::vector<schedule>> load_schedules(const std::filesystem::path &schedule_file_path) {
    using namespace nlohmann;

    std::ifstream file_as_stream(schedule_file_path);

    if (!file_as_stream) {
        logger::instance()->critical("Couldn't open schedule file {}",
                                     std::filesystem::canonical(schedule_file_path).c_str());
        return {};
    }

    json schedule_file;
    bool is_valid = true;

    try {
        schedule_file = json::parse(file_as_stream);
    } catch (std::exception &e) {
        logger::instance()->critical("Schedule file {} contains errors",
                                     std::filesystem::canonical(schedule_file_path).c_str());
        logger::instance()->critical(e.what());
        is_valid = false;
    }

    if (!is_valid) {
        return {};
    }

    auto actions = schedule_file["actions"];

    auto schedule = schedule_file["schedule"];

    for (auto current_action : actions) {
        std::cout << current_action << std::endl;
    }

    return {};
}
