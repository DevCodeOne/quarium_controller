#pragma once

#include <memory>
#include <mutex>
#include <filesystem>
#include <optional>
#include <fstream>

#include "nlohmann/json.hpp"

class config {
   public:
    static std::shared_ptr<config> instance();

    config(const config &other) = delete;
    config(config &&other);

    config &operator=(const config &other) = delete;
    config &operator=(config &&other);

   private:
    config(const std::filesystem::path &config_path);

    bool m_is_valid = false;
    nlohmann::json m_config;

    static inline std::shared_ptr<config> _instance = nullptr;
    static inline std::mutex _instance_mutex;
    static constexpr inline char _default_config_path [] = DEFAULT_CONFIG_PATH;
};
