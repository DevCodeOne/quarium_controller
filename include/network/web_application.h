#pragma once

#include "network_interface.h"

class web_application {
   public:
    static http::response<http::dynamic_body> handle_request(const http::request<http::dynamic_body> &request);
};
