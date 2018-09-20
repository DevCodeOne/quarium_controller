#pragma once

#include <cstdint>

#include <memory>
#include <mutex>
#include <optional>
#include <regex>
#include <thread>
#include <vector>

#include "boost/asio.hpp"
#include "boost/beast.hpp"

enum port : uint16_t {};

namespace http = boost::beast::http;
using tcp = boost::asio::ip::tcp;

class router final : public std::enable_shared_from_this<router> {
   public:
    using route_func = std::function<http::response<http::dynamic_body>(const http::request<http::dynamic_body> &)>;

    router(boost::asio::ip::tcp::socket socket);
    router(const router &other) = default;
    router(router &&other) = default;
    ~router() = default;

    static void add_route(const std::regex &path, route_func func);

    void handle_request();

   private:
    boost::asio::ip::tcp::socket m_socket;
    boost::beast::flat_buffer m_buffer{8192};
    http::request<http::dynamic_body> m_request;
    http::response<http::dynamic_body> m_response;

    static inline std::vector<std::pair<std::regex, route_func>> _routes;
    static inline std::mutex _instance_mutex;
};

class network_interface final {
   public:
    static std::optional<network_interface> create_on_port(port p);
    network_interface(const network_interface &) = delete;
    network_interface(network_interface &&);
    ~network_interface();

    bool start();
    void stop();

    explicit operator bool() const;
    network_interface &operator=(const network_interface &other) = delete;
    network_interface &operator=(network_interface &&other);

    void swap(network_interface &other);

   private:
    network_interface(std::unique_ptr<boost::asio::io_context> io_context,
                      std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor,
                      std::unique_ptr<boost::asio::ip::tcp::socket> socket);
    void setup_routes();
    void run_server();

    std::unique_ptr<boost::asio::io_context> m_io_context = nullptr;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> m_acceptor = nullptr;
    std::unique_ptr<boost::asio::ip::tcp::socket> m_socket = nullptr;
    std::unique_ptr<volatile bool> m_stop = std::make_unique<volatile bool>();
    std::thread m_io_context_thread;
};

void swap(network_interface &lhs, network_interface &rhs);
