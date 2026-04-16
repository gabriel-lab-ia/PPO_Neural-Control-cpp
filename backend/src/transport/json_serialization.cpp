#include "transport/json_serialization.h"

#include "common/json.h"
#include "common/time_utils.h"

#include <sstream>

namespace orbital::backend::transport {
namespace {

std::string json_or_null(const std::string& raw_json) {
    if (raw_json.empty()) {
        return "null";
    }

    const auto first = raw_json.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) {
        return "null";
    }

    const char prefix = raw_json[first];
    if (prefix == '{' || prefix == '[' || prefix == '"' || prefix == 't' || prefix == 'f' || prefix == 'n' || prefix == '-' || (prefix >= '0' && prefix <= '9')) {
        return raw_json;
    }

    return common::json_string(raw_json);
}

std::string join_json(const std::vector<std::string>& items) {
    std::ostringstream stream;
    stream << '[';
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i != 0) {
            stream << ',';
        }
        stream << items[i];
    }
    stream << ']';
    return stream.str();
}

}  // namespace

std::string ok_response(const std::string_view payload_json) {
    std::ostringstream stream;
    stream << "{\"status\":\"ok\",\"timestamp\":" << common::json_string(common::now_utc_iso8601()) << ",\"data\":" << payload_json << '}';
    return stream.str();
}

std::string error_response(const std::string_view code, const std::string_view message, const std::string_view path) {
    std::ostringstream stream;
    stream << "{\"status\":\"error\",\"timestamp\":" << common::json_string(common::now_utc_iso8601()) << ",\"error\":{"
           << "\"code\":" << common::json_string(code) << ','
           << "\"message\":" << common::json_string(message) << ','
           << "\"path\":" << common::json_string(path)
           << "}}";
    return stream.str();
}

std::string run_to_json(const domain::RunRecord& run) {
    std::ostringstream stream;
    stream << '{'
           << "\"run_id\":" << common::json_string(run.run_id) << ','
           << "\"mode\":" << common::json_string(run.mode) << ','
           << "\"environment\":" << common::json_string(run.environment) << ','
           << "\"seed\":" << run.seed << ','
           << "\"started_at\":" << common::json_string(run.started_at) << ','
           << "\"ended_at\":" << (run.ended_at.empty() ? "null" : common::json_string(run.ended_at)) << ','
           << "\"status\":" << common::json_string(run.status) << ','
           << "\"artifact_dir\":" << common::json_string(run.artifact_dir) << ','
           << "\"config\":" << json_or_null(run.config_json) << ','
           << "\"summary\":" << json_or_null(run.summary_json)
           << '}';
    return stream.str();
}

std::string runs_to_json(const std::vector<domain::RunRecord>& runs, const std::int64_t limit, const std::int64_t offset) {
    std::vector<std::string> rows;
    rows.reserve(runs.size());
    for (const auto& run : runs) {
        rows.push_back(run_to_json(run));
    }

    std::ostringstream stream;
    stream << '{'
           << "\"limit\":" << limit << ','
           << "\"offset\":" << offset << ','
           << "\"count\":" << runs.size() << ','
           << "\"items\":" << join_json(rows)
           << '}';
    return stream.str();
}

std::string telemetry_sample_to_json(const domain::TelemetrySample& sample) {
    std::ostringstream stream;
    stream << '{'
           << "\"step\":" << sample.step << ','
           << "\"mission_time_s\":" << sample.mission_time_s << ','
           << "\"reward\":" << sample.reward << ','
           << "\"action\":" << sample.action << ','
           << "\"value\":" << sample.value << ','
           << "\"control_magnitude\":" << sample.control_magnitude << ','
           << "\"orbital_error_km\":" << sample.orbital_error_km << ','
           << "\"velocity_magnitude_kmps\":" << sample.velocity_magnitude_kmps << ','
           << "\"policy_std\":" << sample.policy_std << ','
           << "\"position_km\":[" << sample.position_km[0] << ',' << sample.position_km[1] << ',' << sample.position_km[2] << "],"
           << "\"velocity_kmps\":[" << sample.velocity_kmps[0] << ',' << sample.velocity_kmps[1] << ',' << sample.velocity_kmps[2] << "],"
           << "\"control_vector\":[" << sample.control_vector[0] << ',' << sample.control_vector[1] << ',' << sample.control_vector[2] << "],"
           << "\"terminated\":" << (sample.terminated ? "true" : "false") << ','
           << "\"truncated\":" << (sample.truncated ? "true" : "false") << ','
           << "\"timestamp\":" << common::json_string(sample.timestamp)
           << '}';
    return stream.str();
}

std::string telemetry_to_json(
    const std::vector<domain::TelemetrySample>& samples,
    const std::string_view run_id,
    const std::int64_t limit,
    const std::int64_t offset
) {
    std::vector<std::string> rows;
    rows.reserve(samples.size());
    for (const auto& sample : samples) {
        rows.push_back(telemetry_sample_to_json(sample));
    }

    std::ostringstream stream;
    stream << '{'
           << "\"run_id\":" << common::json_string(run_id) << ','
           << "\"limit\":" << limit << ','
           << "\"offset\":" << offset << ','
           << "\"count\":" << samples.size() << ','
           << "\"items\":" << join_json(rows)
           << '}';
    return stream.str();
}

