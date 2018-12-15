#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

#include "module_interface.h"

class remote_function final : public module_interface {
   public:
    virtual bool value(const std::string &id, const module_value_types &value) override;
    virtual bool update_values() override;

    virtual std::optional<module_value_types> value(const std::string &id) const override;
    virtual const module_interface_description &description() const override;

   private:
    remote_function();

    std::map<std::string, module_value_types> m_values;
};

