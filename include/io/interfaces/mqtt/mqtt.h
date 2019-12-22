#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <utility>

#include "logger.h"
#include "mqtt_client_cpp.hpp"
#include "pattern_templates/static_destructor.h"

enum struct mqtt_port : uint16_t {};

struct mqtt_address {
    static inline constexpr size_t max_server_name_len = 64;

    char m_server_name[max_server_name_len];
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
    static bool start_interface();
    static bool stop_all_instances();

    mqtt(mqtt &&other) = default;
    mqtt(const mqtt &other) = delete;
    ~mqtt() = default;

    mqtt &operator=(const mqtt &other) = delete;
    // TODO: maybe add this back later
    mqtt &operator=(mqtt &&other) = delete;

    // TODO: add library independent qos parameter
    template<typename T>
    bool publish(std::filesystem::path topic, const T &value, mqtt_cpp::qos qos = mqtt_cpp::qos::at_least_once);

   private:
    static inline boost::asio::io_context _global_context;
    static inline std::thread _io_thread;
    static inline std::atomic_bool _is_running{false};
    static inline std::atomic_bool _should_stop{false};
    using mqtt_sock_type =
        decltype(mqtt_cpp::make_sync_client(_global_context, std::declval<std::string>(), std::declval<uint16_t>()));
    using packet_id = mqtt_sock_type::element_type::packet_id_t;

    mqtt(mqtt_address addr, mqtt_sock_type mqtt_socket);
    static void io_handler();

    static inline std::map<mqtt_address, std::shared_ptr<mqtt>, detail::mqtt_address_cmp> _instances;
    static inline std::recursive_mutex _instance_mutex;
    static inline static_desctructor<mqtt, void (*)()> _destructor{+[]() { stop_all_instances(); }};
    mqtt_sock_type m_mqtt_socket;
    mqtt_address m_addr;
};

template<typename T>
bool mqtt::publish(std::filesystem::path topic, const T &value, mqtt_cpp::qos qos) {
    return m_mqtt_socket->publish(topic.c_str(), value, qos);
}
