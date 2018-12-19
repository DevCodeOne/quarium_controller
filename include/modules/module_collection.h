#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <variant>

#include "module_interface.h"
#include "remote_function.h"

using module_id = std::string;

class module_collection final {
   public:
    static std::shared_ptr<module_collection> instance();

    module_collection() = default;
    ~module_collection() = default;

    bool add_module(const module_id &id, std::shared_ptr<module_interface> module);
    const std::map<module_id, std::shared_ptr<module_interface>> get_modules() const;

   private:
    static inline std::shared_ptr<module_collection> _instance = nullptr;
    static inline std::mutex _instance_mutex;

    std::map<module_id, std::shared_ptr<module_interface>> m_modules;
};
