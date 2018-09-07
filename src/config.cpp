#include <utility>

#include "config.h"
#include "logger.h"
#include "run_configuration.h"

std::shared_ptr<config> config::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = std::shared_ptr<config>(new config(run_configuration::instance()->config_path()));
    }

    return _instance;
}

config::config(const std::filesystem::path &config) {
    std::ifstream config_file_stream(config);

    if (!config_file_stream) {
        logger::instance()->warn("Couldn't open config file {}", config.c_str());
        return;
    }

    m_is_valid = true;
    try {
        m_config = nlohmann::json::parse(config_file_stream);
    } catch (...) {
        m_is_valid = false;
        logger::instance()->warn("Config {} contains errors", std::filesystem::canonical(config).c_str());
    }
}

config::config(config &&other) : m_is_valid(other.m_is_valid), m_config(std::move(other.m_config)) {
    other.m_is_valid = false;
}

config &config::operator=(config &&other) {
    config tmp(std::move(other));

    swap(tmp);

    return *this;
}

void config::swap(config &other) {
    using std::swap;

    swap(m_config, other.m_config);
    swap(m_is_valid, other.m_is_valid);
}

nlohmann::json config::find(const std::string &value) const {
    if (!m_is_valid) {
        return nlohmann::json{};
    }
    return m_config[value];
}

void swap(config &lhs, config &rhs) { lhs.swap(rhs); }
