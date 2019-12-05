#pragma once

#include <atomic>
#include <optional>
#include <utility>

#include "network/network_interface.h"
#include "nlohmann/json.hpp"

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

template<typename T>
class rest_resource {
   public:
    rest_resource() = default;
    nlohmann::json serialize() const;
    const rest_resource_id<T> &id() const;
    static std::optional<T> deserialize(nlohmann::json &description);
    static void handle_request(const httplib::Request &request, httplib::Response &response);

   private:
    rest_resource_id<T> m_id{};
};

template<typename T>
nlohmann::json rest_resource<T>::serialize() const {
    return std::move(static_cast<const T *>(this)->serialize());
}

template<typename T>
const rest_resource_id<T> &rest_resource<T>::id() const {
    return m_id;
}

template<typename T>
std::optional<T> rest_resource<T>::deserialize(nlohmann::json &description) {
    return std::move(T::deserialize(description));
}

template<typename T>
void rest_resource<T>::handle_request(const httplib::Request &request, httplib::Response &response) {
    return T::handle_request(request, response);
}
