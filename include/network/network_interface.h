#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

#include "httplib/httplib.h"

enum port : uint16_t {};

class network_interface final {
   public:
    static std::optional<network_interface> create_on_port(port p);
    network_interface(const network_interface &) = delete;
    network_interface(network_interface &&) = default;
    ~network_interface();

    bool start();
    void stop();

    explicit operator bool() const;
    network_interface &operator=(const network_interface &other) = delete;
    network_interface &operator=(network_interface &&other);

    void swap(network_interface &other);

    template<typename F>
    void add_route(const char *path, F function);

   private:
    network_interface(std::unique_ptr<httplib::Server> server_instance);

    std::thread m_listen_thread;
    std::unique_ptr<httplib::Server> m_server_instance = nullptr;
};

void swap(network_interface &lhs, network_interface &rhs);

template<typename F>
void network_interface::network_interface::add_route(const char *path, F function) {
    m_server_instance->Get(path, function);
}
