#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "domain/config/experiment_config.h"
#include "domain/env/environment_factory.h"
#include "domain/ppo/policy_value_model.h"
#include "domain/ppo/ppo_types.h"

namespace nmc::domain::ppo {

class PPOTrainer {
public:
    PPOTrainer(
        config::TrainerConfig config,
        env::EnvironmentPack environment_pack
    );

    std::vector<TrainingMetrics> train();
    std::vector<LiveStep> run_live_episode(int64_t max_steps, bool deterministic, std::ostream* stream);
    std::vector<EpisodeRecord> consume_completed_episodes();

    PolicyValueModel& model();
    const std::string& environment_name() const;
    int64_t observation_dim() const;
    int64_t action_dim() const;

private:
    RolloutBatch collect_rollout();
    TrainingMetrics update_policy(const RolloutBatch& batch, int64_t update_index);
    torch::Tensor stack_observations() const;
    float benchmark_inference_latency_ms();

    config::TrainerConfig config_;
    torch::Device device_{torch::kCPU};
    PolicyValueModel model_{nullptr};
    std::unique_ptr<torch::optim::Adam> optimizer_;

    std::string environment_name_;
    int64_t observation_dim_ = 0;
    int64_t action_dim_ = 0;
    std::vector<std::unique_ptr<env::Environment>> environments_;

    std::vector<torch::Tensor> current_observations_;
    std::vector<float> episode_returns_;
    std::vector<int64_t> episode_lengths_;
    std::vector<float> finished_returns_;
    std::vector<int64_t> finished_lengths_;
    std::vector<float> finished_successes_;
    std::vector<EpisodeRecord> pending_episode_records_;

    int64_t next_episode_index_ = 1;
    int64_t total_env_steps_ = 0;
    float cached_inference_latency_ms_ = 0.0f;
    float parameter_count_k_ = 0.0f;
};

}  // namespace nmc::domain::ppo
