#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "io/gpio/gpio_pin.h"
#include "io/gpio/gpiod_wrapper.h"
#include "network/network_interface.h"
#include "network/rest_resource.h"

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
    std::shared_ptr<gpio_pin> open_pin(const gpio_pin_id &id);

    nlohmann::json serialize() const;
    // TODO implement this
    static std::optional<gpio_chip> deserialize(const nlohmann::json &description);

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
