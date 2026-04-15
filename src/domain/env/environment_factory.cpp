#include "domain/env/environment_factory.h"

#include <filesystem>
#include <stdexcept>

#include "domain/env/point_mass_env.h"

#if defined(NMC_ENABLE_MUJOCO)
#include "domain/env/mujoco_cartpole_env.h"
#endif

namespace nmc::domain::env {
namespace {

std::unique_ptr<Environment> make_single_environment(const EnvironmentSpec& spec) {
    if (spec.kind == "point_mass") {
        return std::make_unique<PointMassEnv>();
    }

#if defined(NMC_ENABLE_MUJOCO)
    if (spec.kind == "mujoco_cartpole") {
        const auto model_path = spec.mujoco_model_path.value_or(std::filesystem::path("assets/mujoco/cartpole.xml"));
        return std::make_unique<MuJoCoCartPoleEnv>(model_path);
    }
#endif

    throw std::runtime_error("unsupported environment kind: " + spec.kind);
}

std::string display_name_for(const EnvironmentSpec& spec) {
    if (spec.kind == "point_mass") {
        return "PointMassEnv";
    }
    if (spec.kind == "mujoco_cartpole") {
        return "MuJoCoCartPoleEnv";
    }
    return spec.kind;
}

}  // namespace

EnvironmentPack make_environment_pack(const EnvironmentSpec& spec, const int64_t num_envs) {
    if (num_envs <= 0) {
        throw std::runtime_error("num_envs must be positive");
    }

    auto first_environment = make_single_environment(spec);
    EnvironmentPack pack;
    pack.display_name = display_name_for(spec);
    pack.observation_dim = first_environment->observation_dim();
    pack.action_dim = first_environment->action_dim();
    pack.environments.reserve(static_cast<std::size_t>(num_envs));
    pack.environments.push_back(std::move(first_environment));

    for (int64_t index = 1; index < num_envs; ++index) {
        pack.environments.push_back(make_single_environment(spec));
    }

    return pack;
}

bool mujoco_support_enabled() {
#if defined(NMC_ENABLE_MUJOCO)
    return true;
#else
    return false;
#endif
}

}  // namespace nmc::domain::env
