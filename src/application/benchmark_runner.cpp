#include "application/benchmark_runner.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "application/evaluation_runner.h"
#include "application/training_runner.h"
#include "common/json_utils.h"
#include "common/time_utils.h"
#include "infrastructure/artifacts/artifact_layout.h"
#include "infrastructure/persistence/sqlite_experiment_store.h"

namespace nmc::application {
namespace {

std::string benchmark_json(
    const std::string& benchmark_id,
    const domain::config::BenchmarkConfig& config,
    const TrainingRunOutput& train,
    const EvaluationRunOutput& eval,
    const bool artifacts_valid,
    const std::vector<std::filesystem::path>& validated_artifacts
) {
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"benchmark_id\": \"" << common::json_escape(benchmark_id) << "\",\n";
    stream << "  \"benchmark_name\": \"" << common::json_escape(config.benchmark_name) << "\",\n";
    stream << "  \"quick\": " << (config.quick ? "true" : "false") << ",\n";
    stream << "  \"seed\": " << config.seed << ",\n";
    stream << "  \"artifacts_valid\": " << (artifacts_valid ? "true" : "false") << ",\n";
    stream << "  \"train\": {\n";
    stream << "    \"run_id\": \"" << common::json_escape(train.run_id) << "\",\n";
    stream << "    \"checkpoint\": \"" << common::json_escape(train.checkpoint_path.string()) << "\",\n";
    stream << "    \"final_avg_episode_return\": " << train.final_metrics.avg_episode_return << ",\n";
    stream << "    \"final_success_rate\": " << train.final_metrics.success_rate << "\n";
    stream << "  },\n";
    stream << "  \"eval\": {\n";
    stream << "    \"run_id\": \"" << common::json_escape(eval.run_id) << "\",\n";
    stream << "    \"avg_episode_return\": " << eval.avg_episode_return << ",\n";
    stream << "    \"avg_episode_length\": " << eval.avg_episode_length << ",\n";
    stream << "    \"success_rate\": " << eval.success_rate << "\n";
    stream << "  },\n";
    stream << "  \"validated_artifacts\": [\n";
    for (std::size_t index = 0; index < validated_artifacts.size(); ++index) {
        stream << "    \"" << common::json_escape(validated_artifacts[index].string()) << "\"";
        if (index + 1 < validated_artifacts.size()) {
            stream << ',';
        }
        stream << "\n";
    }
    stream << "  ]\n";
    stream << "}\n";
    return stream.str();
}

std::string benchmark_csv(
    const std::string& benchmark_id,
    const TrainingRunOutput& train,
    const EvaluationRunOutput& eval,
    const bool artifacts_valid
) {
    std::ostringstream stream;
    stream << "benchmark_id,train_run_id,eval_run_id,train_avg_return,train_success_rate,eval_avg_return,eval_success_rate,artifacts_valid\n";
    stream << benchmark_id << ','
           << train.run_id << ','
           << eval.run_id << ','
           << train.final_metrics.avg_episode_return << ','
           << train.final_metrics.success_rate << ','
           << eval.avg_episode_return << ','
           << eval.success_rate << ','
           << (artifacts_valid ? 1 : 0) << '\n';
    return stream.str();
}

}  // namespace

BenchmarkRunner::BenchmarkRunner(std::filesystem::path artifact_root)
    : artifact_root_(std::move(artifact_root)) {}

BenchmarkRunOutput BenchmarkRunner::run(const domain::config::BenchmarkConfig& config) {
    const auto benchmark_id = common::make_run_id(config.benchmark_name);

    domain::config::TrainConfig train_config;
    train_config.run_id = common::make_run_id("bench_train");
    train_config.environment = "point_mass";
    train_config.trainer.seed = config.seed;
    train_config.trainer.num_envs = config.quick ? 4 : 8;
    train_config.trainer.total_updates = config.quick ? 3 : 6;
    train_config.trainer.hidden_dim = 64;
    train_config.trainer.benchmark_iterations = config.quick ? 64 : 128;
    train_config.trainer.checkpoint_interval_updates = 1;
    train_config.trainer.ppo.rollout_steps = config.quick ? 32 : 64;
    train_config.trainer.ppo.ppo_epochs = 2;
    train_config.trainer.ppo.minibatch_size = 64;
    train_config.live_rollout_steps = config.quick ? 48 : 96;

    TrainingRunner training_runner(artifact_root_);
    const auto train_run = training_runner.run(train_config);

    domain::config::EvalConfig eval_config;
    eval_config.run_id = common::make_run_id("bench_eval");
    eval_config.environment = "point_mass";
    eval_config.checkpoint_path = train_run.checkpoint_path;
    eval_config.episodes = config.quick ? 4 : 8;
    eval_config.max_steps = 120;
    eval_config.seed = config.seed;
    eval_config.deterministic_policy = true;
    eval_config.inference_backend = "libtorch";

    EvaluationRunner evaluation_runner(artifact_root_);
    const auto eval_run = evaluation_runner.run(eval_config);

    const std::vector<std::filesystem::path> required_artifacts = {
        train_run.manifest_path,
        train_run.training_summary_path,
        train_run.checkpoint_path,
        train_run.checkpoint_meta_path,
        eval_run.manifest_path,
        eval_run.evaluation_summary_path
    };

    bool artifacts_valid = true;
    for (const auto& artifact : required_artifacts) {
        if (!infrastructure::artifacts::is_readable_file(artifact)) {
            artifacts_valid = false;
            break;
        }
    }

    if (!artifacts_valid) {
        throw std::runtime_error("benchmark validation failed: required artifacts are missing or unreadable");
    }

    const auto benchmark_json_path = artifact_root_ / "benchmarks" / (benchmark_id + ".json");
    const auto benchmark_csv_path = artifact_root_ / "benchmarks" / (benchmark_id + ".csv");

    const auto json_content = benchmark_json(
        benchmark_id,
        config,
        train_run,
        eval_run,
        artifacts_valid,
        required_artifacts
    );
    const auto csv_content = benchmark_csv(benchmark_id, train_run, eval_run, artifacts_valid);

    infrastructure::artifacts::write_text_file(benchmark_json_path, json_content);
    infrastructure::artifacts::write_text_file(benchmark_csv_path, csv_content);
    infrastructure::artifacts::write_text_file(artifact_root_ / "benchmarks" / "latest.json", json_content);
    infrastructure::artifacts::write_text_file(artifact_root_ / "benchmarks" / "latest.csv", csv_content);

    infrastructure::persistence::SQLiteExperimentStore db(artifact_root_ / "experiments.sqlite");
    db.initialize();
    db.insert_benchmark(
        {
            config.benchmark_name,
            train_run.run_id,
            json_content,
            common::now_utc_iso8601()
        }
    );

    std::cout << "Benchmark completed: " << benchmark_id << '\n';
    std::cout << "Benchmark summary: " << benchmark_json_path << '\n';

    return {
        benchmark_id,
        train_run.run_id,
        eval_run.run_id,
        benchmark_json_path,
        benchmark_csv_path,
        artifacts_valid
    };
}

}  // namespace nmc::application
