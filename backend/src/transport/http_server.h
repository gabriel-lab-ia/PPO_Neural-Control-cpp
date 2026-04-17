#pragma once

#include <cstdint>
#include <string>

#include "application/job_service.h"
#include "application/mission_service.h"

namespace orbital::backend::transport {

struct ServerConfig {
    std::uint16_t port = 8080;
    std::string service_name = "orbital-backend";
    std::string service_version = "0.4.1";
    std::string schema_version = "2026-04-17";
    std::string source = "orbital.backend";
};

class HttpServer {
public:
    HttpServer(ServerConfig config, application::MissionService& mission_service, application::JobService& job_service);

    void run();

private:
    ServerConfig config_;
    application::MissionService& mission_service_;
    application::JobService& job_service_;
};

}  // namespace orbital::backend::transport
