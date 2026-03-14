#include "app/training_app.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "env/environment_registry.h"
#include "train/ppo_trainer.h"
#include "utils/csv_logger.h"
#include "utils/live_rollout_logger.h"
#include "utils/neural_3d_export.h"

namespace nmc {
namespace {

std::string get_env_or_default(const char* key, const std::string& fallback) {
    if (const char* value = std::getenv(key)) {
        return value;
    }
    return fallback;
}

EnvironmentSelection load_environment_selection() {
    EnvironmentSelection selection;
    selection.kind = get_env_or_default("NMC_ENV", "point_mass");

    if (selection.kind == "mujoco_cartpole") {
        const auto model_override = get_env_or_default("NMC_MUJOCO_XML", "");
        if (!model_override.empty()) {
            selection.mujoco_model_path = model_override;
        } else {
            selection.mujoco_model_path = std::filesystem::path("assets/mujoco/cartpole.xml");
        }
    }

    return selection;
}

bool run_live_after_training() {
    const auto value = get_env_or_default("NMC_LIVE_POLICY", "0");
    return value == "1" || value == "true" || value == "TRUE" || value == "on";
}

int64_t live_step_limit() {
    const auto raw = get_env_or_default("NMC_LIVE_STEPS", "240");
    try {
        return std::max<int64_t>(1, std::stoll(raw));
    } catch (const std::exception&) {
        return 240;
    }
}

void write_benchmark_summary(
    const std::filesystem::path& path,
    const std::string& environment_name,
    const TrainerConfig& config,
    const TrainingMetrics& metrics
) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::out | std::ios::trunc);
    if (!stream.is_open()) {
        throw std::runtime_error("unable to open benchmark summary file: " + path.string());
    }

    stream << std::fixed << std::setprecision(6);
    stream << "{\n";
    stream << "  \"environment\": \"" << environment_name << "\",\n";
    stream << "  \"hidden_dim\": " << config.hidden_dim << ",\n";
    stream << "  \"ppo_epochs\": " << config.ppo_epochs << ",\n";
    stream << "  \"minibatch_size\": " << config.minibatch_size << ",\n";
    stream << "  \"policy_loss\": " << metrics.policy_loss << ",\n";
    stream << "  \"value_loss\": " << metrics.value_loss << ",\n";
    stream << "  \"entropy\": " << metrics.entropy << ",\n";
    stream << "  \"action_std\": " << metrics.action_std << ",\n";
    stream << "  \"explained_variance\": " << metrics.explained_variance << ",\n";
    stream << "  \"avg_episode_return\": " << metrics.avg_episode_return << ",\n";
    stream << "  \"success_rate\": " << metrics.success_rate << ",\n";
    stream << "  \"update_time_ms\": " << metrics.update_time_ms << ",\n";
    stream << "  \"samples_per_second\": " << metrics.samples_per_second << ",\n";
    stream << "  \"inference_latency_ms\": " << metrics.inference_latency_ms << ",\n";
    stream << "  \"parameter_count_k\": " << metrics.parameter_count_k << "\n";
    stream << "}\n";
}

}  // namespace

int run_training_app() {
    const auto artifact_dir = std::filesystem::path("artifacts");
    const auto metrics_path = artifact_dir / "learning_curve.csv";
    const auto benchmark_path = artifact_dir / "benchmark_summary.json";

    const TrainerConfig config{};
    const auto environment_selection = load_environment_selection();
    auto environment_pack = make_environment_pack(environment_selection, config.num_envs);
    const auto environment_name = environment_pack.display_name;

    PPOTrainer trainer(config, artifact_dir, std::move(environment_pack));
    CsvLogger logger(metrics_path);

    std::cout << "=============================================================\n";
    std::cout << "  NeuroMotor PPO Foundation\n";
    std::cout << "  C++20 + LibTorch | continuous control baseline for MuJoCo\n";
    std::cout << "=============================================================\n";
    std::cout << "Environment: " << environment_name << '\n';
    std::cout << "Goal: replace the old synthetic AGI prototype with a clean PPO core.\n";
    if (mujoco_support_enabled()) {
        std::cout << "MuJoCo support: enabled at compile time.\n\n";
    } else {
        std::cout << "MuJoCo support: not compiled in yet. Set up the library and rebuild with NMC_ENABLE_MUJOCO=ON.\n\n";
    }

    auto metrics = trainer.train();

    std::cout << "Training progress\n";
    std::cout << "-----------------\n";
    for (const auto& metric : metrics) {
        logger.log(metric);
        std::cout
            << "update " << std::setw(2) << metric.update
            << " | steps=" << std::setw(6) << metric.env_steps
            << " | policy=" << std::setw(8) << std::fixed << std::setprecision(4) << metric.policy_loss
            << " | value=" << std::setw(8) << metric.value_loss
            << " | entropy=" << std::setw(7) << metric.entropy
            << " | std=" << std::setw(7) << metric.action_std
            << " | reward=" << std::setw(8) << metric.avg_episode_return
            << " | fps=" << std::setw(8) << metric.samples_per_second
            << " | latency_ms=" << std::setw(7) << metric.inference_latency_ms
            << '\n';
    }

    if (!metrics.empty()) {
        const auto& final = metrics.back();
        write_benchmark_summary(benchmark_path, environment_name, config, final);

        std::cout << "\nSummary\n";
        std::cout << "-------\n";
        std::cout << "Final avg episode return : " << final.avg_episode_return << '\n';
        std::cout << "Final success rate       : " << final.success_rate << '\n';
        std::cout << "Final value loss         : " << final.value_loss << '\n';
        std::cout << "Final policy entropy     : " << final.entropy << '\n';
        std::cout << "Final action std         : " << final.action_std << '\n';
        std::cout << "Explained variance       : " << final.explained_variance << '\n';
        std::cout << "Inference latency (ms)   : " << final.inference_latency_ms << '\n';
        std::cout << "Samples per second       : " << final.samples_per_second << '\n';
        std::cout << "Parameter count (K)      : " << final.parameter_count_k << '\n';
        std::cout << "Metrics exported to      : " << metrics_path << '\n';
        std::cout << "Benchmark summary        : " << benchmark_path << '\n';
    }

    {
        const auto live_path = artifact_dir / "live_rollout.csv";
        const auto neural_html_path = artifact_dir / "neural_network_3d.html";
        const auto neural_json_path = artifact_dir / "neural_network_3d.json";
        std::ostringstream silent_stream;
        std::ostream& live_stream = run_live_after_training() ? std::cout : silent_stream;
        auto live_steps = trainer.run_live_episode(live_step_limit(), live_stream);
        write_live_rollout_csv(live_path, live_steps);
        write_neural_3d_visualization(
            neural_html_path,
            neural_json_path,
            environment_name,
            trainer.agent(),
            live_steps
        );
        std::cout << "Live rollout exported to : " << live_path << '\n';
        std::cout << "3D neural viewer saved to: " << neural_html_path << '\n';
    }

    return 0;
}

}  // namespace nmc
