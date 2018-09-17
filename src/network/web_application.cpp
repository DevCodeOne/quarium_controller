#include <filesystem>

#include "logger.h"
#include "network/web_application.h"

void web_application::handle(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response) {
    using namespace std::literals;
    using namespace Pistache;

    std::string resource_path = request.resource();

    if (resource_path == "/webapp") {
        resource_path.append("/index.html");
    }

    std::string relative_path = ".."s + resource_path;

    logger::instance()->info("Requesting {}", relative_path);

    // Doesn't close the file properly so it will eventually throw an execption
    Http::serveFile(response, relative_path.c_str());
};
