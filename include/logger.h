#pragma once

#include <memory>
#include <mutex>

#include "spdlog/spdlog.h"

#include "network_header.h"

class logger {
   public:
    enum struct log_level {
        debug = spdlog::level::debug,
        info = spdlog::level::info,
        warn = spdlog::level::warn,
        critical = spdlog::level::critical,
        off = spdlog::level::off,
    };

    enum struct log_type { console, file };

    static void configure_logger(const log_level &level, const log_type &type = log_type::console);
    static std::shared_ptr<spdlog::logger> instance();

    static void handle(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response);

   private:
    static inline std::shared_ptr<spdlog::logger> _instance = nullptr;
    static inline std::mutex _instance_mutex;

    static inline constexpr char _logger_name[] = "default";
};
