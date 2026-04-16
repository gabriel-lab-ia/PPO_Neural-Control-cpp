#include "domain/config/config_validation.h"

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "domain/env/environment_factory.h"
#include "domain/inference/inference_backend_factory.h"

namespace nmc::domain::config {
namespace {

void throw_if_errors(const std::vector<std::string>& errors, const char* config_name) {
    if (errors.empty()) {
        return;
    }

    std::ostringstream stream;
    stream << config_name << " validation failed:";
    for (const auto& error : errors) {
        stream << "\n- " << error;
    }
    throw std::runtime_error(stream.str());
}

}  // namespace

void validate_train_config_or_throw(const TrainConfig& config) {
    std::vector<std::string> errors;
    if (!env::try_parse_environment_kind(config.environment).has_value()) {
        errors.push_back(
            "unsupported --env '" + config.environment + "' (supported: " + env::supported_environment_kinds() + ")"
        );
    }
    if (config.trainer.seed < 0) {
        errors.push_back("--seed must be >= 0");
    }
    if (config.trainer.num_envs <= 0) {
        errors.push_back("--num-envs must be > 0");
    }
    if (config.trainer.total_updates <= 0) {
        errors.push_back("--updates must be > 0");
    }
    if (config.trainer.hidden_dim <= 0) {
        errors.push_back("--hidden-dim must be > 0");
    }
    if (config.trainer.ppo.rollout_steps <= 0) {
        errors.push_back("--rollout-steps must be > 0");
    }
    if (config.trainer.ppo.ppo_epochs <= 0) {
        errors.push_back("--ppo-epochs must be > 0");
    }
    if (config.trainer.ppo.minibatch_size <= 0) {
        errors.push_back("--minibatch-size must be > 0");
    }
    if (config.trainer.ppo.learning_rate <= 0.0f) {
        errors.push_back("--learning-rate must be > 0");
    }
    if (config.live_rollout_steps <= 0) {
        errors.push_back("--live-steps must be > 0");
    }
    if (config.resume_checkpoint.has_value() && config.resume_checkpoint->empty()) {
        errors.push_back("--resume-checkpoint path is empty");
    }
    if (config.mujoco_model_path.has_value() && config.mujoco_model_path->empty()) {
        errors.push_back("--mujoco-model path is empty");
    }
    if (config.point_mass_reward.position_log_weight < 0.0f ||
        config.point_mass_reward.position_exp_weight < 0.0f ||
        config.point_mass_reward.velocity_alignment_weight < 0.0f ||
        config.point_mass_reward.velocity_error_weight < 0.0f ||
        config.point_mass_reward.control_quadratic_weight < 0.0f ||
        config.point_mass_reward.control_soft_weight < 0.0f ||
        config.point_mass_reward.corridor_weight < 0.0f ||
        config.point_mass_reward.boundary_weight < 0.0f) {
        errors.push_back("point_mass_reward weights must be >= 0");
    }
    if (config.point_mass_reward.control_soft_threshold < 0.0f ||
        config.point_mass_reward.control_soft_threshold > 1.0f) {
        errors.push_back("point_mass_reward.control_soft_threshold must be in [0, 1]");
    }
    if (config.point_mass_reward.safety_projection_gain < 0.0f || config.point_mass_reward.safety_projection_gain > 1.5f) {
        errors.push_back("point_mass_reward.safety_projection_gain must be in [0, 1.5]");
    }
    if (config.point_mass_reward.safety_boundary_margin <= 0.0f || config.point_mass_reward.safety_boundary_margin > 1.0f) {
        errors.push_back("point_mass_reward.safety_boundary_margin must be in (0, 1]");
    }
    if (config.point_mass_reward.potential_gamma <= 0.0f || config.point_mass_reward.potential_gamma > 1.0f) {
        errors.push_back("point_mass_reward.potential_gamma must be in (0, 1]");
    }
    throw_if_errors(errors, "train config");
}

void validate_eval_config_or_throw(const EvalConfig& config) {
    std::vector<std::string> errors;
    if (!config.environment.empty() && !env::try_parse_environment_kind(config.environment).has_value()) {
        errors.push_back(
            "unsupported --env '" + config.environment + "' (supported: " + env::supported_environment_kinds() + ")"
        );
    }
    if (!inference::try_parse_inference_backend(config.inference_backend).has_value()) {
        errors.push_back(
            "unsupported --backend '" + config.inference_backend +
            "' (supported: " + inference::supported_inference_backends() + ")"
        );
    }
    if (config.seed < 0) {
        errors.push_back("--seed must be >= 0");
    }
    if (config.episodes <= 0) {
        errors.push_back("--episodes must be > 0");
    }
    if (config.max_steps <= 0) {
        errors.push_back("--max-steps must be > 0");
    }
    if (config.mujoco_model_path.has_value() && config.mujoco_model_path->empty()) {
        errors.push_back("--mujoco-model path is empty");
    }
    if (config.point_mass_reward.safety_projection_gain < 0.0f || config.point_mass_reward.safety_projection_gain > 1.5f) {
        errors.push_back("point_mass_reward.safety_projection_gain must be in [0, 1.5]");
    }
    if (config.point_mass_reward.potential_gamma <= 0.0f || config.point_mass_reward.potential_gamma > 1.0f) {
        errors.push_back("point_mass_reward.potential_gamma must be in (0, 1]");
    }
    throw_if_errors(errors, "eval config");
}

void validate_benchmark_config_or_throw(const BenchmarkConfig& config) {
    std::vector<std::string> errors;
    if (config.benchmark_name.empty()) {
        errors.push_back("--name must not be empty");
    }
    if (config.seed < 0) {
        errors.push_back("--seed must be >= 0");
    }
    throw_if_errors(errors, "benchmark config");
}

}  // namespace nmc::domain::config
