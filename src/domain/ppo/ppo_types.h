#pragma once

#include <cstdint>
#include <vector>

#include <torch/torch.h>

namespace nmc::domain::ppo {

struct RolloutBatch {
    torch::Tensor observations;
    torch::Tensor actions;
    torch::Tensor log_probs;
    torch::Tensor rewards;
    torch::Tensor returns;
    torch::Tensor advantages;
    torch::Tensor values;
};

struct EpisodeRecord {
    int64_t episode_index = 0;
    int64_t env_steps = 0;
    float episode_return = 0.0f;
    int64_t episode_length = 0;
    float success = 0.0f;
};

struct TrainingMetrics {
    int64_t update = 0;
    int64_t env_steps = 0;
    float policy_loss = 0.0f;
    float value_loss = 0.0f;
    float entropy = 0.0f;
    float approx_kl = 0.0f;
    float clip_fraction = 0.0f;
    float avg_step_reward = 0.0f;
    float avg_episode_return = 0.0f;
    float avg_episode_length = 0.0f;
    float success_rate = 0.0f;
    float action_std = 0.0f;
    float explained_variance = 0.0f;
    float update_time_ms = 0.0f;
    float samples_per_second = 0.0f;
    float inference_latency_ms = 0.0f;
    float parameter_count_k = 0.0f;
    int64_t completed_episodes = 0;
};

struct LiveStep {
    int64_t step = 0;
    float reward = 0.0f;
    float action = 0.0f;
    float value = 0.0f;
    std::vector<float> observation;
    bool terminated = false;
    bool truncated = false;
};

}  // namespace nmc::domain::ppo
