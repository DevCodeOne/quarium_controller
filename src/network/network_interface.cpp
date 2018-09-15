#include "network/network_interface.h"
#include "logger.h"
#include "network/web_application.h"

std::optional<network_interface> network_interface::create_on_port(port p) { return network_interface(p); }

network_interface::network_interface(port p) {
    Pistache::Address address(Pistache::Ipv4::any(), Pistache::Port(p));
    auto options = Pistache::Http::Endpoint::options().threads(1);
    try {
        m_server = std::make_unique<Pistache::Http::Endpoint>(address);
        setup_routes();
        m_server->setHandler(Pistache::Http::make_handler<router>(m_router));
        m_server->init(options);
        logger::instance()->info("Created server");
    } catch (...) {
        logger::instance()->warn("Couldn't create server");
    }
}

network_interface::network_interface(network_interface &&other)
    : m_server(std::move(other.m_server)), m_stopped(other.m_stopped) {
    other.m_server = nullptr;
    other.m_stopped = true;
}

network_interface::~network_interface() { stop(); }

bool network_interface::start() {
    if (!m_server) {
        logger::instance()->critical("Couldn't create server");
        return false;
    }

    if (m_server->isBound()) {
        logger::instance()->critical("Couldn't bind port");
        return false;
    }

    try {
        m_server->serveThreaded();
    } catch (...) {
        logger::instance()->critical("Couldn't start serve thread");
        return false;
    }

    logger::instance()->info("Started server");
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
    m_server = nullptr;
    m_stopped = true;

    return true;
}

void network_interface::setup_routes() {
    m_router.add_route(std::regex("/api/v0/log", std::regex_constants::basic), logger::handle);
    m_router.add_route(std::regex("/webapp.*", std::regex_constants::basic), web_application::handle);
}

network_interface::operator bool() const {
    if (!m_server) {
        return false;
    }

    return m_server->isBound();
}

void router::add_route(const std::regex &regex_path, route_func func) {
    m_routes.emplace_back(std::make_pair(regex_path, func));
}

void router::onRequest(const Pistache::Http::Request &req, Pistache::Http::ResponseWriter response) {
    logger::instance()->info("number of routes {}", m_routes.size());
    auto route = std::find_if(m_routes.begin(), m_routes.end(), [this, &req](auto &route_handler_pair) {
        logger::instance()->info("{}", std::regex_match(req.resource(), route_handler_pair.first));
        return std::regex_match(req.resource(), route_handler_pair.first);
    });

    if (route != m_routes.cend()) {
        route->second(req, std::move(response));
    } else {
        response.send(Pistache::Http::Code::Not_Found, "Wrong Route");
    }
}

void router::onTimeout(const Pistache::Http::Request &req, Pistache::Http::ResponseWriter response) {
    response.send(Pistache::Http::Code::Request_Timeout, "Timeout");
}

std::shared_ptr<Pistache::Tcp::Handler> router::clone() const { return std::make_shared<router>(*this); }
