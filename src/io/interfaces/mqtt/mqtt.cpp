#include "io/interfaces/mqtt/mqtt.h"

#include <optional>

#include "logger.h"
#include "mqtt_client_cpp.hpp"

bool detail::mqtt_address_cmp::operator()(const mqtt_address &lhs, const mqtt_address &rhs) const {
    return std::strcmp(lhs.m_server_name, rhs.m_server_name) < 0 || lhs.m_port < rhs.m_port;
}

std::shared_ptr<mqtt> mqtt::instance(const mqtt_address &addr, const std::optional<mqtt_credentials> &credentials) {
    std::lock_guard<std::recursive_mutex> _instance_guard{_instance_mutex};

    if (_global_context.stopped()) {
        logger::instance()->critical("The global context is already stopped");
        return nullptr;
    }

    if (_instances.count(addr) != 0) {
        return _instances[addr];
    }

    mqtt_sock_type mqtt_socket =
        mqtt_cpp::make_sync_client(mqtt::_global_context, addr.m_server_name, (uint16_t)addr.m_port);
    mqtt_socket->set_client_id("QuariumController");
    mqtt_socket->set_clean_session(true);

    if (credentials) {
        mqtt_socket->set_user_name(credentials->m_username);
        mqtt_socket->set_password(credentials->m_password);
    }

    auto connected_sucessfully = std::make_shared<std::atomic_bool>(false);

    mqtt_socket->set_error_handler(
        [](mqtt_cpp::error_code ec) { logger::instance()->warn("New Mqtt error : {}", ec.message()); });

    auto handler = [connected_sucessfully](bool, mqtt_cpp::connect_return_code connack_return_code) -> bool {
        logger::instance()->info("Mqtt connack handler called, return code : {}",
                                 mqtt_cpp::connect_return_code_to_str(connack_return_code));
        *connected_sucessfully = connack_return_code == mqtt_cpp::connect_return_code::accepted;
        return true;
    };

    mqtt_socket->set_connack_handler(handler);

    try {
        mqtt_socket->connect();
    } catch (...) {
        logger::instance()->critical("Couldn't create a mqtt client with the target broker {}:{}", addr.m_server_name,
                                     (uint16_t)addr.m_port);
        return nullptr;
    }

    // TODO: improve this, e.g run as long as the handler wasn't called, not just for 5 seconds even if it was already
    // called. Try running  the context for 5 seconds -> wait 5 seconds for the flag to be set
    _global_context.run_for(std::chrono::seconds(5));

    if (!connected_sucessfully->load()) {
        return nullptr;
    }

    auto created_instance = _instances.emplace(std::make_pair(addr, new mqtt(addr, mqtt_socket, credentials)));

    if (!created_instance.second) {
        return nullptr;
    }

    created_instance.first->second->start_interface();
    return created_instance.first->second;
}

mqtt::mqtt(mqtt_address addr, mqtt_sock_type mqtt_socket, std::optional<mqtt_credentials> credentials)
    : m_addr(std::move(addr)), m_mqtt_socket(mqtt_socket), m_cred(credentials) {}

// TODO: maybe do this differently
bool mqtt::start_interface() {
    std::lock_guard<std::recursive_mutex> _instance_guard{_instance_mutex};

    if (_is_running) {
        return true;
    }
    logger::instance()->info("Starting mqtt interfaces");

    std::thread created_thread(mqtt::io_handler);
    _io_thread.swap(created_thread);

    return true;
}

bool mqtt::stop_all_instances() {
    std::lock_guard<std::recursive_mutex> _instance_guard{_instance_mutex};
    auto logger_instance = logger::instance();

    _should_stop.exchange(true);
    if (!_global_context.stopped()) {
        _global_context.stop();
    }
    logger_instance->info("Notified mqtt io thread to stop");

    if (_io_thread.joinable()) {
        _io_thread.join();
    }

    logger_instance->info("Stopped all mqtt interfaces");

    return true;
}

void mqtt::io_handler() {
    using namespace std::chrono_literals;

    _is_running = true;
    logger::instance()->info("Entering mqtt io thread");
    while (!_should_stop.load()) {
        try {
            _global_context.run();
        } catch (...) {
            logger::instance()->warn("An exception happened in the mqtt io handler");
        }
    }

    logger::instance()->info("Leaving mqtt io thread");
    _is_running = false;
}
