#pragma once

#include <cstdint>

#include "network_header.h"

#include <memory>
#include <optional>
#include <regex>
#include <vector>

enum port : uint16_t {};

class router final : public Pistache::Http::Handler {
   public:
    using route_func = std::function<void(const Pistache::Http::Request &, Pistache::Http::ResponseWriter)>;

    router() = default;
    router(const router &other) = default;
    router(router &&other) = default;
    ~router() = default;

    // Don't use the preprocessor for such unnecessary things
    typedef Pistache::Http::details::prototype_tag tag;
    std::shared_ptr<Pistache::Tcp::Handler> clone() const override;

    void onRequest(const Pistache::Http::Request &req, Pistache::Http::ResponseWriter response) override;
    void onTimeout(const Pistache::Http::Request &req, Pistache::Http::ResponseWriter response) override;

    void add_route(const std::regex &path, route_func func);

   private:
    std::vector<std::pair<std::regex, route_func>> m_routes;
};

class network_interface final {
   public:
    static std::optional<network_interface> create_on_port(port p);
    network_interface(const network_interface &) = delete;
    network_interface(network_interface &&);
    ~network_interface();

    bool start();
    bool stop();

    explicit operator bool() const;
    network_interface &operator=(const network_interface &other) = delete;
    network_interface &operator=(network_interface &&other);

    void swap(network_interface &other);

   private:
    network_interface(port p);

    void setup_routes();

    std::unique_ptr<Pistache::Http::Endpoint> m_server = nullptr;
    router m_router;
    bool m_stopped = false;
    bool m_is_initialized = false;
};

void swap(network_interface &lhs, network_interface &rhs);
