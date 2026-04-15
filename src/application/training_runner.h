#pragma once

#include <filesystem>
#include <string>

#include "domain/config/experiment_config.h"
#include "domain/ppo/ppo_types.h"

namespace nmc::application {

struct TrainingRunOutput {
    std::string run_id;
    std::filesystem::path run_dir;
    std::filesystem::path manifest_path;
    std::filesystem::path checkpoint_path;
    std::filesystem::path checkpoint_meta_path;
    std::filesystem::path training_summary_path;
    domain::ppo::TrainingMetrics final_metrics{};
    int64_t completed_episodes = 0;
};

class TrainingRunner {
public:
    explicit TrainingRunner(std::filesystem::path artifact_root = "artifacts");

    TrainingRunOutput run(const domain::config::TrainConfig& config);

private:
    std::filesystem::path artifact_root_;
};

}  // namespace nmc::application
