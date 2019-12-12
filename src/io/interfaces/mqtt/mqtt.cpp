#include "io/interfaces/mqtt/mqtt.h"

#include "mqtt_client_cpp.hpp"

bool detail::mqtt_address_cmp::operator()(const mqtt_address &lhs, const mqtt_address &rhs) const {
    return lhs.m_server_name < rhs.m_server_name || lhs.m_port < rhs.m_port;
}

std::shared_ptr<mqtt> mqtt::instance(const mqtt_address &addr) {
    std::lock_guard<std::mutex> _instance_guard{_instance_mutex};

    if (_global_context.stopped()) {
        return nullptr;
    }

    if (_instances.count(addr) != 0) {
        return _instances[addr];
    }

    mqtt_sock_type c = mqtt_cpp::make_async_client(mqtt::_global_context, addr.m_server_name, (uint16_t)addr.m_port);
    _instances.emplace(std::make_pair(addr, new mqtt(addr, c)));

    auto created_instance = _instances.find(addr);

    return created_instance != _instances.cend() ? created_instance->second : nullptr;
}

mqtt::mqtt(mqtt_address addr, mqtt_sock_type mqtt_socket) : m_addr(std::move(addr)), m_mqtt_socket(mqtt_socket) {}

bool mqtt::stop_all_instances() {
    std::lock_guard<std::mutex> _instance_guard{_instance_mutex};

    if (!_global_context.stopped()) {
        _global_context.stop();
    }
    return true;
}
