#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <utility>

#include "mqtt_client_cpp.hpp"

enum struct mqtt_port : uint16_t {};

struct mqtt_address {
    static inline constexpr size_t max_server_name_len = 64;

    const char m_server_name[max_server_name_len];
    mqtt_port m_port;
};

namespace detail {
    struct mqtt_address_cmp {
        bool operator()(const mqtt_address &lhs, const mqtt_address &rhs) const;
    };
}  // namespace detail

class mqtt {
   public:
    static inline constexpr mqtt_address _default_adress{"localhost", mqtt_port{1234}};

    static std::shared_ptr<mqtt> instance(const mqtt_address &addr = _default_adress);
    static bool stop_all_instances();

    mqtt(mqtt &&other) = default;
    mqtt(const mqtt &other) = delete;
    ~mqtt() = default;

    mqtt &operator=(const mqtt &other) = delete;
    // TODO: maybe add this back later
    mqtt &operator=(mqtt &&other) = delete;

   private:
    static inline boost::asio::io_context _global_context;
    using mqtt_sock_type =
        decltype(mqtt_cpp::make_async_client(_global_context, std::declval<std::string>(), std::declval<uint16_t>()));

    mqtt(mqtt_address addr, mqtt_sock_type mqtt_socket);

    static inline std::map<mqtt_address, std::shared_ptr<mqtt>, detail::mqtt_address_cmp> _instances;
    static inline std::mutex _instance_mutex;
    mqtt_sock_type m_mqtt_socket;
    mqtt_address m_addr;
};
