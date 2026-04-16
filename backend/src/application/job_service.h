#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "domain/types.h"

namespace orbital::backend::application {

struct JobLaunchRequest {
    domain::JobType type = domain::JobType::Benchmark;
    std::string run_id;
    std::int64_t seed = 7;
    bool quick = true;
};

class JobService {
public:
    JobService(std::filesystem::path repository_root, bool executor_enabled);
    ~JobService();

    JobService(const JobService&) = delete;
    JobService& operator=(const JobService&) = delete;

    domain::JobRecord submit(const JobLaunchRequest& request);
    std::optional<domain::JobRecord> get(const std::string& job_id) const;

private:
    void run_job(std::string job_id, JobLaunchRequest request);

    std::filesystem::path repository_root_;
    bool executor_enabled_ = false;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, domain::JobRecord> jobs_;
    std::vector<std::jthread> workers_;
};

}  // namespace orbital::backend::application
