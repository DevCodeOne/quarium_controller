#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "gpio/gpiod_wrapper.h"
#include "network/network_interface.h"
#include "network/rest_resource.h"

class gpio_chip;
class gpio_pin;

class gpio_pin_id final {
   public:
    gpio_pin_id(unsigned int id, std::shared_ptr<gpio_chip> chip);

    unsigned int id() const;
    const std::shared_ptr<gpio_chip> chip() const;
    std::shared_ptr<gpio_pin> open_pin();

   private:
    const std::shared_ptr<gpio_chip> m_chip = nullptr;
    unsigned int m_id;
};

bool operator<(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator>(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator>=(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator<=(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator==(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator!=(const gpio_pin_id &lhs, const gpio_pin_id &rhs);

class gpio_pin final {
   public:
    enum struct action { off = 0, on = 1, toggle = 2 };

    gpio_pin(gpio_pin &other) = delete;
    gpio_pin(gpio_pin &&other);
    ~gpio_pin() = default;

    gpio_pin &operator=(gpio_pin &other) = delete;
    gpio_pin &operator=(gpio_pin &&other);

    bool control(const action &act);
    bool override_with(const action &act);
    bool restore_control();

    unsigned int gpio_id() const;
    std::optional<gpio_pin::action> is_overriden() const;

   private:
    static std::optional<gpio_pin> open(gpio_pin_id id);

    bool update_gpio();

    gpio_pin(gpio_pin_id id, gpiod::gpiod_line line);

    gpiod::gpiod_line m_line;
    gpio_pin::action m_controlled_action;
    std::optional<gpio_pin::action> m_overriden_action;
    const gpio_pin_id m_id;

    friend class gpio_pin_id;
    friend class gpio_chip;
};

class gpio_chip final : public rest_resource<gpio_chip> {
   public:
    static inline constexpr char default_gpio_dev_path[] = "/dev/gpiochip0";

    static std::shared_ptr<gpio_chip> instance(const std::filesystem::path &gpio_chip_path = default_gpio_dev_path);
    static http::response<http::dynamic_body> handle_request(const http::request<http::dynamic_body> &request);

    gpio_chip(const gpio_chip &other) = delete;
    gpio_chip(gpio_chip &&other);
    ~gpio_chip();

    gpio_chip &operator=(const gpio_chip &other) = delete;
    gpio_chip &operator=(gpio_chip &&other);

    const std::filesystem::path &path_to_file() const;

    // TODO implement these
    nlohmann::json serialize() const;
    static std::optional<gpio_chip> deserialize(nlohmann::json &description);

   private:
    static std::optional<gpio_chip> open(const std::filesystem::path &gpio_chip_path);
    static bool reserve_gpiochip(const std::filesystem::path &gpiochip_path);
    static bool free_gpiochip(const std::filesystem::path &gpiochip_path);

    gpio_chip(const std::filesystem::path &gpio_dev, gpiod::gpiod_chip chip);

    std::filesystem::path m_gpiochip_path;
    std::map<gpio_pin_id, std::shared_ptr<gpio_pin>> m_reserved_pins;
    gpiod::gpiod_chip m_chip;

    static inline std::recursive_mutex _instance_mutex;
    static inline std::map<std::filesystem::path, std::shared_ptr<gpio_chip>> _gpiochip_access_map;

    friend class gpio_handler;
    friend class gpio_pin;
};
