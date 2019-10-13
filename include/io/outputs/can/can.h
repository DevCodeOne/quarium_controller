#pragma once
#include <unistd.h>

#include <filesystem>
#include <memory>
#include <mutex>

#include "io/outputs/output_interface.h"

class can final {
   public:
    static inline constexpr char default_can_path[] = "/dev/can0";

    static std::shared_ptr<can> instance(const std::filesystem::path &can_path = default_can_path);

    can(can &&other) = default;
    can(const can &other) = delete;
    ~can() = default;

    can &operator=(const can &other) = delete;
    can &operator=(can &&other) = default;

    const std::filesystem::path &path_to_file() const;

   private:
    static inline std::shared_ptr<can> _instance = nullptr;
    static inline std::mutex _instance_mutex{};

    static inline auto socket_deleter = [](int *socket_handle) {
        if (socket_handle && *socket_handle) {
            close(*socket_handle);
        }
    };
    using socket_handle_ptr_type = std::unique_ptr<int, decltype(+socket_deleter)>;

    can(socket_handle_ptr_type &&socket_handle);

    socket_handle_ptr_type m_socket_handle;
};
