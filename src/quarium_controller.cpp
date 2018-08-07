#include <atomic>
#include <string>

#include "clara.hpp"

#include "config.h"
#include "gpio_handler.h"
#include "gui.h"
#include "logger.h"
#include "network.h"
#include "network_interface.h"
#include "schedule.h"
#include "signal_handler.h"

std::atomic_bool _should_exit = false;

void sigint(int) { _should_exit = true; }
void sigterm(int) { _should_exit = true; }

int main(int argc, char *argv[]) {
    signal_handler handler;
    handler.install_signal_handler(signal_handler::signal::sigint, sigint, {});
    handler.install_signal_handler(signal_handler::signal::sigterm, sigterm, {});

    std::string config_path = "";
    uint16_t server_port;

    // clang-format off
    auto cli =
        clara::Opt(config_path, "config_path")
            ["-c"]["--config"]
            ("Location of the configuration file to use")
        | clara::Opt(
                [&server_port](uint16_t p) {
                server_port = p;
                }, "sever_port")
            ["-p"]["--port"]
            ("Which port to start the http server on");
    // clang-format on

    auto result = cli.parse(clara::Args(argc, argv));

    if (!result) {
        logger::instance()->critical("Error in command line : {}", result.errorMessage());

        for (auto current_line : cli.getHelpColumns()) {
            logger::instance()->info("{}, ", current_line.left, current_line.right);
        }

        return EXIT_FAILURE;
    }

    auto conf = config::instance();
    auto schedule_file_paths = conf->find("schedule_list");
    auto schedule = schedule::create_from_file(schedule_file_paths.at(0).get<std::string>());
    if (schedule.has_value()) {
        schedule_handler::instance()->start_event_handler();
        schedule_handler::instance()->add_schedule(schedule.value());
    } else {
        logger::instance()->critical("Schedule is not valid");
    }

    auto gpiochip = gpio_chip::instance(gpio_chip::default_gpio_dev_path);

    if (!gpiochip) {
        logger::instance()->critical("Couldn't open gpiochip");
    }

    auto inst = gui::instance();

    if (inst) {
        inst.value()->open_gui();
    } else {
        logger::instance()->warn("Couldn't open inst");
    }

    // auto network_iface = network_interface::create_on_port(port(9980));

    // if (!network_iface) {
    //     logger::instance()->critical("Couldn't start network interface");

    //     return EXIT_FAILURE;
    // }

    // network_iface->start();

    while (true) {
        if (_should_exit) {
            break;
        }

        if (pause() < 0) {
            if (errno == EINTR) {
                continue;
            }

            logger::instance()->warn("Error in pause() {}", strerror(errno));
        }
    }

    logger::instance()->info("Shutting down server, schedule handler and gui");
    schedule_handler::instance()->stop_event_handler();

    if (inst) {
        inst.value()->close_gui();
    }
    // network_iface->stop();
    return EXIT_SUCCESS;
}
