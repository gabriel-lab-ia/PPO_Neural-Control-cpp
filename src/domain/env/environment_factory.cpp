#include "domain/env/environment_factory.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <string>

#include "domain/env/point_mass_env.h"
#include "domain/env/orbital_6dof_env.h"

#if defined(NMC_ENABLE_MUJOCO)
#include "domain/env/mujoco_cartpole_env.h"
#endif

namespace nmc::domain::env {
namespace {

std::string normalize_kind_string(std::string_view value) {
    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return normalized;
}

std::unique_ptr<Environment> make_single_environment(const EnvironmentSpec& spec) {
    if (spec.kind == EnvironmentKind::kPointMass) {
        if (spec.point_mass_reward.has_value()) {
            return std::make_unique<PointMassEnv>(*spec.point_mass_reward);
        }
        return std::make_unique<PointMassEnv>();
    }

    if (spec.kind == EnvironmentKind::kOrbitalSixDof) {
        return std::make_unique<OrbitalSixDofEnv>();
    }

#if defined(NMC_ENABLE_MUJOCO)
    if (spec.kind == EnvironmentKind::kMuJoCoCartPole) {
        const auto model_path = spec.mujoco_model_path.value_or(std::filesystem::path("assets/mujoco/cartpole.xml"));
        return std::make_unique<MuJoCoCartPoleEnv>(model_path);
    }
#endif

    throw std::runtime_error("unsupported environment kind");
}

std::string display_name_for(const EnvironmentSpec& spec) {
    switch (spec.kind) {
        case EnvironmentKind::kPointMass:
            return "PointMassEnv";
        case EnvironmentKind::kOrbitalSixDof:
            return "OrbitalSixDofEnv";
        case EnvironmentKind::kMuJoCoCartPole:
            return "MuJoCoCartPoleEnv";
    }
    return "UnknownEnv";
}

}  // namespace

std::optional<EnvironmentKind> try_parse_environment_kind(std::string_view value) {
    const auto normalized = normalize_kind_string(value);
    if (normalized == "point_mass") {
        return EnvironmentKind::kPointMass;
    }
    if (normalized == "orbital_6dof" || normalized == "sixdof") {
        return EnvironmentKind::kOrbitalSixDof;
    }
    if (normalized == "mujoco_cartpole") {
        return EnvironmentKind::kMuJoCoCartPole;
    }
    return std::nullopt;
}

EnvironmentKind parse_environment_kind_or_throw(const std::string_view value) {
    const auto parsed = try_parse_environment_kind(value);
    if (!parsed.has_value()) {
        std::ostringstream stream;
        stream << "unsupported environment kind: " << value
               << " (supported: " << supported_environment_kinds() << ")";
        throw std::runtime_error(stream.str());
    }

#if !defined(NMC_ENABLE_MUJOCO)
    if (*parsed == EnvironmentKind::kMuJoCoCartPole) {
        throw std::runtime_error(
            "mujoco_cartpole is unavailable in this build (configure with -DNMC_ENABLE_MUJOCO=ON)"
        );
    }
#endif

    return *parsed;
}

std::string environment_kind_to_string(const EnvironmentKind kind) {
    switch (kind) {
        case EnvironmentKind::kPointMass:
            return "point_mass";
        case EnvironmentKind::kOrbitalSixDof:
            return "orbital_6dof";
        case EnvironmentKind::kMuJoCoCartPole:
            return "mujoco_cartpole";
    }
    return "unknown";
}

std::string supported_environment_kinds() {
#if defined(NMC_ENABLE_MUJOCO)
    return "point_mass|orbital_6dof|mujoco_cartpole";
#else
    return "point_mass|orbital_6dof";
#endif
}

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

    if (spec.seed.has_value()) {
        for (int64_t index = 0; index < num_envs; ++index) {
            const auto seed = *spec.seed + static_cast<uint64_t>(index);
            pack.environments[static_cast<std::size_t>(index)]->set_seed(seed);
        }
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
