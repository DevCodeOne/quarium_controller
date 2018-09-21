#include <functional>

#include "logger.h"
#include "network/network_interface.h"
#include "network/web_application.h"

std::optional<network_interface> network_interface::create_on_port(port p) {
    try {
        auto io_context = std::make_unique<boost::asio::io_context>(1);
        auto connection_acceptor = std::unique_ptr<boost::asio::ip::tcp::acceptor>(new boost::asio::ip::tcp::acceptor{
            *io_context.get(), {boost::asio::ip::make_address("0.0.0.0"), (uint16_t)p}});
        auto socket = std::make_unique<boost::asio::ip::tcp::socket>(*io_context.get());

        return network_interface(std::move(io_context), std::move(connection_acceptor), std::move(socket));
    } catch (std::exception e) {
        return {};
        logger::instance()->warn("Couldn't create server");
    }

    return {};
}

network_interface::network_interface(std::unique_ptr<boost::asio::io_context> io_context,
                                     std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor,
                                     std::unique_ptr<boost::asio::ip::tcp::socket> socket)
    : m_io_context(std::move(io_context)), m_acceptor(std::move(acceptor)), m_socket(std::move(socket)) {
    setup_routes();
}

network_interface::network_interface(network_interface &&other)
    : m_io_context(std::move(other.m_io_context)),
      m_acceptor(std::move(other.m_acceptor)),
      m_socket(std::move(other.m_socket)),
      m_stop(std::move(other.m_stop)) {}

network_interface::~network_interface() { stop(); }

// TODO prevent double start
bool network_interface::start() {
    if (!m_io_context) {
        logger::instance()->critical("Io context is not valid");
        return false;
    }

    if (!m_acceptor->is_open()) {
        logger::instance()->critical("Acceptor isn't open");
        return false;
    }

    logger::instance()->info("Started server");

    run_server();
    m_io_context_thread = std::thread([this]() { m_io_context->run(); });
    return true;
}

void network_interface::stop() {
    if (m_stop) {
        *m_stop = true;
    }

    if (m_acceptor) {
        m_acceptor->cancel();
    }

    if (m_io_context_thread.joinable()) {
        m_io_context_thread.join();
    }
}

void network_interface::setup_routes() {
    router::add_route(std::regex("/api/v0/log", std::regex_constants::basic), logger::handle_request);
    router::add_route(std::regex("/webapp.*", std::regex_constants::basic), web_application::handle_request);
}

network_interface::operator bool() const { return m_socket->is_open() && m_acceptor->is_open(); }

void network_interface::run_server() {
    m_acceptor->async_accept(*m_socket.get(), [this](boost::beast::error_code ec) {
        if (m_stop && !(*m_stop)) {
            std::make_shared<router>(std::move(*m_socket.get()))->handle_request();
            this->run_server();
        }
    });
}

router::router(boost::asio::ip::tcp::socket socket) : m_socket(std::move(socket)) {}

void router::add_route(const std::regex &regex_path, route_func func) {
    _routes.emplace_back(std::make_pair(regex_path, func));
}

void router::handle_request() {
    auto self = shared_from_this();
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> timeout_timer(m_socket.get_executor().context(),
                                                                               std::chrono::seconds(30));
    timeout_timer.async_wait([self](boost::beast::error_code code) { self->m_socket.close(code); });

    http::read(m_socket, m_buffer, m_request);
    std::lock_guard<std::mutex> instance_guard(_instance_mutex);

    std::string target_as_string = m_request.target().to_string();

    auto route = std::find_if(_routes.begin(), _routes.end(), [target_as_string](auto &route_handler) {
        return std::regex_match(target_as_string, route_handler.first);
    });

    http::response<http::dynamic_body> response;

    logger::instance()->info("Requesting resource {}", target_as_string);

    if (route == _routes.cend()) {
        response.version(m_request.version());
        response.result(http::status::not_found);
        response.set(http::field::server, "Beast");

        boost::beast::ostream(response.body()) << "Route not found";

    } else {
        response = route->second(m_request);
    }

    response.set(http::field::content_length, response.body().size());
    http::write(m_socket, response);

    m_socket.shutdown(tcp::socket::shutdown_send);
    timeout_timer.cancel();
}
