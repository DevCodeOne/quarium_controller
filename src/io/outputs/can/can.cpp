#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "io/outputs/can/can.h"

std::shared_ptr<can> can::instance(const std::filesystem::path &can_path) {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (_instance) {
        return _instance;
    }

    ifreq ifr;
    std::strncpy(ifr.ifr_name, can_path.c_str(), IFNAMSIZ - 1);
    // Definitly set terminating null character
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    ifr.ifr_ifindex = if_nametoindex(ifr.ifr_name);

    if (!ifr.ifr_ifindex) {
        return nullptr;
    }

    socket_handle_ptr_type socket_handle{new int{socket(PF_CAN, SOCK_RAW, CAN_RAW)}, socket_deleter};
    sockaddr_can addr{.can_family = AF_CAN, .can_ifindex = ifr.ifr_ifindex};

    if (*socket_handle == 0) {
        return nullptr;
    }

    if (bind(*socket_handle, (sockaddr *)&addr, sizeof(addr)) < 0) {
        return nullptr;
    }

    return (_instance = std::shared_ptr<can>(new can(std::move(socket_handle))));
}

can::can(socket_handle_ptr_type &&socket_handle) : m_socket_handle(std::move(socket_handle)) {}
