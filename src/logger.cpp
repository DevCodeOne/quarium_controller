#include <fstream>
#include <sstream>

#include "logger.h"
#include "run_configuration.h"

void logger::configure_logger(const log_level &level, const log_type &type) {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_st>();
    std::shared_ptr<spdlog::sinks::rotating_file_sink_st> file_sink = nullptr;
    try {
        file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(run_configuration::instance()->log_file(),
                                                                           1024 * 1024 * 5, 0);
    } catch (...) {
    }

    if (!_instance) {
        switch (type) {
            case log_type::console:
                if (file_sink) {
                    _instance =
                        std::shared_ptr<spdlog::logger>(new spdlog::logger(_logger_name, {console_sink, file_sink}));
                } else {
                    _instance = std::shared_ptr<spdlog::logger>(new spdlog::logger(_logger_name, console_sink));
                }
                break;
            case log_type::file:
                if (file_sink) {
                    _instance = std::shared_ptr<spdlog::logger>(new spdlog::logger(_logger_name, file_sink));
                } else {
                    _instance = std::shared_ptr<spdlog::logger>(new spdlog::logger(_logger_name, console_sink));
                }
        }
    }

    _instance->set_level((spdlog::level::level_enum)level);
    _instance->flush_on((spdlog::level::level_enum)level);
}

std::shared_ptr<spdlog::logger> logger::instance() {
    std::lock_guard<std::mutex> instance_guard{_instance_mutex};

    if (!_instance) {
        _instance = spdlog::stdout_color_mt(_logger_name);
    }

    return _instance;
}

void logger::handle(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response) {
    std::ifstream log_file(run_configuration::instance()->log_file());

    std::ostringstream complete_log;
    std::string current_line;

    while (std::getline(log_file, current_line)) {
        complete_log << current_line << '\n';
    }

    complete_log.flush();
    response.send(Pistache::Http::Code::Ok, complete_log.str().c_str());
}
