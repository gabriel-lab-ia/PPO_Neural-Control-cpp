#include "domain/env/mujoco_cartpole_env.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace nmc::domain::env {
namespace {

constexpr float kControlScale = 1.0f;
constexpr float kCartLimit = 2.4f;
constexpr float kAngleLimit = 0.80f;
constexpr int64_t kMaxSteps = 1000;

float sample_uniform(const float low, const float high) {
    const auto value = torch::rand({1}, torch::TensorOptions().dtype(torch::kFloat32));
    return low + (high - low) * value.item<float>();
}

}  // namespace

MuJoCoCartPoleEnv::MuJoCoCartPoleEnv(const std::filesystem::path& model_path)
    : model_path_(model_path) {
    std::array<char, 1024> error{};
    model_ = mj_loadXML(model_path_.string().c_str(), nullptr, error.data(), error.size());
    if (model_ == nullptr) {
        throw std::runtime_error("failed to load MuJoCo XML: " + std::string(error.data()));
    }

    data_ = mj_makeData(model_);
    if (data_ == nullptr) {
        mj_deleteModel(model_);
        model_ = nullptr;
        throw std::runtime_error("failed to allocate MuJoCo data");
    }

    if (model_->nq < 2 || model_->nv < 2 || model_->nu < 1) {
        throw std::runtime_error("MuJoCo cartpole model must expose nq>=2, nv>=2 and nu>=1");
    }
}

MuJoCoCartPoleEnv::~MuJoCoCartPoleEnv() {
    if (data_ != nullptr) {
        mj_deleteData(data_);
    }
    if (model_ != nullptr) {
        mj_deleteModel(model_);
    }
}

torch::Tensor MuJoCoCartPoleEnv::reset() {
    mj_resetData(model_, data_);
    data_->qpos[0] = sample_uniform(-0.05f, 0.05f);
    data_->qpos[1] = sample_uniform(-0.08f, 0.08f);
    data_->qvel[0] = sample_uniform(-0.02f, 0.02f);
    data_->qvel[1] = sample_uniform(-0.02f, 0.02f);
    mj_forward(model_, data_);
    step_count_ = 0;
    return make_observation();
}

StepResult MuJoCoCartPoleEnv::step(const torch::Tensor& action) {
    const auto safe_action = torch::clamp(action, -1.0f, 1.0f).to(torch::kCPU);
    data_->ctrl[0] = static_cast<mjtNum>(kControlScale * safe_action[0].item<float>());
    mj_step(model_, data_);
    ++step_count_;

    const float cart_position = static_cast<float>(data_->qpos[0]);
    const float pole_angle = static_cast<float>(data_->qpos[1]);
    const float cart_velocity = static_cast<float>(data_->qvel[0]);
    const float pole_velocity = static_cast<float>(data_->qvel[1]);
    const float control = static_cast<float>(data_->ctrl[0]);

    const float upright = std::cos(pole_angle);
    const float reward =
        upright -
        (0.12f * cart_position * cart_position) -
        (0.01f * cart_velocity * cart_velocity) -
        (0.02f * pole_velocity * pole_velocity) -
        (0.001f * control * control);

    const bool terminated =
        std::abs(cart_position) > kCartLimit ||
        std::abs(pole_angle) > kAngleLimit;
    const bool truncated = step_count_ >= kMaxSteps;

    return {
        make_observation(),
        reward,
        terminated,
        truncated
    };
}

int64_t MuJoCoCartPoleEnv::observation_dim() const {
    return static_cast<int64_t>(model_->nq + model_->nv);
}

int64_t MuJoCoCartPoleEnv::action_dim() const {
    return static_cast<int64_t>(model_->nu);
}

std::string MuJoCoCartPoleEnv::name() const {
    return "MuJoCoCartPoleEnv";
}

float MuJoCoCartPoleEnv::success_signal(const StepResult& result) const {
    return (result.truncated && !result.terminated) ? 1.0f : 0.0f;
}

torch::Tensor MuJoCoCartPoleEnv::make_observation() const {
    std::vector<float> values;
    values.reserve(static_cast<std::size_t>(model_->nq + model_->nv));

    for (int index = 0; index < model_->nq; ++index) {
        values.push_back(static_cast<float>(data_->qpos[index]));
    }
    for (int index = 0; index < model_->nv; ++index) {
        values.push_back(static_cast<float>(data_->qvel[index]));
    }

    return torch::tensor(values, torch::TensorOptions().dtype(torch::kFloat32));
}

}  // namespace nmc::domain::env
