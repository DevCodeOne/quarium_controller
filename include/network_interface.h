#pragma once

#include <cstdint>

#include "network_header.h"

#include <memory>
#include <optional>

enum port : uint16_t {};

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
    Pistache::Rest::Router m_router;
    bool m_stopped = false;
    bool m_is_initialized = false;
};

void swap(network_interface &lhs, network_interface &rhs);
