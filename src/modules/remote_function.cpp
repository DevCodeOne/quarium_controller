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

    auto url_entry = description().lookup_other_value("url");

    if (!url_entry.has_value() || !url_entry->is_string()) {
        return false;
    }

    auto url = (*url_entry).get<std::string>();
    // Check if valid url
    auto port_start = url.find_last_of(":");
    auto port_end = url.find_first_not_of("0123456789", port_start + 1);

    std::string host = url.substr(0, port_start);
    std::string port = "80";
    std::string function = "";

    if (port_start != std::string::npos) {
        port = url.substr(port_start + 1, port_end - (port_start + 1));
        function = url.substr(port_end);
    }

    try {
        auto const results = resolver.resolve(host, port);

        std::ostringstream remote_function;

        remote_function << function;
        for (auto &[id, value] : description().values()) {
            if (value.what_type() == module_value_type::integer) {
                remote_function << "?" << id << "=" << value.value<int>();
            } else {
                logger::instance()->info("Values of other type than int are not supported right now");
            }
        }

        boost::asio::connect(socket, results.begin(), results.end());

        http::request<http::empty_body> req(http::verb::get, remote_function.str(), 10);
        req.set(http::field::host, "localhost");

        http::write(socket, req);
        logger::instance()->info("{}", req.target().data());

        boost::beast::error_code ec;
        socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        if (ec && ec != boost::beast::errc::not_connected) {
            logger::instance()->info("An error occured when trying to connect to the module : {}", id());
            return false;
        }
    } catch (std::exception e) {
        logger::instance()->info("{}", e.what());
        return false;
    } catch (...) {
        logger::instance()->info("An exception occured when trying to connect to the module", id());
        return false;
    }

    return true;
}
