#include "network/network_interface.h"

#include <functional>

#include "logger.h"
#include "schedule/schedule_handler.h"

std::optional<network_interface> network_interface::create_on_port(port p) {
    using namespace httplib;
    auto server_instance = std::make_unique<Server>();
    server_instance->bind_to_port("localhost", p);

    if (!server_instance->is_valid()) {
        logger::instance()->critical("Couldn't start server");
        return std::nullopt;
    }

    return network_interface(std::move(server_instance));
}

network_interface::network_interface(std::unique_ptr<httplib::Server> server_instance)
    : m_server_instance(std::move(server_instance)) {}

network_interface::~network_interface() { stop(); }

// TODO: prevent double start
bool network_interface::start() {
    logger::instance()->info("Starting up server");
    m_listen_thread = std::thread([this]() { m_server_instance->listen_after_bind(); });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!m_server_instance->is_valid() || !m_server_instance->is_running()) {
        logger::instance()->warn("Couldn't start up server");
        return false;
    }
    return true;
}

void network_interface::stop() {
    if (!m_server_instance) {
        return;
    }

    m_server_instance->stop();

    if (m_listen_thread.joinable()) {
        m_listen_thread.join();
    }
}

network_interface::operator bool() const { return m_server_instance->is_valid() && m_server_instance->is_running(); }
