#pragma once

#include <cstdint>

#include <torch/torch.h>

namespace nmc {

struct RolloutBatch {
    torch::Tensor observations;
    torch::Tensor actions;
    torch::Tensor log_probs;
    torch::Tensor rewards;
    torch::Tensor returns;
    torch::Tensor advantages;
    torch::Tensor values;
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
};

struct TrainerConfig {
    int64_t seed = 7;
    int64_t num_envs = 16;
    int64_t rollout_steps = 128;
    int64_t total_updates = 30;
    int64_t ppo_epochs = 5;
    int64_t minibatch_size = 192;
    int64_t hidden_dim = 96;
    float gamma = 0.99f;
    float gae_lambda = 0.95f;
    float clip_epsilon = 0.2f;
    float learning_rate = 3.0e-4f;
    float value_loss_weight = 0.5f;
    float entropy_weight = 0.02f;
    float max_grad_norm = 0.5f;
    float value_clip_epsilon = 0.2f;
    float target_action_std = 0.45f;
    float std_floor_weight = 0.015f;
    int64_t benchmark_iterations = 256;
};

}  // namespace nmc
