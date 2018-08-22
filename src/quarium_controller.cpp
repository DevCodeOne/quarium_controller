#include <atomic>
#include <string>

#include "clara.hpp"

#include "config.h"
#include "gpio_handler.h"
#include "gui/view.h"
#include "logger.h"
#include "network.h"
#include "network_interface.h"
#include "schedule/schedule_handler.h"
#include "signal_handler.h"

std::atomic_bool _should_exit = false;

void sigint(int) { _should_exit = true; }
void sigterm(int) { _should_exit = true; }

int main(int argc, char *argv[]) {
    signal_handler handler;
    handler.install_signal_handler(signal_handler::signal::sigint, sigint, {});
    handler.install_signal_handler(signal_handler::signal::sigterm, sigterm, {});

    std::string config_path = "";
    std::string log_file = "log";
    // TODO add other command line options
    std::string log_level = "";
    uint16_t server_port = 0;
    bool show_help = false;

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
            ("Which port to start the http server on")
        | clara::Opt(log_file, "log_file")
                ["-l"]["--log-file"]
                ("Location of the log file to write to")
        | clara::Help(show_help);
    // clang-format on

    auto result = cli.parse(clara::Args(argc, argv));

    if (!result || show_help) {
        if (!result) {
            logger::instance()->critical("Error in command line : {}", result.errorMessage());
        }

        for (auto current_line : cli.getHelpColumns()) {
            std::cout << current_line.left << " " << current_line.right << std::endl;
        }

        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    logger::configure_logger(logger::log_level::debug, logger::log_type::file);

    auto conf = config::instance();
    auto schedule_file_paths = conf->find("schedule_list");
    auto schedule = schedule::create_from_file(schedule_file_paths.at(0).get<std::string>());
    if (schedule.has_value()) {
        schedule_handler::instance()->start_event_handler();
        schedule_handler::instance()->add_schedule(schedule.value());
    } else {
        logger::instance()->critical("Schedule is not valid");
    }

    auto inst = view::instance();

    if (inst) {
        inst->open_view();
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
        inst->close_view();
    }
    // network_iface->stop();
    return EXIT_SUCCESS;
}
