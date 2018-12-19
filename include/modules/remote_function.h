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
    static std::shared_ptr<remote_function> deserialize(const nlohmann::json &description);
    virtual ~remote_function() = default;

    virtual bool update_values() override;

   private:
    remote_function(const std::string &id, const module_interface_description &description);
};

