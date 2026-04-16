#include "domain/env/orbital_6dof_env.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <random>

namespace nmc::domain::env {
namespace {

constexpr double kDtSeconds = 0.25;
constexpr int64_t kMaxSteps = 720;
constexpr int64_t kStableStepGoal = 20;
constexpr double kTargetRadiusM = 6'878'000.0;
constexpr double kTargetSpeedMps = 7'612.0;
constexpr double kMaxThrustN = 0.80;
constexpr double kMaxTorqueNm = 0.08;
constexpr double kRateLimitRps = 0.045;
constexpr double kPositionToleranceM = 250.0;
constexpr double kVelocityToleranceMps = 2.5;

double vector_norm(const std::array<double, 3>& value) {
    return std::sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2]);
}

double clamp_unit(const double value) {
    return std::clamp(value, -1.0, 1.0);
}

}  // namespace

OrbitalSixDofEnv::OrbitalSixDofEnv()
    : translational_shield_(
          {
              .max_action_magnitude = 1.0f,
              .boundary_margin = 0.25f,
              .projection_gain = 0.80f
          }
      ) {
    reset();
}

torch::Tensor OrbitalSixDofEnv::reset() {
    std::uniform_real_distribution<double> phase_dist(-0.025, 0.025);
    std::uniform_real_distribution<double> z_dist(-45.0, 45.0);
    std::uniform_real_distribution<double> vz_dist(-0.10, 0.10);

    const double phase = phase_dist(rng_);
    const double x = kTargetRadiusM * std::cos(phase);
    const double y = kTargetRadiusM * std::sin(phase);

    state_ = {};
    state_.position_m = {x, y, z_dist(rng_)};
    state_.velocity_mps = {-kTargetSpeedMps * std::sin(phase), kTargetSpeedMps * std::cos(phase), vz_dist(rng_)};
    state_.attitude_wxyz = {1.0, 0.0, 0.0, 0.0};
    state_.angular_rate_rps = {0.0, 0.0, 0.0};
    state_.mission_time_s = 0.0;

    step_count_ = 0;
    stable_steps_ = 0;
    last_safety_violation_ = false;

    return make_observation();
}

StepResult OrbitalSixDofEnv::step(const torch::Tensor& action) {
    const auto projected = project_action(action);
    state_ = dynamics_.propagate(state_, projected, kDtSeconds);
    ++step_count_;

    const double radius = vector_norm(state_.position_m);
    const double speed = vector_norm(state_.velocity_mps);

    const double radius_error = std::abs(radius - kTargetRadiusM);
    const double speed_error = std::abs(speed - kTargetSpeedMps);
    const double rate = vector_norm(state_.angular_rate_rps);

    if (radius_error < kPositionToleranceM && speed_error < kVelocityToleranceMps && rate < kRateLimitRps) {
        ++stable_steps_;
    } else {
        stable_steps_ = 0;
    }

    const bool terminated = stable_steps_ >= kStableStepGoal;
    const bool truncated = step_count_ >= kMaxSteps;

    const float reward = compute_reward(projected, last_safety_violation_);

    return {
        make_observation(),
        reward,
        terminated,
        truncated
    };
}

void OrbitalSixDofEnv::set_seed(const uint64_t seed) {
    rng_.seed(seed);
}

int64_t OrbitalSixDofEnv::observation_dim() const {
    return 16;
}

int64_t OrbitalSixDofEnv::action_dim() const {
    return 6;
}

std::string OrbitalSixDofEnv::name() const {
    return "OrbitalSixDofEnv";
}

float OrbitalSixDofEnv::success_signal(const StepResult& result) const {
    return result.terminated ? 1.0f : 0.0f;
}

