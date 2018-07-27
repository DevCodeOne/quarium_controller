#include "config.h"

#include "pistache/endpoint.h"
#include "network.h"

int main(int argc, char *argv[]) {
    config::instance();
    network::wpa_interface_path interface_path("/var/run/wpa_supplicant", "wlan0pistachste");
    return EXIT_SUCCESS;
}