std::string event_to_json(const domain::EventRecord& event) {
    std::ostringstream stream;
    stream << '{'
           << "\"id\":" << event.id << ','
           << "\"run_id\":" << common::json_string(event.run_id) << ','
           << "\"level\":" << common::json_string(event.level) << ','
           << "\"event_type\":" << common::json_string(event.event_type) << ','
           << "\"message\":" << common::json_string(event.message) << ','
           << "\"payload\":" << json_or_null(event.payload_json) << ','
           << "\"created_at\":" << common::json_string(event.created_at)
           << '}';
    return stream.str();
}

std::string events_to_json(const std::vector<domain::EventRecord>& events, const std::string_view run_id) {
    std::vector<std::string> rows;
    rows.reserve(events.size());
    for (const auto& event : events) {
        rows.push_back(event_to_json(event));
    }

    std::ostringstream stream;
    stream << '{'
           << "\"run_id\":" << common::json_string(run_id) << ','
           << "\"count\":" << events.size() << ','
           << "\"items\":" << join_json(rows)
           << '}';
    return stream.str();
}

std::string artifact_to_json(const domain::ArtifactRecord& artifact) {
    std::ostringstream stream;
    stream << '{'
           << "\"name\":" << common::json_string(artifact.name) << ','
           << "\"path\":" << common::json_string(artifact.path) << ','
           << "\"type\":" << common::json_string(artifact.type) << ','
           << "\"size_bytes\":" << artifact.size_bytes
           << '}';
    return stream.str();
}

std::string artifacts_to_json(const std::vector<domain::ArtifactRecord>& artifacts, const std::string_view run_id) {
    std::vector<std::string> rows;
    rows.reserve(artifacts.size());
    for (const auto& artifact : artifacts) {
        rows.push_back(artifact_to_json(artifact));
    }

    std::ostringstream stream;
    stream << '{'
           << "\"run_id\":" << common::json_string(run_id) << ','
           << "\"count\":" << artifacts.size() << ','
           << "\"items\":" << join_json(rows)
           << '}';
    return stream.str();
}

std::string benchmark_to_json(const domain::BenchmarkRecord& benchmark) {
    std::ostringstream stream;
    stream << '{'
           << "\"benchmark_id\":" << benchmark.id << ','
           << "\"benchmark_name\":" << common::json_string(benchmark.benchmark_name) << ','
           << "\"run_id\":" << (benchmark.run_id.empty() ? "null" : common::json_string(benchmark.run_id)) << ','
           << "\"summary\":" << json_or_null(benchmark.summary_json) << ','
           << "\"created_at\":" << common::json_string(benchmark.created_at)
           << '}';
    return stream.str();
}

std::string benchmarks_to_json(
    const std::vector<domain::BenchmarkRecord>& benchmarks,
    const std::int64_t limit,
    const std::int64_t offset
) {
    std::vector<std::string> rows;
    rows.reserve(benchmarks.size());
    for (const auto& benchmark : benchmarks) {
        rows.push_back(benchmark_to_json(benchmark));
    }

    std::ostringstream stream;
    stream << '{'
           << "\"limit\":" << limit << ','
           << "\"offset\":" << offset << ','
           << "\"count\":" << benchmarks.size() << ','
           << "\"items\":" << join_json(rows)
           << '}';
    return stream.str();
}

std::string replay_to_json(const replay::ReplayWindow& replay_window) {
    std::vector<std::string> sample_rows;
    sample_rows.reserve(replay_window.samples.size());
    for (const auto& sample : replay_window.samples) {
        sample_rows.push_back(telemetry_sample_to_json(sample));
    }

    std::vector<std::string> event_rows;
    event_rows.reserve(replay_window.events.size());
    for (const auto& event : replay_window.events) {
        event_rows.push_back(event_to_json(event));
    }

    std::ostringstream stream;
    stream << '{'
           << "\"run_id\":" << common::json_string(replay_window.run_id) << ','
           << "\"run\":" << (replay_window.run.has_value() ? run_to_json(*replay_window.run) : "null") << ','
           << "\"telemetry\":" << join_json(sample_rows) << ','
           << "\"events\":" << join_json(event_rows)
           << '}';
    return stream.str();
}

std::string job_to_json(const domain::JobRecord& job) {
    std::ostringstream stream;
    stream << '{'
           << "\"job_id\":" << common::json_string(job.job_id) << ','
           << "\"job_type\":" << common::json_string(domain::to_string(job.job_type)) << ','
           << "\"status\":" << common::json_string(domain::to_string(job.status)) << ','
           << "\"run_id\":" << common::json_string(job.run_id) << ','
           << "\"created_at\":" << common::json_string(job.created_at) << ','
           << "\"updated_at\":" << common::json_string(job.updated_at) << ','
           << "\"details\":" << json_or_null(job.details_json)
           << '}';
    return stream.str();
}

std::string config_presets_json() {
    return R"json({
  "presets": [
    {
      "name": "train.quick.point_mass",
      "mode": "train",
      "description": "Quick CPU smoke training preset for point_mass PPO.",
      "arguments": {
        "env": "point_mass",
        "quick": true,
        "seed": 7
      }
    },
    {
      "name": "eval.deterministic.point_mass",
      "mode": "eval",
      "description": "Deterministic evaluation from latest checkpoint.",
      "arguments": {
        "env": "point_mass",
        "checkpoint": "artifacts/latest/checkpoint.pt",
        "deterministic": true,
        "seed": 7
      }
    },
    {
      "name": "benchmark.quick",
      "mode": "benchmark",
      "description": "CI-friendly deterministic benchmark preset.",
      "arguments": {
        "quick": true,
        "seed": 7
      }
    }
  ]
})json";
}

}  // namespace orbital::backend::transport
