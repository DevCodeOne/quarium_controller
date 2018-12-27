#pragma once

#include <memory>
#include <optional>
#include <string>

#include "pattern_templates/singleton.h"
#include "schedule/output_value.h"

using schedule_output_id = std::string;

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
    static std::unique_ptr<output_interface> deserialize(const std::string &type, const json &description);
    static bool register_interface(const std::string &type, factory_func func);

   private:
    std::map<std::string, factory_func> m_factories;
};

