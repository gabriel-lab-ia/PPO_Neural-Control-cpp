#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "domain/types.h"
#include "replay/replay_service.h"

namespace orbital::backend::transport {

std::string ok_response(std::string_view payload_json);
std::string error_response(std::string_view code, std::string_view message, std::string_view path);

std::string run_to_json(const domain::RunRecord& run);
std::string runs_to_json(const std::vector<domain::RunRecord>& runs, std::int64_t limit, std::int64_t offset);

std::string telemetry_sample_to_json(const domain::TelemetrySample& sample);
std::string telemetry_to_json(
    const std::vector<domain::TelemetrySample>& samples,
    std::string_view run_id,
    std::int64_t limit,
    std::int64_t offset
);

std::string event_to_json(const domain::EventRecord& event);
std::string events_to_json(const std::vector<domain::EventRecord>& events, std::string_view run_id);

std::string artifact_to_json(const domain::ArtifactRecord& artifact);
std::string artifacts_to_json(const std::vector<domain::ArtifactRecord>& artifacts, std::string_view run_id);

std::string benchmark_to_json(const domain::BenchmarkRecord& benchmark);
std::string benchmarks_to_json(const std::vector<domain::BenchmarkRecord>& benchmarks, std::int64_t limit, std::int64_t offset);

std::string replay_to_json(const replay::ReplayWindow& replay_window);

std::string job_to_json(const domain::JobRecord& job);

std::string config_presets_json();

}  // namespace orbital::backend::transport
