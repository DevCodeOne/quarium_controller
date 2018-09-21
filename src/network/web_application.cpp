#include <filesystem>
#include <fstream>

#include "logger.h"
#include "network/web_application.h"

http::response<http::dynamic_body> web_application::handle_request(const http::request<http::dynamic_body> &request) {
    using namespace std::literals;

    http::response<http::dynamic_body> response;

    std::string resource_path_string(".."s + request.target().to_string());

    if ("../webapp"s == resource_path_string) {
        resource_path_string = resource_path_string.append("/index.html");
    }
    logger::instance()->info("Requesting {} from web_application", resource_path_string);

    std::filesystem::path resource_path(resource_path_string);

    if (std::filesystem::exists(resource_path)) {
        std::ifstream input_file(resource_path);
        std::string current_line;

        if (!input_file) {
            boost::beast::ostream(response.body())
                << "Resource : " << resource_path.c_str() << " couldn't be opened" << '\n';
        }

        while (std::getline(input_file, current_line)) {
            boost::beast::ostream(response.body()) << current_line << '\n';
        }

    } else {
        boost::beast::ostream(response.body()) << "Resource : " << resource_path.c_str() << "doesn't exist" << '\n';
    }

    return std::move(response);
}