torch::Tensor OrbitalSixDofEnv::make_observation() const {
    const double radius = vector_norm(state_.position_m);
    const double speed = vector_norm(state_.velocity_mps);

    return torch::tensor(
        {
            static_cast<float>(state_.position_m[0]),
            static_cast<float>(state_.position_m[1]),
            static_cast<float>(state_.position_m[2]),
            static_cast<float>(state_.velocity_mps[0]),
            static_cast<float>(state_.velocity_mps[1]),
            static_cast<float>(state_.velocity_mps[2]),
            static_cast<float>(state_.attitude_wxyz[0]),
            static_cast<float>(state_.attitude_wxyz[1]),
            static_cast<float>(state_.attitude_wxyz[2]),
            static_cast<float>(state_.attitude_wxyz[3]),
            static_cast<float>(state_.angular_rate_rps[0]),
            static_cast<float>(state_.angular_rate_rps[1]),
            static_cast<float>(state_.angular_rate_rps[2]),
            static_cast<float>((radius - kTargetRadiusM) / 5'000.0),
            static_cast<float>((speed - kTargetSpeedMps) / 100.0),
            static_cast<float>(last_safety_violation_ ? 1.0 : 0.0),
        },
        torch::TensorOptions().dtype(torch::kFloat32)
    );
}

orbital::sim::OrbitalControlCommand6DOF OrbitalSixDofEnv::project_action(const torch::Tensor& action) {
    const auto bounded = torch::clamp(action, -1.0f, 1.0f).to(torch::kCPU);

    orbital::sim::OrbitalControlCommand6DOF command;
    last_safety_violation_ = false;

    const double radius = vector_norm(state_.position_m);
    const float normalized_boundary = static_cast<float>((radius - dynamics_config_.earth_radius_m) / 1'000'000.0);

    for (std::size_t axis = 0; axis < command.thrust_body_n.size(); ++axis) {
        const float raw = bounded[static_cast<int64_t>(axis)].item<float>();
        const float safe = translational_shield_.project_1d(raw, normalized_boundary, 0.0f, 1.0f);
        command.thrust_body_n[axis] = kMaxThrustN * clamp_unit(safe);
    }

    for (std::size_t axis = 0; axis < command.torque_body_nm.size(); ++axis) {
        const float raw = bounded[static_cast<int64_t>(axis) + 3].item<float>();
        const double requested = kMaxTorqueNm * clamp_unit(raw);
        const double omega = state_.angular_rate_rps[axis];

        if (std::abs(omega) > kRateLimitRps && (omega * requested) > 0.0) {
            command.torque_body_nm[axis] = 0.0;
            last_safety_violation_ = true;
        } else {
            command.torque_body_nm[axis] = requested;
        }
    }

    return command;
}

float OrbitalSixDofEnv::compute_reward(
    const orbital::sim::OrbitalControlCommand6DOF& command,
    const bool safety_violation
) const {
    const double radius = vector_norm(state_.position_m);
    const double speed = vector_norm(state_.velocity_mps);
    const double rate = vector_norm(state_.angular_rate_rps);

    const double radius_error = radius - kTargetRadiusM;
    const double speed_error = speed - kTargetSpeedMps;

    const double thrust_mag = vector_norm(command.thrust_body_n);
    const double torque_mag = vector_norm(command.torque_body_nm);

    const double position_term = std::exp(-std::abs(radius_error) / 450.0);
    const double velocity_term = std::exp(-std::abs(speed_error) / 4.0);
    const double control_cost = 0.30 * thrust_mag * thrust_mag + 0.15 * torque_mag * torque_mag;
    const double attitude_cost = 0.20 * rate * rate;
    const double lyapunov = (radius_error * radius_error) / (5'000.0 * 5'000.0) + (speed_error * speed_error) / (60.0 * 60.0);
    const double safety_cost = safety_violation ? 0.8 : 0.0;

    const double reward =
        1.50 * position_term +
        1.00 * velocity_term -
        control_cost -
        attitude_cost -
        0.60 * lyapunov -
        safety_cost;

    return static_cast<float>(reward);
}

}  // namespace nmc::domain::env
