#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "domain/env/environment.h"

namespace nmc::domain::env {

struct EnvironmentSpec {
    std::string kind = "point_mass";
    std::optional<std::filesystem::path> mujoco_model_path;
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
