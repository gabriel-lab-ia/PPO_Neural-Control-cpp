#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace orbital::backend::domain {

enum class JobType {
    Train,
    Eval,
    Benchmark
};

enum class JobStatus {
    Queued,
    Running,
    Completed,
    Failed
};

struct RunRecord {
    std::string run_id;
    std::string mode;
    std::string environment;
    std::int64_t seed = 0;
    std::string started_at;
    std::string ended_at;
    std::string status;
    std::string artifact_dir;
    std::string config_json;
    std::string summary_json;
};

struct EventRecord {
    std::int64_t id = 0;
    std::string run_id;
    std::string level;
    std::string event_type;
    std::string message;
    std::string payload_json;
    std::string created_at;
};

struct BenchmarkRecord {
    std::int64_t id = 0;
    std::string benchmark_name;
    std::string run_id;
    std::string summary_json;
    std::string created_at;
};

struct ArtifactRecord {
    std::string name;
    std::string path;
    std::string type;
    std::uintmax_t size_bytes = 0;
};

struct TelemetrySample {
    std::int64_t step = 0;
    double mission_time_s = 0.0;
    double reward = 0.0;
    double action = 0.0;
    double value = 0.0;
    double control_magnitude = 0.0;
    double orbital_error_km = 0.0;
    double velocity_magnitude_kmps = 0.0;
    double policy_std = 0.0;
    std::array<double, 3> position_km{0.0, 0.0, 0.0};
    std::array<double, 3> velocity_kmps{0.0, 0.0, 0.0};
    std::array<double, 3> control_vector{0.0, 0.0, 0.0};
    bool terminated = false;
    bool truncated = false;
    std::string timestamp;
};

struct JobRecord {
    std::string job_id;
    JobType job_type = JobType::Benchmark;
    JobStatus status = JobStatus::Queued;
    std::string run_id;
    std::string created_at;
    std::string updated_at;
    std::string details_json;
};

inline const char* to_string(const JobType type) {
    switch (type) {
        case JobType::Train:
            return "train";
        case JobType::Eval:
            return "eval";
        case JobType::Benchmark:
            return "benchmark";
    }
    return "unknown";
}

inline const char* to_string(const JobStatus status) {
    switch (status) {
        case JobStatus::Queued:
            return "queued";
        case JobStatus::Running:
            return "running";
        case JobStatus::Completed:
            return "completed";
        case JobStatus::Failed:
            return "failed";
    }
    return "unknown";
}

}  // namespace orbital::backend::domain
