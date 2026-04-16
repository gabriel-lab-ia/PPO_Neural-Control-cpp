#pragma once

#include <random>

#include <torch/torch.h>

#include "domain/env/action_safety_shield.h"
#include "domain/env/environment.h"
#include "orbital/sim/orbital_dynamics.hpp"

namespace nmc::domain::env {

class OrbitalSixDofEnv final : public Environment {
public:
    OrbitalSixDofEnv();

    torch::Tensor reset() override;
    StepResult step(const torch::Tensor& action) override;
    void set_seed(uint64_t seed) override;

    int64_t observation_dim() const override;
    int64_t action_dim() const override;
    std::string name() const override;
    float success_signal(const StepResult& result) const override;

private:
    torch::Tensor make_observation() const;
    orbital::sim::OrbitalControlCommand6DOF project_action(const torch::Tensor& action);
    float compute_reward(const orbital::sim::OrbitalControlCommand6DOF& command, bool safety_violation) const;

    orbital::sim::OrbitalDynamicsConfig6DOF dynamics_config_{};
    orbital::sim::HighFidelityOrbitalDynamics6DOF dynamics_{dynamics_config_};
    orbital::sim::OrbitalBodyState6DOF state_{};

    ActionSafetyShield translational_shield_{};
    std::mt19937_64 rng_{7};

    int64_t step_count_ = 0;
    int64_t stable_steps_ = 0;
    bool last_safety_violation_ = false;
};

}  // namespace nmc::domain::env
