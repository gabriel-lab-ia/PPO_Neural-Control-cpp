#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

struct sqlite3;

namespace nmc::infrastructure::persistence {

struct RunStart {
    std::string run_id;
    std::string mode;
    std::string environment;
    int64_t seed = 0;
    std::string started_at;
    std::string status;
    std::string artifact_dir;
    std::string config_json;
};

struct EpisodeTelemetry {
    std::string run_id;
    std::string phase;
    int64_t episode_index = 0;
    int64_t env_steps = 0;
    float episode_return = 0.0f;
    int64_t episode_length = 0;
    float success = 0.0f;
    std::string created_at;
};

struct EventTelemetry {
    std::string run_id;
    std::string level;
    std::string event_type;
    std::string message;
    std::string payload_json;
    std::string created_at;
};

struct BenchmarkTelemetry {
    std::string benchmark_name;
    std::string run_id;
    std::string summary_json;
    std::string created_at;
};

class SQLiteExperimentStore {
public:
    explicit SQLiteExperimentStore(std::filesystem::path db_path);
    ~SQLiteExperimentStore();

    SQLiteExperimentStore(const SQLiteExperimentStore&) = delete;
    SQLiteExperimentStore& operator=(const SQLiteExperimentStore&) = delete;

    void initialize();
    void insert_run_start(const RunStart& run);
    void finalize_run(
        const std::string& run_id,
        const std::string& status,
        const std::string& ended_at,
        const std::string& summary_json
    );

    void insert_episode(const EpisodeTelemetry& episode);
    void insert_event(const EventTelemetry& event);
    void insert_benchmark(const BenchmarkTelemetry& benchmark);

private:
    void execute(const std::string& sql) const;

    std::filesystem::path db_path_;
    sqlite3* db_ = nullptr;
};

}  // namespace nmc::infrastructure::persistence
