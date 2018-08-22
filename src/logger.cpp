#include "logger.h"

void logger::configure_logger(const log_level &level, const log_type &type) {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        switch (type) {
            case log_type::console:
                _instance = spdlog::stdout_color_mt(logger_name);
                break;
            case log_type::file:
                _instance = spdlog::basic_logger_mt(logger_name, filepath, true);
        }
    }

    _instance->set_level((spdlog::level::level_enum)level);
    _instance->flush_on((spdlog::level::level_enum)level);
}

std::shared_ptr<spdlog::logger> logger::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = spdlog::stdout_color_mt(logger_name);
    }

    return _instance;
}
