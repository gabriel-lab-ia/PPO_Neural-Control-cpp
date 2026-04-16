#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "domain/types.h"
#include "persistence/sqlite_store.h"
#include "replay/replay_service.h"
#include "telemetry/csv_telemetry_store.h"

namespace orbital::backend::application {

struct ReplayRequest {
    std::string run_id;
    std::int64_t start_step = 0;
    std::int64_t end_step = -1;
    std::int64_t downsample_points = 1200;
    std::int64_t event_limit = 512;
};

class MissionService {
public:
    MissionService(
        persistence::SQLiteStore store,
        telemetry::CsvTelemetryStore telemetry_store,
        std::filesystem::path artifact_root
    );

    [[nodiscard]] std::vector<domain::RunRecord> list_runs(std::int64_t limit, std::int64_t offset) const;
    [[nodiscard]] std::optional<domain::RunRecord> get_run(const std::string& run_id) const;

    [[nodiscard]] std::vector<domain::TelemetrySample> get_telemetry(
        const std::string& run_id,
        std::int64_t limit,
        std::int64_t offset
    ) const;

    [[nodiscard]] std::vector<domain::TelemetrySample> get_telemetry_window(
        const std::string& run_id,
        std::int64_t start_step,
        std::int64_t end_step,
        std::int64_t downsample_points
    ) const;

    [[nodiscard]] std::vector<domain::EventRecord> list_events(
        const std::string& run_id,
        std::int64_t limit,
        std::int64_t offset
    ) const;

    [[nodiscard]] std::vector<domain::BenchmarkRecord> list_benchmarks(std::int64_t limit, std::int64_t offset) const;
    [[nodiscard]] std::optional<domain::BenchmarkRecord> get_benchmark(const std::string& benchmark_id_or_name) const;

    [[nodiscard]] replay::ReplayWindow build_replay(const ReplayRequest& request) const;
    [[nodiscard]] std::vector<domain::ArtifactRecord> list_artifacts(const std::string& run_id) const;

private:
    persistence::SQLiteStore store_;
    telemetry::CsvTelemetryStore telemetry_store_;
    replay::ReplayService replay_service_;
    std::filesystem::path artifact_root_;
};

}  // namespace orbital::backend::application
