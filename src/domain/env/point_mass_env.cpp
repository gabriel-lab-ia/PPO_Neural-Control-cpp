#include "domain/env/point_mass_env.h"

#include <algorithm>
#include <cmath>

namespace nmc::domain::env {
namespace {

constexpr float kDt = 0.10f;
constexpr float kAccelerationScale = 0.22f;
constexpr float kDamping = 0.05f;
constexpr float kPositionLimit = 2.50f;
constexpr float kTargetTolerance = 0.10f;
constexpr int64_t kMaxSteps = 120;
constexpr int64_t kStableStepGoal = 6;

float sample_uniform(const float low, const float high) {
    const auto value = torch::rand({1}, torch::TensorOptions().dtype(torch::kFloat32));
    return low + (high - low) * value.item<float>();
}

}  // namespace

PointMassEnv::PointMassEnv() {
    reset();
}

torch::Tensor PointMassEnv::reset() {
    position_ = sample_uniform(-1.0f, 1.0f);
    velocity_ = sample_uniform(-0.10f, 0.10f);
    target_ = sample_uniform(-0.9f, 0.9f);
    step_count_ = 0;
    stable_steps_ = 0;
    return make_observation();
}

StepResult PointMassEnv::step(const torch::Tensor& action) {
    const auto safe_action = torch::clamp(action, -1.0f, 1.0f).to(torch::kCPU);
    const float force = safe_action[0].item<float>();

    velocity_ += (kAccelerationScale * force) - (kDamping * velocity_);
    position_ += velocity_ * kDt;
    position_ = std::clamp(position_, -kPositionLimit, kPositionLimit);
    ++step_count_;

    const float distance = target_ - position_;
    const float shaping =
        1.0f -
        (0.55f * distance * distance) -
        (0.03f * force * force) -
        (0.02f * velocity_ * velocity_);

    if (std::abs(distance) < kTargetTolerance) {
        ++stable_steps_;
    } else {
        stable_steps_ = 0;
    }

    const bool terminated = stable_steps_ >= kStableStepGoal;
    const bool truncated = step_count_ >= kMaxSteps;
    const float success_bonus = terminated ? 1.50f : 0.0f;

    return {
        make_observation(),
        shaping + success_bonus,
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
