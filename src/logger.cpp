#include <fstream>
#include <sstream>

#include "logger.h"

void logger::configure_logger(const log_level &level, const log_type &type) {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    // TODO apply runtime_configuration
    auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_st>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(filepath, 1024 * 1024 * 5, 0);
    if (!_instance) {
        switch (type) {
            case log_type::console:
                _instance = std::shared_ptr<spdlog::logger>(new spdlog::logger(logger_name, {console_sink, file_sink}));
                break;
            case log_type::file:
                try {
                    _instance = std::shared_ptr<spdlog::logger>(new spdlog::logger(logger_name, {file_sink}));
                } catch (...) {
                    _instance = spdlog::stdout_color_mt(logger_name);
                    _instance->critical("Couldn't open log file");
                }
        }
    }

    _instance->set_level((spdlog::level::level_enum)level);
    _instance->flush_on((spdlog::level::level_enum)level);
}

std::shared_ptr<spdlog::logger> logger::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = spdlog::stdout_color_mt(logger_name);
    }

    return _instance;
}

void logger::handle(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response) {
    std::ifstream log_file(filepath);

    std::ostringstream complete_log;
    std::string current_line;

    while (std::getline(log_file, current_line)) {
        complete_log << current_line << '\n';
    }

    complete_log.flush();
    response.send(Pistache::Http::Code::Ok, complete_log.str().c_str());
}
