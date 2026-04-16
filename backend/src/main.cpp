#include "application/job_service.h"
#include "application/mission_service.h"
#include "common/logger.h"
#include "persistence/sqlite_store.h"
#include "telemetry/csv_telemetry_store.h"
#include "transport/http_server.h"

#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <string>

namespace {

std::uint16_t parse_port_from_env() {
    const char* raw = std::getenv("ORBITAL_BACKEND_PORT");
    if (raw == nullptr) {
        return 8080;
    }

    try {
        const int value = std::stoi(raw);
        if (value <= 0 || value > 65535) {
            return 8080;
        }
        return static_cast<std::uint16_t>(value);
    } catch (...) {
        return 8080;
    }
}

std::filesystem::path path_from_env(const char* key, const std::filesystem::path& fallback) {
    const char* raw = std::getenv(key);
    if (raw == nullptr || std::string(raw).empty()) {
        return fallback;
    }
    return std::filesystem::path(raw);
}

bool executor_enabled() {
    const char* raw = std::getenv("ORBITAL_JOB_EXECUTOR");
    return raw != nullptr && std::string(raw) == "1";
}

}  // namespace

int main() {
    try {
        const auto artifact_root = path_from_env("ORBITAL_ARTIFACT_ROOT", std::filesystem::path{"artifacts"});
        const auto sqlite_path = path_from_env("ORBITAL_SQLITE_PATH", artifact_root / "experiments.sqlite");
        const auto repo_root = path_from_env("ORBITAL_REPO_ROOT", std::filesystem::current_path());

        orbital::backend::persistence::SQLiteStore sqlite_store(sqlite_path);
        orbital::backend::telemetry::CsvTelemetryStore telemetry_store(artifact_root);
        orbital::backend::application::MissionService mission_service(
            std::move(sqlite_store),
            std::move(telemetry_store),
            artifact_root
        );
        orbital::backend::application::JobService job_service(repo_root, executor_enabled());

        orbital::backend::transport::ServerConfig config;
        config.port = parse_port_from_env();

        orbital::backend::transport::HttpServer server(config, mission_service, job_service);
        server.run();
        return 0;
    } catch (const std::exception& ex) {
        orbital::backend::common::log(orbital::backend::common::LogLevel::Error, ex.what());
        return 1;
    }
}
