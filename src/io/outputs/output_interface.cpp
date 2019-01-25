#include "io/outputs/output_interface.h"
#include "logger.h"

std::shared_ptr<output_factory> output_factory::instance() { return singleton<output_factory>::instance(); }

std::unique_ptr<output_interface> output_factory::deserialize(const std::string &type, const json &description) {
    auto lock = retrieve_instance_lock();
    auto output_factory_instance = instance();

    if (output_factory_instance == nullptr) {
        return nullptr;
    }

    auto result = output_factory_instance->m_factories.find(type);

    if (result == output_factory_instance->m_factories.cend()) {
        logger::instance()->critical("Output type {} couldn't be found", type);
        return nullptr;
    }

    return result->second(description);
}
