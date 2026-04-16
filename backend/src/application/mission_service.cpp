#include "application/mission_service.h"

#include <algorithm>

namespace orbital::backend::application {
namespace {

std::string artifact_type_from_path(const std::filesystem::path& path) {
    if (path.extension() == ".json") {
        return "json";
    }
    if (path.extension() == ".csv") {
        return "csv";
    }
    if (path.extension() == ".pt") {
        return "model";
    }
    if (path.extension() == ".meta") {
        return "metadata";
    }
    return "file";
}

}  // namespace

MissionService::MissionService(
    persistence::SQLiteStore store,
    telemetry::CsvTelemetryStore telemetry_store,
    std::filesystem::path artifact_root
)
    : store_(std::move(store)),
      telemetry_store_(std::move(telemetry_store)),
      replay_service_(store_, telemetry_store_),
      artifact_root_(std::move(artifact_root)) {}

std::vector<domain::RunRecord> MissionService::list_runs(const std::int64_t limit, const std::int64_t offset) const {
    return store_.list_runs(limit, offset);
}

std::optional<domain::RunRecord> MissionService::get_run(const std::string& run_id) const {
    return store_.get_run(run_id);
}

std::vector<domain::TelemetrySample> MissionService::get_telemetry(
    const std::string& run_id,
    const std::int64_t limit,
    const std::int64_t offset
) const {
    const auto all_samples = telemetry_store_.load_run_samples(run_id);
    if (offset < 0 || limit <= 0 || all_samples.empty()) {
        return {};
    }

    const std::size_t start = static_cast<std::size_t>(std::min<std::int64_t>(offset, static_cast<std::int64_t>(all_samples.size())));
    const std::size_t end = static_cast<std::size_t>(
        std::min<std::int64_t>(
            static_cast<std::int64_t>(all_samples.size()),
            offset + limit
        )
    );

    if (start >= end) {
        return {};
    }

    return std::vector<domain::TelemetrySample>(all_samples.begin() + static_cast<std::ptrdiff_t>(start), all_samples.begin() + static_cast<std::ptrdiff_t>(end));
}

std::vector<domain::TelemetrySample> MissionService::get_telemetry_window(
    const std::string& run_id,
    const std::int64_t start_step,
    const std::int64_t end_step,
    const std::int64_t downsample_points
) const {
    const auto all_samples = telemetry_store_.load_run_samples(run_id);
    const auto clipped = telemetry::clip_by_step_window(all_samples, start_step, end_step);
    return telemetry::downsample_samples(clipped, downsample_points);
}

std::vector<domain::EventRecord> MissionService::list_events(
    const std::string& run_id,
    const std::int64_t limit,
    const std::int64_t offset
) const {
    return store_.list_events(run_id, limit, offset);
}

std::vector<domain::BenchmarkRecord> MissionService::list_benchmarks(
    const std::int64_t limit,
    const std::int64_t offset
) const {
    return store_.list_benchmarks(limit, offset);
}

std::optional<domain::BenchmarkRecord> MissionService::get_benchmark(const std::string& benchmark_id_or_name) const {
    return store_.get_benchmark(benchmark_id_or_name);
}

replay::ReplayWindow MissionService::build_replay(const ReplayRequest& request) const {
    return replay_service_.build_window(
        request.run_id,
        request.start_step,
        request.end_step,
        request.downsample_points,
        request.event_limit
    );
}

std::vector<domain::ArtifactRecord> MissionService::list_artifacts(const std::string& run_id) const {
    const auto run_dir = artifact_root_ / "runs" / run_id;

    std::vector<domain::ArtifactRecord> artifacts;
    if (!std::filesystem::exists(run_dir)) {
        return artifacts;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(run_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        domain::ArtifactRecord record;
        record.name = entry.path().filename().string();
        record.path = std::filesystem::relative(entry.path(), artifact_root_).string();
        record.type = artifact_type_from_path(entry.path());
        record.size_bytes = entry.file_size();
        artifacts.push_back(std::move(record));
    }

    std::sort(artifacts.begin(), artifacts.end(), [](const domain::ArtifactRecord& lhs, const domain::ArtifactRecord& rhs) {
        return lhs.path < rhs.path;
    });

    return artifacts;
}

}  // namespace orbital::backend::application
