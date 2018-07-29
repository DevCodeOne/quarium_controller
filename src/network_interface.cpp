#include "network_interface.h"
#include "logger.h"

std::optional<network_interface> network_interface::create_on_port(port p) {
    return network_interface(p);
}

network_interface::network_interface(port p) {
    Pistache::Address address(Pistache::Ipv4::any(), Pistache::Port(p));
    auto options = Pistache::Http::Endpoint::options().threads(1);
    m_server = std::make_unique<Pistache::Http::Endpoint>(address);
    m_server->setHandler(
        Pistache::Http::make_handler<detail::network_interface_handler>());
    m_server->init(options);
}

network_interface::network_interface(network_interface &&other)
    : m_server(std::move(other.m_server)), m_stopped(other.m_stopped) {
    other.m_server = nullptr;
    other.m_stopped = true;
}

network_interface::~network_interface() { stop(); }

bool network_interface::start() {
    if (!m_server) {
        return false;
    }

    if (m_server->isBound()) {
        return false;
    }

    m_server->serveThreaded();
    return true;
}

bool network_interface::stop() {
    if (!m_server) {
        return false;
    }

    if (m_stopped || !m_server->isBound()) {
        return false;
    }

    m_server->shutdown();
    m_stopped = true;

    return true;
}

network_interface::operator bool() const {
    if (!m_server) {
        return false;
    }

    return m_server->isBound();
}

void detail::network_interface_handler::onRequest(
    const Pistache::Http::Request &request,
    Pistache::Http::ResponseWriter response) {
    response.send(Pistache::Http::Code::Ok, "Hello World");
    logger::instance()->info("Resource {}", request.resource());
}
