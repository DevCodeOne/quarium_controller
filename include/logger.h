#pragma once

#include <memory>
#include <mutex>

#include "spdlog/spdlog.h"

class logger {
    public:
        static std::shared_ptr<spdlog::logger> instance();

    private:

        static inline std::shared_ptr<spdlog::logger> _instance = nullptr;
        static inline std::mutex _instance_mutex;
};
