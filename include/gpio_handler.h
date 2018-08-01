#pragma once

#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

class gpio_pin {
   public:
    enum struct action { OFF, ON };

    gpio_pin(unsigned int id);

   private:
    unsigned int m_id;
};

class gpio_chip {
   public:
    static std::optional<gpio_chip> open(const std::filesystem::path &gpiochip_path = default_gpio_dev_path);

    gpio_chip(const gpio_chip &other) = delete;
    gpio_chip(gpio_chip &&other);
    ~gpio_chip();

    gpio_chip &operator=(const gpio_chip &other) = delete;
    gpio_chip &operator=(gpio_chip &&other);

    static inline constexpr char default_gpio_dev_path[] = "/dev/gpiochip0";

   private:
    gpio_chip(const std::filesystem::path &gpio_dev);

    std::filesystem::path m_gpiochip_path;
    std::vector<gpio_pin> m_reserved_pins;

    static bool reserve_gpiochip(const std::filesystem::path &gpiochip_path);
    static bool free_gpiochip(const std::filesystem::path &gpiochip_path);

    static inline std::mutex _instance_mutex;
    static inline std::map<std::filesystem::path, bool> _gpiochip_access_map;
};
