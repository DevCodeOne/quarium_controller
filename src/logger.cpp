#include <fstream>
#include <sstream>

#include "logger.h"
#include "run_configuration.h"

void logger::configure_logger(const log_level &level, const log_type &type) {
    std::lock_guard<std::recursive_mutex> instance_guard{_instance_mutex};

    auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_st>();
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> file_sink = nullptr;
    try {
        file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(run_configuration::instance()->log_file(),
                                                                           1024 * 1024 * 5, 0);
    } catch (...) {
        logger::instance()->warn("Couldn't open log file");
    }

    if (!_instance && file_sink) {
        if (type == log_type::console || run_configuration::instance()->print_to_console()) {
            _instance = std::shared_ptr<spdlog::logger>(new spdlog::logger(_logger_name, {console_sink, file_sink}));
        } else if (type == log_type::file) {
            _instance = std::shared_ptr<spdlog::logger>(new spdlog::logger(_logger_name, file_sink));
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

http::response<http::dynamic_body> logger::handle_request(const http::request<http::dynamic_body> &request) {
    http::response<http::dynamic_body> response;

    std::ifstream log_file(run_configuration::instance()->log_file());
    std::string current_line;

    if (log_file) {
        while (std::getline(log_file, current_line)) {
            boost::beast::ostream(response.body()) << current_line << '\n';
        }
    } else {
        boost::beast::ostream(response.body()) << "The log file couldn't be opened" << '\n';
    }

    return std::move(response);
}
