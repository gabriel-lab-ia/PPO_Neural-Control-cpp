#pragma once

#include "domain/env/environment.h"

namespace nmc::domain::env {

class PointMassEnv final : public Environment {
public:
    PointMassEnv();

    torch::Tensor reset() override;
    StepResult step(const torch::Tensor& action) override;
    int64_t observation_dim() const override;
    int64_t action_dim() const override;
    std::string name() const override;
    float success_signal(const StepResult& result) const override;

private:
    torch::Tensor make_observation() const;

    float position_ = 0.0f;
    float velocity_ = 0.0f;
    float target_ = 0.0f;
    int64_t step_count_ = 0;
    int64_t stable_steps_ = 0;
};

}  // namespace nmc::domain::env
