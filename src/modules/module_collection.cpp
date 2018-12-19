#include "modules/module_collection.h"

std::shared_ptr<module_collection> module_collection::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (_instance == nullptr) {
        _instance = std::shared_ptr<module_collection>(new module_collection);
    }

    return _instance;
}

bool module_collection::add_module(const module_id &id, std::shared_ptr<module_interface> module) {
    return m_modules.try_emplace(id, module).second;
}

const std::map<module_id, std::shared_ptr<module_interface>> module_collection::get_modules() const {
    return m_modules;
}
