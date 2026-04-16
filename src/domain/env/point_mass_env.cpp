#include "domain/env/point_mass_env.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

namespace nmc::domain::env {
namespace {

constexpr float kDt = 0.10f;
constexpr float kAccelerationScale = 0.22f;
constexpr float kDamping = 0.05f;
constexpr float kPositionLimit = 2.50f;
constexpr float kTargetTolerance = 0.10f;
constexpr int64_t kMaxSteps = 120;
constexpr int64_t kStableStepGoal = 6;
constexpr float kAlignmentEpsilon = 1.0e-4f;

float sample_uniform(std::mt19937_64& rng, const float low, const float high) {
    std::uniform_real_distribution<float> distribution(low, high);
    return distribution(rng);
}

float stable_softplus(const float value, const float scale) {
    const float scaled = std::max(std::min(value * scale, 40.0f), -40.0f);
    return std::log1p(std::exp(scaled)) / scale;
}

}  // namespace

PointMassEnv::PointMassEnv(PointMassRewardConfig reward_config)
    : reward_config_(std::move(reward_config)),
      safety_shield_(
          {
              .max_action_magnitude = 1.0f,
              .boundary_margin = reward_config_.safety_boundary_margin,
              .projection_gain = reward_config_.safety_projection_gain
          }
      ) {
    reset();
}

torch::Tensor PointMassEnv::reset() {
    position_ = sample_uniform(rng_, -1.0f, 1.0f);
    velocity_ = sample_uniform(rng_, -0.10f, 0.10f);
    target_ = sample_uniform(rng_, -0.9f, 0.9f);
    step_count_ = 0;
    stable_steps_ = 0;
    previous_potential_ = potential(target_ - position_, velocity_);
    return make_observation();
}

void PointMassEnv::set_seed(const uint64_t seed) {
    rng_.seed(seed);
}

StepResult PointMassEnv::step(const torch::Tensor& action) {
    const auto bounded_action = torch::clamp(action, -1.0f, 1.0f).to(torch::kCPU);
    const float projected_force = safety_shield_.project_1d(
        bounded_action[0].item<float>(),
        position_,
        velocity_,
        kPositionLimit
    );
    const float previous_potential = previous_potential_;

    velocity_ += (kAccelerationScale * projected_force) - (kDamping * velocity_);
    position_ += velocity_ * kDt;
    position_ = std::clamp(position_, -kPositionLimit, kPositionLimit);
    ++step_count_;

    const float position_error = target_ - position_;

    if (std::abs(position_error) < kTargetTolerance) {
        ++stable_steps_;
    } else {
        stable_steps_ = 0;
    }

    const bool terminated = stable_steps_ >= kStableStepGoal;
    const bool truncated = step_count_ >= kMaxSteps;

    float reward = compute_reward(projected_force, position_error, previous_potential);
    if (terminated) {
        reward += reward_config_.success_bonus;
    }

    return {
        make_observation(),
        reward,
        terminated,
        truncated
    };
}

int64_t PointMassEnv::observation_dim() const {
    return 4;
}

int64_t PointMassEnv::action_dim() const {
    return 1;
}

std::string PointMassEnv::name() const {
    return "PointMassEnv";
}

float PointMassEnv::success_signal(const StepResult& result) const {
    return result.terminated ? 1.0f : 0.0f;
}

float PointMassEnv::compute_reward(
    const float force,
    const float position_error,
    const float previous_potential
) {
    // Position shaping: combined log-penalty + exponential bonus gives smoother gradients
    // near target while still discouraging large orbital-state deviations.
    const float position_error_sq = position_error * position_error;
    const float position_cost =
        reward_config_.position_log_weight *
        std::log1p(reward_config_.position_log_scale * position_error_sq);
    const float position_bonus =
        reward_config_.position_exp_weight *
        std::exp(-reward_config_.position_exp_scale * std::abs(position_error));

    // Velocity shaping: match velocity direction/magnitude to target approach profile.
    const float desired_velocity = std::clamp(
        reward_config_.desired_velocity_scale * position_error,
        -reward_config_.desired_velocity_limit,
        reward_config_.desired_velocity_limit
    );
    const float velocity_error = velocity_ - desired_velocity;
    const float alignment_denominator =
        (std::abs(velocity_) * std::abs(desired_velocity)) + kAlignmentEpsilon;
    const float velocity_alignment = (velocity_ * desired_velocity) / alignment_denominator;
    const float velocity_cost = reward_config_.velocity_error_weight * velocity_error * velocity_error;
    const float velocity_bonus = reward_config_.velocity_alignment_weight * velocity_alignment;

    // Fuel/control shaping: quadratic control cost + soft constraint for aggressive thrust.
    const float control_cost = reward_config_.control_quadratic_weight * force * force;
    const float control_excess = std::max(std::abs(force) - reward_config_.control_soft_threshold, 0.0f);
    const float control_soft_cost =
        reward_config_.control_soft_weight *
        stable_softplus(control_excess, reward_config_.control_soft_scale);

    // Safety corridor: penalize trajectory outside corridor and near state boundaries.
    const float corridor_excess = std::max(std::abs(position_error) - reward_config_.corridor_half_width, 0.0f);
    const float corridor_cost = reward_config_.corridor_weight * corridor_excess * corridor_excess;
    const float boundary_distance = kPositionLimit - std::abs(position_);
    const float boundary_excess = std::max(reward_config_.boundary_soft_margin - boundary_distance, 0.0f);
    const float boundary_cost = reward_config_.boundary_weight * boundary_excess * boundary_excess;

    // Efficiency bonus: favors low-thrust, low-kinetic maneuvering near target.
    const float efficiency_bonus =
        reward_config_.efficiency_bonus_weight *
        std::exp(-(force * force + reward_config_.efficiency_velocity_weight * velocity_ * velocity_));

    const float lyapunov = position_error * position_error + 0.5f * velocity_ * velocity_;
    const float lyapunov_cost = reward_config_.lyapunov_weight * lyapunov;

    float potential_shaping = 0.0f;
    const float current_potential = potential(position_error, velocity_);
    if (reward_config_.potential_shaping_enabled) {
        potential_shaping = reward_config_.potential_gamma * current_potential - previous_potential;
    }
    previous_potential_ = current_potential;

    return
        position_bonus -
        position_cost +
        velocity_bonus -
        velocity_cost -
        control_cost -
        control_soft_cost -
        corridor_cost -
        boundary_cost +
        efficiency_bonus +
        potential_shaping -
        lyapunov_cost;
}

float PointMassEnv::potential(const float position_error, const float velocity) const {
    return -(
        reward_config_.potential_position_weight * position_error * position_error +
        reward_config_.potential_velocity_weight * velocity * velocity
    );
}

torch::Tensor PointMassEnv::make_observation() const {
    return torch::tensor(
        {
            position_,
            velocity_,
            target_,
            target_ - position_
        },
        torch::TensorOptions().dtype(torch::kFloat32)
    );
}

}  // namespace nmc::domain::env
