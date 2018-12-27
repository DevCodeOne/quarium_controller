#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "nlohmann/json.hpp"

#include "gpio/gpio_chip.h"
#include "pattern_templates/singleton.h"

using schedule_output_id = std::string;

using json = nlohmann::json;

// variant with all possible values for now implicit convertible
class output_value {
   public:
    output_value(const gpio_pin::action &action);

    template<typename T>
    std::optional<T> get();
};

class output_interface {
   public:
    output_interface() = default;
    virtual ~output_interface() = default;

    virtual bool control_output(const output_value &value) = 0;
    virtual output_value is_overriden() = 0;
    virtual bool override_with(const output_value &value) = 0;
    virtual bool restore_control() = 0;
    virtual output_value current_state() = 0;

   private:
};

class output_factory : public singleton<output_factory> {
   public:
    using factory_func = std::function<std::unique_ptr<output_interface>(const json &description)>;
    static std::unique_ptr<output_interface> deserialize(const std::string &type, const json &description);
    static bool register_interface(const std::string &type, factory_func func);

   private:
    std::map<std::string, factory_func> m_factories;
};

class schedule_output {
   public:
    static bool is_valid_id(const schedule_output_id &id);
    static bool control_pin(const schedule_output_id &id, const output_value &action);
    static std::optional<output_value> is_overriden(const schedule_output_id &id);
    static bool override_with(const schedule_output_id &id, const output_value &action);
    static bool restore_control(const schedule_output_id &id);
    static std::optional<output_value> current_state(const schedule_output_id &id);
    static std::vector<schedule_output_id> get_ids();

   private:
    static bool add_gpio(json &gpio_description);

    static inline std::map<schedule_output_id, std::unique_ptr<output_interface>> _outputs;
    static inline std::recursive_mutex _list_mutex;

    friend class schedule;
};
