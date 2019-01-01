#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include "nlohmann/json.hpp"

#include "pattern_templates/singleton.h"
#include "io/output_value.h"

using json = nlohmann::json;

class output_interface {
   public:
    output_interface() = default;
    virtual ~output_interface() = default;

    virtual bool control_output(const output_value &value) = 0;
    virtual bool override_with(const output_value &value) = 0;
    virtual bool restore_control() = 0;
    virtual std::optional<output_value> is_overriden() const = 0;
    virtual output_value current_state() const = 0;

   private:
};

class output_factory : public singleton<output_factory> {
   public:
    using factory_func = std::function<std::unique_ptr<output_interface>(const json &description)>;

    static std::shared_ptr<output_factory> instance();
    static std::unique_ptr<output_interface> deserialize(const std::string &type, const json &description);
    template<typename T>
    static bool register_interface(const std::string &type, T func);

   private:
    std::map<std::string, factory_func> m_factories;
};

template<typename T>
bool output_factory::register_interface(const std::string &type, T func) {
    auto lock = retrieve_instance_lock();
    auto factory_instance = instance();

    if (!factory_instance) {
        return false;
    }

    return factory_instance->m_factories.try_emplace(type, func).second;
}

