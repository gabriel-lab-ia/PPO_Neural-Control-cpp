#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "domain/ppo/policy_value_model.h"

namespace nmc::infrastructure::artifacts {

struct CheckpointMetadata {
    std::string run_id;
    std::string environment;
    int64_t observation_dim = 0;
    int64_t action_dim = 0;
    int64_t hidden_dim = 0;
    int64_t seed = 0;
    std::string created_at;
};

std::filesystem::path metadata_path_for(const std::filesystem::path& checkpoint_path);
void save_policy_checkpoint(
    const std::filesystem::path& checkpoint_path,
    const std::filesystem::path& metadata_path,
    domain::ppo::PolicyValueModel& model,
    const CheckpointMetadata& metadata
);
void load_policy_checkpoint(const std::filesystem::path& checkpoint_path, domain::ppo::PolicyValueModel& model);
CheckpointMetadata load_checkpoint_metadata(const std::filesystem::path& metadata_path);

}  // namespace nmc::infrastructure::artifacts
