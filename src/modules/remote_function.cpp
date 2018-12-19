#include "modules/remote_function.h"
#include "logger.h"

std::shared_ptr<remote_function> remote_function::deserialize(const nlohmann::json &description) {
    using json = nlohmann::json;

    json id_entry = description["id"];
    auto values_description = module_interface_description::deserialize(description["description"]);

    if (id_entry.is_null() || !id_entry.is_string()) {
        logger::instance()->warn("ID is not valid");
        return nullptr;
    }

    std::string id = id_entry.get<std::string>();

    // TODO check if values_description is valid for this module type

    return std::shared_ptr<remote_function>(new remote_function(id, values_description.value()));
}

remote_function::remote_function(const std::string &id, const module_interface_description &description)
    : module_interface(id, description) {}

bool remote_function::update_values() { return true; }
