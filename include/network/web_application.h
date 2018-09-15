#pragma once

#include "network_header.h"

class web_application {
   public:
    static void handle(const Pistache::Http::Request &request, Pistache::Http::ResponseWriter response);
};
