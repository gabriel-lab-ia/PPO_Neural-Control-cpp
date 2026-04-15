#include "infrastructure/reporting/live_rollout_writer.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace nmc::infrastructure::reporting {

void write_live_rollout_csv(
    const std::filesystem::path& path,
    const std::vector<domain::ppo::LiveStep>& steps
) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::out | std::ios::trunc);
    if (!stream.is_open()) {
        throw std::runtime_error("unable to open live rollout file: " + path.string());
    }

    stream << "step,reward,action,value,obs0,obs1,obs2,obs3,terminated,truncated\n";
    stream << std::fixed << std::setprecision(6);

    for (const auto& step : steps) {
        const auto observation_0 = step.observation.size() > 0 ? step.observation[0] : 0.0f;
        const auto observation_1 = step.observation.size() > 1 ? step.observation[1] : 0.0f;
        const auto observation_2 = step.observation.size() > 2 ? step.observation[2] : 0.0f;
        const auto observation_3 = step.observation.size() > 3 ? step.observation[3] : 0.0f;
        stream
            << step.step << ','
            << step.reward << ','
            << step.action << ','
            << step.value << ','
            << observation_0 << ','
            << observation_1 << ','
            << observation_2 << ','
            << observation_3 << ','
            << (step.terminated ? 1 : 0) << ','
            << (step.truncated ? 1 : 0) << '\n';
    }
}

}  // namespace nmc::infrastructure::reporting
