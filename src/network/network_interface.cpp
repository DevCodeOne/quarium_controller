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

//
// void router::handle_request() {
//     auto self = shared_from_this();
//
//     boost::asio::basic_waitable_timer<std::chrono::steady_clock> timeout_timer(m_socket.get_executor().context(),
//                                                                                std::chrono::seconds(10));
//     timeout_timer.async_wait([self](boost::beast::error_code code) { self->m_socket.close(code); });
//
//     boost::beast::error_code code;
//     http::read(m_socket, m_buffer, m_request, code);
//
//     if (code) {
//         logger::instance()->critical("An error occured when trying to read data from the socket : {}",
//         code.message());
//     }
//
//     std::string target_as_string = m_request.target().to_string();
//
//     std::lock_guard<std::mutex> instance_guard(_instance_mutex);
//     auto route = std::find_if(_routes.begin(), _routes.end(), [target_as_string](auto &route_handler) {
//         return std::regex_match(target_as_string, route_handler.first);
//     });
//
//     http::response<http::dynamic_body> response;
//
//     logger::instance()->info("Requesting resource {}", target_as_string);
//
//     if (route == _routes.cend()) {
//         response.version(m_request.version());
//         response.result(http::status::not_found);
//         response.set(http::field::server, "Beast");
//
//         boost::beast::ostream(response.body()) << "Route not found";
//
//     } else {
//         response = route->second(m_request);
//     }
//
//     response.set(http::field::content_length, response.body().size());
//     http::write(m_socket, response, code);
//
//     if (code) {
//         logger::instance()->critical("An error occured when trying to send data to the socket : {}", code.message());
//     }
//
//     m_socket.shutdown(tcp::socket::shutdown_send, code);
//
//     if (code) {
//         logger::instance()->critical("An error occured when trying to close the socket : {}", code.message());
//     }
//
//     timeout_timer.cancel();
// }
