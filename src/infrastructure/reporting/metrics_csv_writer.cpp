#include "infrastructure/reporting/metrics_csv_writer.h"

#include <iomanip>
#include <stdexcept>

namespace nmc::infrastructure::reporting {

MetricsCsvWriter::MetricsCsvWriter(const std::filesystem::path& path) {
    std::filesystem::create_directories(path.parent_path());
    stream_.open(path, std::ios::out | std::ios::trunc);
    if (!stream_.is_open()) {
        throw std::runtime_error("unable to open metrics file: " + path.string());
    }

    stream_ << "update,env_steps,policy_loss,value_loss,entropy,approx_kl,clip_fraction," \
              "avg_step_reward,avg_episode_return,avg_episode_length,success_rate,action_std," \
              "explained_variance,update_time_ms,samples_per_second,inference_latency_ms,parameter_count_k,completed_episodes\n";
    stream_ << std::fixed << std::setprecision(6);
}

MetricsCsvWriter::~MetricsCsvWriter() {
    if (stream_.is_open()) {
        stream_.flush();
    }
}

void MetricsCsvWriter::append(const domain::ppo::TrainingMetrics& metrics) {
    stream_
        << metrics.update << ','
        << metrics.env_steps << ','
        << metrics.policy_loss << ','
        << metrics.value_loss << ','
        << metrics.entropy << ','
        << metrics.approx_kl << ','
        << metrics.clip_fraction << ','
        << metrics.avg_step_reward << ','
        << metrics.avg_episode_return << ','
        << metrics.avg_episode_length << ','
        << metrics.success_rate << ','
        << metrics.action_std << ','
        << metrics.explained_variance << ','
        << metrics.update_time_ms << ','
        << metrics.samples_per_second << ','
        << metrics.inference_latency_ms << ','
        << metrics.parameter_count_k << ','
        << metrics.completed_episodes << '\n';
    stream_.flush();
}

}  // namespace nmc::infrastructure::reporting
