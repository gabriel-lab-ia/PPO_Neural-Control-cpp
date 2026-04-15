#pragma once

#include <filesystem>
#include <fstream>

#include "domain/ppo/ppo_types.h"

namespace nmc::infrastructure::reporting {

class MetricsCsvWriter {
public:
    explicit MetricsCsvWriter(const std::filesystem::path& path);
    ~MetricsCsvWriter();

    void append(const domain::ppo::TrainingMetrics& metrics);

private:
    std::ofstream stream_;
};

}  // namespace nmc::infrastructure::reporting
