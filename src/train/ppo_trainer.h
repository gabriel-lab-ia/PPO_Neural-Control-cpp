#pragma once

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <vector>

#include "env/environment.h"
#include "env/environment_registry.h"
#include "model/ppo_agent.h"
#include "train/rollout_buffer.h"

namespace nmc {

class PPOTrainer {
public:
    struct LiveStep {
        int64_t step = 0;
        float reward = 0.0f;
        float action = 0.0f;
        float value = 0.0f;
        std::vector<float> observation;
        bool terminated = false;
        bool truncated = false;
    };

    PPOTrainer(
        TrainerConfig config,
        std::filesystem::path artifact_dir,
        EnvironmentPack environment_pack
    );

    std::vector<TrainingMetrics> train();
    std::vector<LiveStep> run_live_episode(int64_t max_steps, std::ostream& stream);
    PPOAgent& agent();

private:
    RolloutBatch collect_rollout();
    TrainingMetrics update_policy(const RolloutBatch& batch, int64_t update_index);
    torch::Tensor stack_observations() const;
    float benchmark_inference_latency_ms();

    TrainerConfig config_;
    std::filesystem::path artifact_dir_;
    torch::Device device_{torch::kCPU};
    PPOAgent agent_{nullptr};
    std::unique_ptr<torch::optim::Adam> optimizer_;
    std::vector<std::unique_ptr<Environment>> environments_;
    std::vector<torch::Tensor> current_observations_;
    std::vector<float> episode_returns_;
    std::vector<int64_t> episode_lengths_;
    std::vector<float> finished_returns_;
    std::vector<int64_t> finished_lengths_;
    std::vector<float> finished_successes_;
    int64_t total_env_steps_ = 0;
    float cached_inference_latency_ms_ = 0.0f;
    float parameter_count_k_ = 0.0f;
};

}  // namespace nmc
