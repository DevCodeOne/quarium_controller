#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include <cstdint>

#include <memory>
#include <optional>

#include "pistache/endpoint.h"

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

    std::unique_ptr<Pistache::Http::Endpoint> m_server = nullptr;
    bool m_stopped = false;
    bool m_is_initialized = false;
};

void swap(network_interface &lhs, network_interface &rhs);

namespace detail {
class network_interface_handler final : public Pistache::Http::Handler {
   public:
    HTTP_PROTOTYPE(network_interface_handler)
    void onRequest(const Pistache::Http::Request &request,
                   Pistache::Http::ResponseWriter response);
};
}  // namespace detail

#pragma GCC diagnostic pop
