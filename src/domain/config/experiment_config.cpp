#include "domain/config/experiment_config.h"

#include <sstream>

#include "common/json_utils.h"

namespace nmc::domain::config {
namespace {

std::string bool_to_json(const bool value) {
    return value ? "true" : "false";
}

}  // namespace

std::string to_json(const PPOHyperparameters& config) {
    std::ostringstream stream;
    stream << '{';
    stream << "\"rollout_steps\":" << config.rollout_steps << ',';
    stream << "\"ppo_epochs\":" << config.ppo_epochs << ',';
    stream << "\"minibatch_size\":" << config.minibatch_size << ',';
    stream << "\"gamma\":" << config.gamma << ',';
    stream << "\"gae_lambda\":" << config.gae_lambda << ',';
    stream << "\"clip_epsilon\":" << config.clip_epsilon << ',';
    stream << "\"learning_rate\":" << config.learning_rate << ',';
    stream << "\"value_loss_weight\":" << config.value_loss_weight << ',';
    stream << "\"entropy_weight\":" << config.entropy_weight << ',';
    stream << "\"max_grad_norm\":" << config.max_grad_norm << ',';
    stream << "\"value_clip_epsilon\":" << config.value_clip_epsilon << ',';
    stream << "\"target_action_std\":" << config.target_action_std << ',';
    stream << "\"std_floor_weight\":" << config.std_floor_weight;
    stream << '}';
    return stream.str();
}

std::string to_json(const TrainerConfig& config) {
    std::ostringstream stream;
    stream << '{';
    stream << "\"seed\":" << config.seed << ',';
    stream << "\"num_envs\":" << config.num_envs << ',';
    stream << "\"total_updates\":" << config.total_updates << ',';
    stream << "\"hidden_dim\":" << config.hidden_dim << ',';
    stream << "\"benchmark_iterations\":" << config.benchmark_iterations << ',';
    stream << "\"torch_num_threads\":" << config.torch_num_threads << ',';
    stream << "\"checkpoint_interval_updates\":" << config.checkpoint_interval_updates << ',';
    stream << "\"ppo\":" << to_json(config.ppo);
    stream << '}';
    return stream.str();
}

std::string to_json(const env::PointMassRewardConfig& config) {
    std::ostringstream stream;
    stream << '{';
    stream << "\"position_log_weight\":" << config.position_log_weight << ',';
    stream << "\"position_log_scale\":" << config.position_log_scale << ',';
    stream << "\"position_exp_weight\":" << config.position_exp_weight << ',';
    stream << "\"position_exp_scale\":" << config.position_exp_scale << ',';
    stream << "\"velocity_alignment_weight\":" << config.velocity_alignment_weight << ',';
    stream << "\"velocity_error_weight\":" << config.velocity_error_weight << ',';
    stream << "\"desired_velocity_scale\":" << config.desired_velocity_scale << ',';
    stream << "\"desired_velocity_limit\":" << config.desired_velocity_limit << ',';
    stream << "\"control_quadratic_weight\":" << config.control_quadratic_weight << ',';
    stream << "\"control_soft_weight\":" << config.control_soft_weight << ',';
    stream << "\"control_soft_threshold\":" << config.control_soft_threshold << ',';
    stream << "\"control_soft_scale\":" << config.control_soft_scale << ',';
    stream << "\"corridor_weight\":" << config.corridor_weight << ',';
    stream << "\"corridor_half_width\":" << config.corridor_half_width << ',';
    stream << "\"boundary_weight\":" << config.boundary_weight << ',';
    stream << "\"boundary_soft_margin\":" << config.boundary_soft_margin << ',';
    stream << "\"efficiency_bonus_weight\":" << config.efficiency_bonus_weight << ',';
    stream << "\"efficiency_velocity_weight\":" << config.efficiency_velocity_weight << ',';
    stream << "\"lyapunov_weight\":" << config.lyapunov_weight << ',';
    stream << "\"safety_projection_gain\":" << config.safety_projection_gain << ',';
    stream << "\"safety_boundary_margin\":" << config.safety_boundary_margin << ',';
    stream << "\"potential_shaping_enabled\":" << bool_to_json(config.potential_shaping_enabled) << ',';
    stream << "\"potential_position_weight\":" << config.potential_position_weight << ',';
    stream << "\"potential_velocity_weight\":" << config.potential_velocity_weight << ',';
    stream << "\"potential_gamma\":" << config.potential_gamma << ',';
    stream << "\"success_bonus\":" << config.success_bonus;
    stream << '}';
    return stream.str();
}

std::string to_json(const TrainConfig& config) {
    std::ostringstream stream;
    stream << '{';
    stream << "\"run_id\":\"" << common::json_escape(config.run_id) << "\",";
    stream << "\"environment\":\"" << common::json_escape(config.environment) << "\",";
    stream << "\"deterministic_live_rollout\":" << bool_to_json(config.deterministic_live_rollout) << ',';
    stream << "\"live_rollout_steps\":" << config.live_rollout_steps << ',';
    stream << "\"resume_checkpoint\":"
           << (config.resume_checkpoint.has_value()
                   ? "\"" + common::json_escape(config.resume_checkpoint->string()) + "\""
                   : "null")
           << ',';
    stream << "\"mujoco_model_path\":"
           << (config.mujoco_model_path.has_value()
                   ? "\"" + common::json_escape(config.mujoco_model_path->string()) + "\""
                   : "null")
           << ',';
    stream << "\"point_mass_reward\":" << to_json(config.point_mass_reward) << ',';
    stream << "\"trainer\":" << to_json(config.trainer);
    stream << '}';
    return stream.str();
}

std::string to_json(const EvalConfig& config) {
    std::ostringstream stream;
    stream << '{';
    stream << "\"run_id\":\"" << common::json_escape(config.run_id) << "\",";
    stream << "\"environment\":\"" << common::json_escape(config.environment) << "\",";
    stream << "\"checkpoint_path\":\"" << common::json_escape(config.checkpoint_path.string()) << "\",";
    stream << "\"episodes\":" << config.episodes << ',';
    stream << "\"max_steps\":" << config.max_steps << ',';
    stream << "\"seed\":" << config.seed << ',';
    stream << "\"deterministic_policy\":" << bool_to_json(config.deterministic_policy) << ',';
    stream << "\"inference_backend\":\"" << common::json_escape(config.inference_backend) << "\",";
    stream << "\"mujoco_model_path\":"
           << (config.mujoco_model_path.has_value()
                   ? "\"" + common::json_escape(config.mujoco_model_path->string()) + "\""
                   : "null")
           << ',';
    stream << "\"point_mass_reward\":" << to_json(config.point_mass_reward);
    stream << '}';
    return stream.str();
}

std::string to_json(const BenchmarkConfig& config) {
    std::ostringstream stream;
    stream << '{';
    stream << "\"benchmark_name\":\"" << common::json_escape(config.benchmark_name) << "\",";
    stream << "\"seed\":" << config.seed << ',';
    stream << "\"quick\":" << bool_to_json(config.quick);
    stream << '}';
    return stream.str();
}

}  // namespace nmc::domain::config
