#include <atomic>
#include <string>

#include "clara.hpp"

#include "config.h"
#include "gui/main_view.h"
#include "logger.h"
#include "modules/module_collection.h"
#include "network/network_interface.h"
#include "network/web_application.h"
#include "run_configuration.h"
#include "schedule/schedule_handler.h"
#include "signal_handler.h"

std::atomic_bool _should_exit = false;

void sigint(int) { _should_exit = true; }
void sigterm(int) { _should_exit = true; }

int main(int argc, char *argv[]) {
    signal_handler::install_signal_handler(signal_handler::signal::sigint, sigint, {});
    signal_handler::install_signal_handler(signal_handler::signal::sigterm, sigterm, {});

    // TODO add option to disable gui and network
    bool show_help = false;
    bool print_to_console = false;

    // clang-format off
    auto cli =
        clara::Opt([](const std::string &config_path) { run_configuration::instance()->config_path(config_path); }, "config_path")
            ["-c"]["--config"]
            ("location of the configuration file to use")
        | clara::Opt(
                [](const uint16_t &server_port) {
                run_configuration::instance()->server_port(port(server_port));
                }, "sever_port")
            ["-p"]["--port"]
            ("which port to start the http server on")
        | clara::Opt([](const std::string &log_file) { run_configuration::instance()->log_file(log_file); }, "log_file")
                ["-l"]["--log-file"]
                ("location of the log file to write to")
        | clara::Opt(print_to_console)["--print-to-console"]("specify if the output should also be printed to the standard output")
        | clara::Help(show_help);
    // clang-format on

    auto result = cli.parse(clara::Args(argc, argv));

    if (!result || show_help) {
        if (!result) {
            logger::instance()->critical("Error in command line : {}", result.errorMessage());
        }

        cli.writeToStream(std::cout);

        return result ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    run_configuration::instance()->print_to_console(print_to_console);
    logger::configure_logger(logger::log_level::debug, logger::log_type::file);

    auto conf = config::instance();

    if (conf == nullptr) {
        logger::instance()->critical("No configuration file is specified ...");
        return EXIT_FAILURE;
    }

    auto schedule_file_paths = conf->find("schedule_list");

    if (schedule_file_paths.size() == 0) {
        logger::instance()->critical("No schedule is specified ...");
        return EXIT_FAILURE;
    }

    auto schedule = schedule::create_from_file(schedule_file_paths.at(0).get<std::string>());
    if (schedule.has_value()) {
        schedule_handler::instance()->start_event_handler();
        schedule_handler::instance()->add_schedule(schedule.value());
    } else {
        logger::instance()->critical("Schedule is not valid");
        return EXIT_FAILURE;
    }

    auto inst = main_view::instance();

    if (inst) {
        inst->open_view();
    } else {
        logger::instance()->warn("Couldn't open gui");
    }

    auto network_iface = network_interface::create_on_port(port(run_configuration::instance()->server_port()));
    network_iface->add_route(std::regex("/api/v0/log", std::regex_constants::extended),
                             rest_resource<logger>::handle_request);
    network_iface->add_route(std::regex("/api/v0/schedules.*", std::regex_constants::extended),
                             rest_resource<schedule_handler>::handle_request);
    network_iface->add_route(std::regex("/api/v0/gpio_chip.*", std::regex_constants::extended),
                             rest_resource<gpio_chip>::handle_request);
    network_iface->add_route(std::regex("/webapp.*", std::regex_constants::extended),
                             rest_resource<web_application>::handle_request);

    if (!(network_iface && network_iface->start())) {
        logger::instance()->critical("Couldn't start network interface");

        return EXIT_FAILURE;
    }

    while (!_should_exit) {
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

    network_iface->stop();
    return EXIT_SUCCESS;
}
