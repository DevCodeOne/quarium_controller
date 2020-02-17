#include "logger.h"

#include <fstream>
#include <sstream>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "run_configuration.h"

void logger::configure_logger(const log_level &level, const log_type &type) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> file_sink = nullptr;
    try {
        file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(run_configuration::instance()->log_file(),
                                                                           1024 * 1024 * 5, 0);
    } catch (...) {
        logger::instance()->warn("Couldn't open log file");
    }

    if (!_instance && file_sink) {
        if (type == log_type::console || run_configuration::instance()->print_to_console()) {
            _instance = std::make_shared<spdlog::logger>(_logger_name, spdlog::sinks_init_list({console_sink, file_sink}));
        } else if (type == log_type::file) {
            _instance = std::make_shared<spdlog::logger>(_logger_name, file_sink);
        }
    } else {
        logger::instance();
    }
    _instance->set_level((spdlog::level::level_enum)level);
    _instance->flush_on((spdlog::level::level_enum)level);
}

std::shared_ptr<spdlog::logger> logger::instance() {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = spdlog::stdout_color_mt(_logger_name);
    }

    return _instance;
}

void logger::handle_request(const httplib::Request &request, httplib::Response &response) {
    using namespace std::literals;
    std::ifstream log_file(run_configuration::instance()->log_file());
    std::string current_line;
    std::ostringstream buffer;

    if (log_file && !run_configuration::instance()->print_to_console()) {
        while (std::getline(log_file, current_line)) {
            buffer << current_line << '\n';
        }

        current_line = buffer.str();
        response.set_content(current_line.c_str(), "text/plain");
    } else {
        response.set_content("The log file couldn't be opened \n"s, "text/plain");
    }
}
