#include "config.h"
#include "logger.h"

std::shared_ptr<config> config::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = std::make_shared<config>(config(_default_config_path));
    }

    return _instance;
}

config::config(const std::filesystem::path &config) {
    std::ifstream config_file_stream(config);

    if (!config_file_stream) {
        logger::instance()->warn("Config {} not found or couldn't be opened",
                                 config.c_str());
        return;
    }

    m_is_valid = true;
    try {
        config_file_stream >> m_config;
    } catch (nlohmann::detail::parse_error &error) {
        m_is_valid = false;
        logger::instance()->warn("Config {} contains errors",
                                 std::filesystem::canonical(config).c_str());
    }
}

config::config(config &&other) {}

config &config::operator=(config &&other) { return *this; }
