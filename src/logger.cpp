#include "logger.h"

std::shared_ptr<spdlog::logger> logger::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = spdlog::stdout_color_mt("default");
    }

    return _instance;
}
