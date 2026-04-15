#pragma once

#include <filesystem>
#include <string>

#include "domain/config/experiment_config.h"

namespace nmc::application {

struct BenchmarkRunOutput {
    std::string benchmark_id;
    std::string train_run_id;
    std::string eval_run_id;
    std::filesystem::path summary_json_path;
    std::filesystem::path summary_csv_path;
    bool artifacts_valid = false;
};

class BenchmarkRunner {
public:
    explicit BenchmarkRunner(std::filesystem::path artifact_root = "artifacts");

    BenchmarkRunOutput run(const domain::config::BenchmarkConfig& config);

private:
    std::filesystem::path artifact_root_;
};

}  // namespace nmc::application
