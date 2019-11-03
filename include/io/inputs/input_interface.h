#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "pattern_templates/singleton.h"

using json = nlohmann::json;

class input_interface {
   public:
    input_interface() = default;
    virtual ~input_interface() = default;

    void read_value();
};

class input_factory : public singleton<input_factory> {
   public:
    using factory_func = std::function<std::unique_ptr<input_interface>(const json &description)>;
    static std::shared_ptr<input_factory> instance();
    static std::unique_ptr<input_factory> deserialize(const std::string &type, const json &description);
    template<typename T>
    static bool register_interface(const std::string &type, T func);

   private:
    std::map<std::string, factory_func> m_factories;
};
