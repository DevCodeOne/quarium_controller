#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "gpio/gpiod_wrapper.h"
#include "network/rest_resource.h"

class gpio_chip;
class gpio_pin;

// TODO replace std::shared_ptr with std::weak_ptr because gpio_chip has a reference to gpio_pin_id and gpio_pin_id to
// gpio_chip -> issues occur when gpio_chip tries to destroy its references to gpio_pin_id
class gpio_pin_id final {
   public:
    gpio_pin_id(unsigned int id, std::shared_ptr<gpio_chip> chip);

    unsigned int id() const;
    const std::filesystem::path &gpio_chip_path() const;

    std::shared_ptr<gpio_pin> open_pin();

   private:
    const std::filesystem::path m_gpiochip_path = "";
    unsigned int m_id;
};

bool operator<(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator>(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator>=(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator<=(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator==(const gpio_pin_id &lhs, const gpio_pin_id &rhs);
bool operator!=(const gpio_pin_id &lhs, const gpio_pin_id &rhs);

class gpio_pin final : public rest_resource<gpio_pin> {
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
    gpio_pin::action current_state() const;

    nlohmann::json serialize() const;
    // TODO implement this
    static std::optional<gpio_chip> deserialize(nlohmann::json &description);

   private:
    static std::optional<gpio_pin> open(gpio_pin_id id);

    bool update_gpio();

    gpio_pin(gpio_pin_id id, gpiod::gpiod_line line);

    gpiod::gpiod_line m_line;
    gpio_pin::action m_controlled_action;
    std::optional<gpio_pin::action> m_overriden_action;
    const gpio_pin_id m_id;

    friend class gpio_chip;
};
