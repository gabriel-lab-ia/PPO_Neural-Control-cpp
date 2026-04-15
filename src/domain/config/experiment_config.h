#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace nmc::domain::config {

struct PPOHyperparameters {
    int64_t rollout_steps = 128;
    int64_t ppo_epochs = 5;
    int64_t minibatch_size = 192;
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
};

struct TrainerConfig {
    int64_t seed = 7;
    int64_t num_envs = 16;
    int64_t total_updates = 30;
    int64_t hidden_dim = 96;
    int64_t benchmark_iterations = 256;
    int64_t torch_num_threads = 1;
    int64_t checkpoint_interval_updates = 10;
    PPOHyperparameters ppo{};
};

struct TrainConfig {
    std::string run_id;
    std::string environment = "point_mass";
    TrainerConfig trainer{};
    bool deterministic_live_rollout = true;
    int64_t live_rollout_steps = 240;
    std::optional<std::filesystem::path> resume_checkpoint;
    std::optional<std::filesystem::path> mujoco_model_path;
};

struct EvalConfig {
    std::string run_id;
    std::string environment;
    std::filesystem::path checkpoint_path;
    int64_t episodes = 10;
    int64_t max_steps = 240;
    int64_t seed = 7;
    bool deterministic_policy = true;
    std::string inference_backend = "libtorch";
    std::optional<std::filesystem::path> mujoco_model_path;
};

struct BenchmarkConfig {
    std::string benchmark_name = "smoke";
    int64_t seed = 7;
    bool quick = true;
};

std::string to_json(const PPOHyperparameters& config);
std::string to_json(const TrainerConfig& config);
std::string to_json(const TrainConfig& config);
std::string to_json(const EvalConfig& config);
std::string to_json(const BenchmarkConfig& config);

}  // namespace nmc::domain::config
