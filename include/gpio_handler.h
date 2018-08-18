#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "gpiod.h"

class gpio_pin_id {
   public:
    gpio_pin_id(unsigned int id, std::filesystem::path gpio_chip_path);

    unsigned int id() const;
    const std::filesystem::path &gpio_chip_path() const;

   private:
    std::filesystem::path m_gpio_chip_path;
    unsigned int m_id;
};

bool operator<(const gpio_pin_id &lhs, const gpio_pin_id &rhs);

class gpio_pin {
   public:
    enum struct action { off, on, toggle };

    gpio_pin(gpio_pin &other) = delete;
    gpio_pin(gpio_pin &&other);
    ~gpio_pin() = default;

    gpio_pin &operator=(gpio_pin &other) = delete;
    gpio_pin &operator=(gpio_pin &&other);

    bool control(const action &act);
    bool override_with(const action &act);
    bool restore_control();

    unsigned int id() const;
    std::optional<gpio_pin::action> is_overriden() const;

   private:
    static std::optional<gpio_pin> open(gpio_pin_id id);

    bool update_gpio();

    gpio_pin(gpio_pin_id id, gpiod_line *line);

    std::shared_ptr<gpiod_line> m_line;
    gpio_pin::action m_controlled_action;
    std::optional<gpio_pin::action> m_overriden_action;
    const gpio_pin_id m_id;

    friend class gpio_chip;
};

class gpio_chip {
   public:
    static inline constexpr char default_gpio_dev_path[] = "/dev/gpiochip0";

    static std::optional<std::shared_ptr<gpio_chip>> instance(const std::filesystem::path &gpio_chip_path = default_gpio_dev_path);

    gpio_chip(const gpio_chip &other) = delete;
    gpio_chip(gpio_chip &&other);
    ~gpio_chip();

    gpio_chip &operator=(const gpio_chip &other) = delete;
    gpio_chip &operator=(gpio_chip &&other);

    bool control_pin(const gpio_pin_id &id, const gpio_pin::action &action);
    std::shared_ptr<gpio_pin> access_pin(const gpio_pin_id &id);

   private:
    static std::optional<gpio_chip> open(const std::filesystem::path &gpio_chip_path);

    gpio_chip(const std::filesystem::path &gpio_dev, gpiod_chip *chip);

    std::filesystem::path m_gpiochip_path;
    std::map<gpio_pin_id, std::shared_ptr<gpio_pin>> m_reserved_pins;
    gpiod_chip *m_chip;

    static bool reserve_gpiochip(const std::filesystem::path &gpiochip_path);
    static bool free_gpiochip(const std::filesystem::path &gpiochip_path);

    static inline std::recursive_mutex _instance_mutex;
    static inline std::map<std::filesystem::path, std::shared_ptr<gpio_chip>> _gpiochip_access_map;

    friend class gpio_handler;
    friend class gpio_pin;
};
