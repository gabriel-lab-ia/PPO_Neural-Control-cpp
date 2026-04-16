#include "application/job_service.h"

#include "common/logger.h"
#include "common/time_utils.h"

#include <cstdlib>
#include <sstream>

namespace orbital::backend::application {
namespace {

std::string command_for(const JobLaunchRequest& request) {
    std::ostringstream command;
    command << "./build/nmc " << domain::to_string(request.type) << " ";

    switch (request.type) {
        case domain::JobType::Train:
            command << "--env point_mass ";
            command << "--seed " << request.seed << ' ';
            command << "--run-id " << request.run_id << ' ';
            if (request.quick) {
                command << "--quick ";
            }
            break;
        case domain::JobType::Eval:
            command << "--env point_mass ";
            command << "--seed " << request.seed << ' ';
            command << "--checkpoint artifacts/latest/checkpoint.pt ";
            command << "--run-id " << request.run_id << ' ';
            break;
        case domain::JobType::Benchmark:
            command << "--seed " << request.seed << ' ';
            command << "--name " << request.run_id << ' ';
            if (request.quick) {
                command << "--quick ";
            } else {
                command << "--full ";
            }
            break;
    }

    return command.str();
}

std::string default_run_id(const domain::JobType type) {
    return std::string(domain::to_string(type)) + '_' + common::make_id("jobrun");
}

}  // namespace

JobService::JobService(std::filesystem::path repository_root, const bool executor_enabled)
    : repository_root_(std::move(repository_root)), executor_enabled_(executor_enabled) {}

JobService::~JobService() {
    workers_.clear();
}

domain::JobRecord JobService::submit(const JobLaunchRequest& request) {
    domain::JobRecord record;
    record.job_id = common::make_id("job");
    record.job_type = request.type;
    record.status = domain::JobStatus::Queued;
    record.run_id = request.run_id.empty() ? default_run_id(request.type) : request.run_id;
    record.created_at = common::now_utc_iso8601();
    record.updated_at = record.created_at;

    {
        std::scoped_lock lock(mutex_);
        jobs_.insert_or_assign(record.job_id, record);
    }

    JobLaunchRequest launch_request = request;
    launch_request.run_id = record.run_id;

    workers_.emplace_back([this, job_id = record.job_id, launch_request](std::stop_token) mutable {
        run_job(job_id, std::move(launch_request));
    });

    return record;
}

std::optional<domain::JobRecord> JobService::get(const std::string& job_id) const {
    std::scoped_lock lock(mutex_);
    const auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return std::nullopt;
    }
    return it->second;
}

void JobService::run_job(std::string job_id, JobLaunchRequest request) {
    {
        std::scoped_lock lock(mutex_);
        auto it = jobs_.find(job_id);
        if (it == jobs_.end()) {
            return;
        }
        it->second.status = domain::JobStatus::Running;
        it->second.updated_at = common::now_utc_iso8601();
        it->second.details_json = "{\"executor_enabled\":" + std::string(executor_enabled_ ? "true" : "false") + "}";
    }

    if (!executor_enabled_) {
        std::scoped_lock lock(mutex_);
        auto it = jobs_.find(job_id);
        if (it == jobs_.end()) {
            return;
        }
        it->second.status = domain::JobStatus::Completed;
        it->second.updated_at = common::now_utc_iso8601();
        it->second.details_json =
            "{\"mode\":\"dry_run\",\"message\":\"set ORBITAL_JOB_EXECUTOR=1 to execute nmc jobs from backend\"}";
        return;
    }

    const std::string command = command_for(request);
    const std::string wrapped_command =
        "bash -lc \"cd '" + repository_root_.string() + "' && " + command + "\"";
    common::log(common::LogLevel::Info, "executing job command: " + command, request.run_id);

    const int exit_code = std::system(wrapped_command.c_str());

    std::scoped_lock lock(mutex_);
    auto it = jobs_.find(job_id);
    if (it == jobs_.end()) {
        return;
    }

    it->second.updated_at = common::now_utc_iso8601();
    if (exit_code == 0) {
        it->second.status = domain::JobStatus::Completed;
        it->second.details_json = "{\"exit_code\":0,\"command\":\"" + command + "\"}";
    } else {
        it->second.status = domain::JobStatus::Failed;
        it->second.details_json =
            "{\"exit_code\":" + std::to_string(exit_code) + ",\"command\":\"" + command + "\"}";
    }
}

}  // namespace orbital::backend::application
