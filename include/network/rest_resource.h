#pragma once

#include <atomic>
#include <optional>
#include <utility>
#include <variant>

#include "nlohmann/json.hpp"

#include "network/network_interface.h"

// TODO Test mechanism
template<typename T>
class rest_resource_id final {
   public:
    rest_resource_id();
    rest_resource_id(const rest_resource_id &other);
    rest_resource_id(rest_resource_id &&other) = default;
    ~rest_resource_id() = default;

    rest_resource_id &operator=(const rest_resource_id &other);
    rest_resource_id &operator=(rest_resource_id &&other) = default;

    uint32_t as_number() const;

   private:
    static uint32_t next_id();

    static inline std::atomic<uint32_t> _next_id;

    uint32_t m_id;
};

template<typename T>
rest_resource_id<T>::rest_resource_id() : m_id(next_id()) {}

template<typename T>
rest_resource_id<T>::rest_resource_id(const rest_resource_id &other) : m_id(next_id()) {}

template<typename T>
rest_resource_id<T> &rest_resource_id<T>::operator=(const rest_resource_id &other) {
    m_id = next_id();

    return *this;
}

template<typename T>
uint32_t rest_resource_id<T>::as_number() const {
    return m_id;
}

template<typename T>
uint32_t rest_resource_id<T>::next_id() {
    return _next_id.fetch_add(1);
}

enum struct rest_resource_types { list, entry };

template<typename T, rest_resource_types ResourceType>
class rest_resource {
   public:
    rest_resource() = default;
    nlohmann::json serialize() const;
    const rest_resource_id<T> &id() const;
    static std::optional<T> deserialize(nlohmann::json &description);
    static http::response<http::dynamic_body> handle_request(const http::request<http::dynamic_body> &request);

   private:
    rest_resource_id<T> m_id{};
};

template<typename T>
class rest_resource<T, rest_resource_types::entry> {
   public:
    rest_resource() = default;
    nlohmann::json serialize() const;
    void update(nlohmann::json serialized);

    const rest_resource_id<T> &id() const;
    static std::optional<T> deserialize(nlohmann::json &description);
    static http::response<http::dynamic_body> handle_request(const http::request<http::dynamic_body> &request);

   private:
    rest_resource_id<T> m_id{};
};

template<typename T>
class rest_resource<T, rest_resource_types::list> {
   public:
    rest_resource() = default;
    nlohmann::json serialize() const;
    void update(nlohmann::json serialized);

    const rest_resource_id<T> &id() const;
    static std::optional<T> deserialize(nlohmann::json &description);
    static http::response<http::dynamic_body> handle_request(const http::request<http::dynamic_body> &request);

   private:
    rest_resource_id<T> m_id{};
};

template<typename T>
nlohmann::json rest_resource<T, rest_resource_types::entry>::serialize() const {
    return std::move(static_cast<const T *>(this)->serialize());
}

template<typename T>
const rest_resource_id<T> &rest_resource<T, rest_resource_types::entry>::id() const {
    return m_id;
}

template<typename T>
std::optional<T> rest_resource<T, rest_resource_types::entry>::deserialize(nlohmann::json &description) {
    return std::move(T::deserialize(description));
}

template<typename T>
http::response<http::dynamic_body> rest_resource<T, rest_resource_types::entry>::handle_request(
    const http::request<http::dynamic_body> &request) {
    return std::move(T::handle_request(request));
}

template<typename T>
class rest_resource_description;

template<typename T>
class rest_resource_node {
   public:
    static constexpr inline rest_resource_types resource_type = rest_resource_description<T>::resource_type;
    using resource = T;

    // TODO add static_assert to check if all classes are rest_resources
    template<typename... Args>
    rest_resource_node(Args... args);

   private:
    std::vector<T> m_entry;
};

template<typename... RestTypeList>
class rest_resource_layout {
   public:
    using variant_type = std::variant<std::monostate, rest_resource_node<RestTypeList>...>;

    template<typename... Args>
    rest_resource_layout(Args... args);

   private:
    std::vector<variant_type> m_top_entries;
};

/**
 * rest_resource_layout<webapp, schedules, gpio>(rest_resource_node<gpio, rest_resource_types::list>("/api/v0/gpio",
 rest_list_interface<gpio>,
 * rest_resource_node<gpios, rest_resource_types::list>));

 */
