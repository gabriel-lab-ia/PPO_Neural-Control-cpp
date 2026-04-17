#pragma once

#include <filesystem>
#include <string>

#include "domain/config/experiment_config.h"

namespace nmc::application {

struct EvaluationRunOutput {
    std::string run_id;
    std::filesystem::path run_dir;
    std::filesystem::path manifest_path;
    std::filesystem::path evaluation_summary_path;
    std::string backend_runtime;
    float avg_episode_return = 0.0f;
    float avg_episode_length = 0.0f;
    float success_rate = 0.0f;
    float avg_inference_latency_ms = 0.0f;
    float p95_inference_latency_ms = 0.0f;
};

class EvaluationRunner {
public:
    explicit EvaluationRunner(std::filesystem::path artifact_root = "artifacts");

    EvaluationRunOutput run(const domain::config::EvalConfig& config);

private:
    std::filesystem::path artifact_root_;
};

}  // namespace nmc::application
