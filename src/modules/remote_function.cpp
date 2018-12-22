#include "boost/asio/connect.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/http.hpp"
#include "boost/beast/version.hpp"

#include "logger.h"
#include "modules/remote_function.h"
#include "utils.h"

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

bool remote_function::update_values() {
    boost::asio::io_context context;

    boost::asio::ip::tcp::resolver resolver{context};
    boost::asio::ip::tcp::socket socket{context};

    auto url_entry = description().lookup_value("url");

    if (!url_entry.has_value()) {
        return false;
    }

    auto url = (*url_entry).value<std::string>();
    auto port_start = url.find_last_of(":");
    auto port_end = url.find_first_not_of("0123456789", port_start);
    auto host = url.substr(0, port_start - 1);
    std::string port = port_start != std::string::npos ? url.substr(port_start + 1, port_end - 1).substr() : "80";

    try {
        auto const results = resolver.resolve(host, port);

        logger::instance()->info("Connection to {}:{}/{}", host, port, url.substr(port_end + 1));

        // boost::asio::connect(socket, results.begin(), results.end());

        // http::request<http::empty_body> req(http::verb::get, url.substr(port_end), 10);
        // req.set(http::field::host, host);

        // http::write(socket, req);

        // boost::beast::error_code ec;
        // socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        // if (ec && ec != boost::beast::errc::not_connected) {
        //     return false;
        // }
    } catch (...) {
        return false;
    }

    return true;
}
