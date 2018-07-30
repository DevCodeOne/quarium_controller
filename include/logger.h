#pragma once

#include <memory>
#include <mutex>

#include "spdlog/spdlog.h"

class logger {
    public:
        enum struct log_level {
            debug = spdlog::level::debug,
            info = spdlog::level::info,
            warn = spdlog::level::warn,
            critical = spdlog::level::critical,
            off = spdlog::level::off,
        };

        static std::shared_ptr<spdlog::logger> instance();

    private:

        static inline std::shared_ptr<spdlog::logger> _instance = nullptr;
        static inline std::mutex _instance_mutex;
};
