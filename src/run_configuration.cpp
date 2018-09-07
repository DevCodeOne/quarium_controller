#include "run_configuration.h"

std::shared_ptr<run_configuration> run_configuration::instance() {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = std::shared_ptr<run_configuration>(new run_configuration());
    }

    return _instance;
}

run_configuration &run_configuration::config_path(std::string new_config_path) {
    m_config_path = new_config_path;
    return *this;
}

run_configuration &run_configuration::log_file(std::string new_log_file) {
    m_log_file = new_log_file;
    return *this;
}

run_configuration &run_configuration::server_port(uint16_t new_server_port) {
    m_server_port = new_server_port;
    return *this;
}

run_configuration &run_configuration::log_level(logger::log_level &new_log_level) {
    m_log_level = new_log_level;
    return *this;
}

const std::string &run_configuration::config_path() const { return m_config_path; }

const std::string &run_configuration::log_file() const { return m_log_file; }

const uint16_t &run_configuration::server_port() const { return m_server_port; }

const logger::log_level &run_configuration::log_level() const { return m_log_level; }
