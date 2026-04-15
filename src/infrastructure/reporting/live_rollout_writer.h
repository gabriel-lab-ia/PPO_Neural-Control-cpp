#pragma once

#include <filesystem>
#include <vector>

#include "domain/ppo/ppo_types.h"

namespace nmc::infrastructure::reporting {

void write_live_rollout_csv(
    const std::filesystem::path& path,
    const std::vector<domain::ppo::LiveStep>& steps
);

}  // namespace nmc::infrastructure::reporting
