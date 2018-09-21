#pragma once

#include <optional>

#include "nlohmann/json.hpp"

template<typename T>
class rest_resource {
   public:
    nlohmann::json serialize() const;
    static std::optional<T> deserialize(nlohmann::json &description);
};
