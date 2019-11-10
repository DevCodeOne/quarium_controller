#include "io/interfaces/can/can.h"

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <climits>
#include <cstring>

#include "logger.h"

std::shared_ptr<can> can::instance(const std::filesystem::path &can_path) {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (_instance) {
        return _instance;
    }

    auto logger_instance = logger::instance();

    ifreq ifr;
    std::strncpy(ifr.ifr_name, can_path.c_str(), IFNAMSIZ - 1);
    // Definetly set terminating null character, so if the string is too big the terminating \0 is set, to prevent
    // overflows
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);

    if (!ifr.ifr_ifindex) {
        logger_instance->critical("Couldn't find the can bus device {}", can_path.c_str());
        return nullptr;
    }

    // TODO: Find out why this is an error in valgrind
    int *socket_handle_raw = new int(socket(PF_CAN, SOCK_RAW, CAN_RAW));
    socket_handle_ptr_type socket_handle(socket_handle_raw, socket_deleter);
    sockaddr_can addr{.can_family = AF_CAN, .can_ifindex = ifr.ifr_ifindex};

    if (*socket_handle == 0) {
        logger_instance->critical("Couldn't open a socket for the can device {}", can_path.c_str());
        return nullptr;
    }

    if (bind(*socket_handle, (sockaddr *)&addr, sizeof(addr)) < 0) {
        logger_instance->critical("Couldn't bind the socket for the can device {}", can_path.c_str());
        return nullptr;
    }

    return (_instance = std::shared_ptr<can>(new can(std::move(socket_handle))));
}

can::can(socket_handle_ptr_type &&socket_handle) : m_socket_handle(std::move(socket_handle)) {}

can_error_code can::send(const can_object_identifier &identifier, uint64_t data) {
    canfd_frame frame;
    std::memset((char *)&frame, 0, sizeof(frame));

    frame.can_id = (canid_t)identifier;
    frame.len = sizeof(data) / CHAR_BIT;
    std::memcpy(frame.data, (char *)&data, sizeof(data));

    return write(*m_socket_handle, &frame, CAN_MTU) == CAN_MTU ? can_error_code::ok : can_error_code::send_error;
}
