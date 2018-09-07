#pragma once

#include <memory>
#include <mutex>
#include <optional>

#include "logger.h"

class run_configuration {
   public:
    static std::shared_ptr<run_configuration> instance();

    run_configuration(const run_configuration &other) = delete;
    run_configuration(run_configuration &&other) = delete;
    ~run_configuration() = default;

    run_configuration &operator=(const run_configuration &other) = delete;
    run_configuration &operator=(run_configuration &&other) = delete;

    run_configuration &config_path(std::string new_config_path);
    run_configuration &log_file(std::string new_log_file);
    run_configuration &server_port(uint16_t new_server_port);
    run_configuration &log_level(logger::log_level &new_log_level);

    const std::string &config_path() const;
    const std::optional<std::string> &log_file() const;
    const uint16_t &server_port() const;
    const logger::log_level &log_level() const;

   private:
    run_configuration() = default;

    static inline std::shared_ptr<run_configuration> _instance{nullptr};
    static inline std::recursive_mutex _instance_mutex{};

    std::string m_config_path;
    std::optional<std::string> m_log_file;
    uint16_t m_server_port;
    logger::log_level m_log_level;
};
