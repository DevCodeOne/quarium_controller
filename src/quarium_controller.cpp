#include <iostream>

#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "pistache/endpoint.h"

int main(int argc, char *argv[]) {
    std::shared_ptr<spdlog::logger> instance = spdlog::stdout_color_mt("console");

    instance->info("Hello World");
    return EXIT_SUCCESS;
}
