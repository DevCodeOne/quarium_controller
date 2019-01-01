#include <string>

#include "boost/asio/connect.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/version.hpp"

#include "io/remote_function/remote_function.h"
#include "logger.h"

std::unique_ptr<output_interface> remote_function::create_for_interface(const nlohmann::json &description_parameter) {
    nlohmann::json description = description_parameter;
    if (!description.is_object()) {
        return nullptr;
    }

    auto url_entry = description["url"];

    if (url_entry.is_null() || !url_entry.is_string()) {
        logger::instance()->info("The url_entry of one output is invalid");
        return {};
    }

    auto value_entry = description["value"];

    if (value_entry.is_null() || !value_entry.is_object()) {
        logger::instance()->info("The value_entry of one output is invalid");
        return {};
    }

    auto value_id_entry = value_entry["name"];

    if (value_id_entry.is_null() || !value_id_entry.is_string()) {
        logger::instance()->info("The value_id of a value of one output is invalid");
        return {};
    }

    auto created_output_value = output_value::deserialize(value_entry["description"]);

    if (!created_output_value.has_value()) {
        logger::instance()->info("The parsed output_value description was invalid");
        return {};
    }

    return std::unique_ptr<output_interface>(
        new remote_function(url_entry.get<std::string>(), value_id_entry.get<std::string>(), *created_output_value));
}

remote_function::remote_function(const std::string &url, const std::string &value_id, const output_value &value)
    : m_url(url), m_value_id(value_id), m_value(value) {}

bool remote_function::remote_function::control_output(const output_value &value) {
    m_value = value;
    return update_values();
}

bool remote_function::override_with(const output_value &value) {
    m_overriden_value = value;
    return update_values();
}

bool remote_function::restore_control() {
    m_overriden_value = {};
    return update_values();
}

std::optional<output_value> remote_function::is_overriden() const {
    if (!m_overriden_value.has_value()) {
        return {};
    }
    return m_overriden_value.value();
}

output_value remote_function::current_state() const {
    if (m_overriden_value.has_value()) {
        return *m_overriden_value;
    }

    return m_value;
}

bool remote_function::update_values() {
    boost::asio::io_context context;

    boost::asio::ip::tcp::resolver resolver{context};
    boost::asio::ip::tcp::socket socket{context};

    // Check if it is a valid url
    auto port_start = m_url.find_last_of(":");
    auto port_end = m_url.find_first_not_of("0123456789", port_start + 1);

    std::string host = m_url.substr(0, port_start);
    std::string port = "80";
    std::string function = "";

    if (port_start != std::string::npos) {
        port = m_url.substr(port_start + 1, port_end - (port_start + 1));
        function = m_url.substr(port_end);
    }

    try {
        auto const results = resolver.resolve(host, port);

        std::ostringstream remote_function;

        remote_function << function;
        auto value = current_state();
        if (value.holds_type<int>()) {
            remote_function << "?" << m_value_id << "=" << *value.get<int>();
        } else {
            logger::instance()->info("Values with a type different than int are not supported right now");
        }

        boost::asio::connect(socket, results.begin(), results.end());

        http::request<http::empty_body> req(http::verb::get, remote_function.str(), 10);
        req.set(http::field::host, "localhost");

        http::write(socket, req);
        logger::instance()->info("{}", req.target().data());

        boost::beast::error_code ec;
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::beast::errc::not_connected) {
            logger::instance()->info("An error occured when trying to connect to : {}", m_url);
            return false;
        }
    } catch (std::exception e) {
        logger::instance()->info("{}", e.what());
        return false;
    } catch (...) {
        logger::instance()->info("An exception occured when trying to connect to : {}", m_url);
        return false;
    }

    return true;
}
