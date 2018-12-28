#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include "gpio/gpiod_wrapper.h"
#include "io/output_interface.h"
#include "io/output_value.h"
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

class gpio_pin final : public rest_resource<gpio_pin>, public output_interface {
   public:
    gpio_pin(gpio_pin &other) = delete;
    gpio_pin(gpio_pin &&other);
    ~gpio_pin() = default;

    gpio_pin &operator=(gpio_pin &other) = delete;
    gpio_pin &operator=(gpio_pin &&other);

    virtual bool control_output(const output_value &value);
    virtual bool override_with(const output_value &value);
    virtual bool restore_control();
    virtual std::optional<output_value> is_overriden() const;
    virtual output_value current_state() const;

    unsigned int gpio_id() const;

    nlohmann::json serialize() const;
    static std::optional<gpio_pin> deserialize(const nlohmann::json &description);
    static std::unique_ptr<output_interface> create_for_interface(const nlohmann::json &description);

   private:
    static std::optional<gpio_pin> open(gpio_pin_id id);

    bool update_gpio();

    gpio_pin(gpio_pin_id id, gpiod::gpiod_line line);

    gpiod::gpiod_line m_line;
    switch_output m_controlled_value;
    std::optional<switch_output> m_overriden_value;
    const gpio_pin_id m_id;

    friend class gpio_chip;
};
