#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "domain/env/environment.h"
#include "domain/env/point_mass_reward.h"

namespace nmc::domain::env {

enum class EnvironmentKind {
    kPointMass,
    kOrbitalSixDof,
    kMuJoCoCartPole
};

std::optional<EnvironmentKind> try_parse_environment_kind(std::string_view value);
EnvironmentKind parse_environment_kind_or_throw(std::string_view value);
std::string environment_kind_to_string(EnvironmentKind kind);
std::string supported_environment_kinds();

struct EnvironmentSpec {
    EnvironmentKind kind = EnvironmentKind::kPointMass;
    std::optional<uint64_t> seed;
    std::optional<std::filesystem::path> mujoco_model_path;
    std::optional<PointMassRewardConfig> point_mass_reward;
};

struct EnvironmentPack {
    std::string display_name;
    int64_t observation_dim = 0;
    int64_t action_dim = 0;
    std::vector<std::unique_ptr<Environment>> environments;
};

EnvironmentPack make_environment_pack(const EnvironmentSpec& spec, int64_t num_envs);
bool mujoco_support_enabled();

}  // namespace nmc::domain::env
