#pragma once

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <utility>
#include <variant>

#include "nlohmann/json.hpp"

#include "network/network_interface.h"

// TODO Test mechanism
// TODO maybe remove template definitions from the inside of the classes
template<typename T>
class rest_resource_id final {
   public:
    rest_resource_id() : m_id(next_id()) {}
    rest_resource_id(const rest_resource_id &other) : m_id(next_id()) {}
    rest_resource_id(rest_resource_id &&other) = default;
    ~rest_resource_id() = default;

    rest_resource_id &operator=(const rest_resource_id &other) {
        m_id = next_id();
        return *this;
    }

    rest_resource_id &operator=(rest_resource_id &&other) = default;

    uint32_t as_number() const { return m_id; }

   private:
    static uint32_t next_id() { return _next_id.fetch_add(1); }

    static inline std::atomic<uint32_t> _next_id;

    uint32_t m_id;
};

enum struct rest_resource_types { list, entry };

template<typename T>
struct rest_resource_description;

template<typename T, rest_resource_types ResourceType>
class rest_resource;

// TODO rest resources should be copies without system resources with no guarantee of actually existing anymore
template<typename T>
class rest_resource<T, rest_resource_types::list> {
   public:
    using resource_description = rest_resource_description<T>;
    using value_type = typename resource_description::value;

    rest_resource() = default;
    nlohmann::json serialize() const;
    const rest_resource_id<T> &id() const;
    static std::optional<T> deserialize(nlohmann::json &description);

    // Retrieve specific entry of the list
    // TODO use pointer
    value_type get(const rest_resource_id<value_type> &resource_id);
    // Remove specific entry in the list
    void remove(const rest_resource_id<value_type> &resource_id);
    // Update specific entry in the list
    void update(nlohmann::json serialized);
    // Create entry in the list
    value_type create(nlohmann::json serialized);
    // Create entry in the list or update specific entry
    value_type put(nlohmann::json serialized);

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
class rest_resource_node {
   public:
    static constexpr inline rest_resource_types resource_type = rest_resource_description<T>::resource_type;
    using resource = T;

    // TODO add static_assert to check if the class is a rest_resources
    // and add runtime check if any regexpressions collide with each other
    inline rest_resource_node(std::regex path_regex) : m_path_regex(path_regex) {}

    const std::regex &path_regex() const { return m_path_regex; }

   private:
    const std::regex m_path_regex;
};

template<typename... RestTypeList>
class rest_resource_layout {
   public:
    using variant_type = std::variant<rest_resource_node<RestTypeList>...>;

    // TODO check if all required types are listed in RestTypeList with static_assert
    template<typename... Args>
    inline static void create_layout(Args... args) {
        std::lock_guard<std::mutex> _instance_guard(_instance_mutex);
        _instances.emplace_back(new rest_resource_layout{args...});
    }

    static http::response<http::dynamic_body> handle_request(const http::request<http::dynamic_body> &request);

   private:
    inline rest_resource_layout(std::initializer_list<variant_type> arguments) : m_top_entries(arguments) {}

    template<typename T>
    inline static typename rest_resource_description<T>::value &do_iteration() {}

    std::vector<variant_type> m_top_entries;

    static inline std::mutex _instance_mutex;
    static inline std::vector<std::unique_ptr<rest_resource_layout<RestTypeList...>>> _instances;
};

template<typename... RestTypeList>
http::response<http::dynamic_body> rest_resource_layout<RestTypeList...>::handle_request(
    const http::request<http::dynamic_body> &request) {
    std::lock_guard<std::mutex> _instance_guard{_instance_mutex};

    http::response<http::dynamic_body> response;
    std::string target_as_string = request.target().to_string();

    for (const auto &current_layout : _instances) {
        // TODO add private observer for top_entries
        for (auto &current_entry : current_layout->m_top_entries) {
            std::visit(
                [&response, target_as_string](auto &resource) {
                    if (std::regex_match(target_as_string, resource.path_regex())) {
                        if constexpr (resource.resource_type == rest_resource_types::list) {
                            boost::beast::ostream(response.body()) << target_as_string << "is a list";
                        }
                        return;
                    }

                    std::smatch match;
                    std::regex_search(target_as_string, resource.path_regex());
                    target_as_string = match.suffix();
                },
                current_entry);
        }
    }

    return std::move(response);
}
