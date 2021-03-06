#pragma once

#include <memory>
#include <mutex>
#include <optional>

#include "logger.h"
#include "network/network_interface.h"

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
    run_configuration &server_port(port new_server_port);
    run_configuration &log_level(logger::log_level &new_log_level);
    run_configuration &print_to_console(bool new_print_to_console);

    const std::string &config_path() const;
    const std::string &log_file() const;
    const port &server_port() const;
    const bool &print_to_console() const;
    const logger::log_level &log_level() const;

   private:
    run_configuration() = default;

    static inline std::shared_ptr<run_configuration> _instance{nullptr};
    static inline std::recursive_mutex _instance_mutex{};

    static constexpr inline char _default_config_path[] = DEFAULT_CONFIG_PATH;

    std::string m_config_path = _default_config_path;
    std::string m_log_file = "/var/log/quarium_controller.log";
    bool m_print_to_console = false;
    port m_server_port = port(9980);
    logger::log_level m_log_level;
};
