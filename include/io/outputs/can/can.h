#pragma once
#include <unistd.h>

#include <filesystem>
#include <memory>
#include <mutex>

#include "io/outputs/output_interface.h"
#include "logger.h"

enum struct can_object_identifier : uint16_t {};

enum struct can_error_code { ok = 0, send_error = 1, receive_error };

// TODO: map of can instances, since there can be multiple can devices simultaniously
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

    // TODO: Allow different packet size
    can_error_code send(const can_object_identifier &identifier, uint64_t data);
    can_error_code receive(const can_object_identifier &identifier, uint64_t *data);

   private:
    static inline std::shared_ptr<can> _instance = nullptr;
    static inline std::mutex _instance_mutex{};

    static inline auto socket_deleter = [](int *socket_handle) {
        if (socket_handle && *socket_handle) {
            logger::instance()->warn("Closing can socket");
            close(*socket_handle);
        }
    };
    using socket_handle_ptr_type = std::unique_ptr<int, decltype(+socket_deleter)>;

    can(socket_handle_ptr_type &&socket_handle);

    socket_handle_ptr_type m_socket_handle;
};
