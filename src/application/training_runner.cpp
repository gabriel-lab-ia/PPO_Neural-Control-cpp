#include "application/training_runner.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "common/json_utils.h"
#include "common/time_utils.h"
#include "common/determinism.h"
#include "domain/config/config_validation.h"
#include "domain/env/environment_factory.h"
#include "domain/ppo/trainer.h"
#include "infrastructure/artifacts/artifact_layout.h"
#include "infrastructure/artifacts/checkpoint_manager.h"
#include "infrastructure/artifacts/run_manifest.h"
#include "infrastructure/persistence/sqlite_experiment_store.h"
#include "infrastructure/reporting/live_rollout_writer.h"
#include "infrastructure/reporting/metrics_csv_writer.h"

namespace nmc::application {
namespace {

std::string training_summary_json(
    const std::string& run_id,
    const std::string& environment,
    const domain::ppo::TrainingMetrics& final,
    const int64_t completed_episodes,
    const std::string& checkpoint_path
) {
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"run_id\": \"" << common::json_escape(run_id) << "\",\n";
    stream << "  \"environment\": \"" << common::json_escape(environment) << "\",\n";
    stream << "  \"completed_episodes\": " << completed_episodes << ",\n";
    stream << "  \"final_metrics\": {\n";
    stream << "    \"update\": " << final.update << ",\n";
    stream << "    \"env_steps\": " << final.env_steps << ",\n";
    stream << "    \"policy_loss\": " << final.policy_loss << ",\n";
    stream << "    \"value_loss\": " << final.value_loss << ",\n";
    stream << "    \"entropy\": " << final.entropy << ",\n";
    stream << "    \"approx_kl\": " << final.approx_kl << ",\n";
    stream << "    \"clip_fraction\": " << final.clip_fraction << ",\n";
    stream << "    \"avg_episode_return\": " << final.avg_episode_return << ",\n";
    stream << "    \"avg_episode_length\": " << final.avg_episode_length << ",\n";
    stream << "    \"success_rate\": " << final.success_rate << ",\n";
    stream << "    \"samples_per_second\": " << final.samples_per_second << ",\n";
    stream << "    \"inference_latency_ms\": " << final.inference_latency_ms << "\n";
    stream << "  },\n";
    stream << "  \"checkpoint_path\": \"" << common::json_escape(checkpoint_path) << "\"\n";
    stream << "}\n";
    return stream.str();
}

}  // namespace

TrainingRunner::TrainingRunner(std::filesystem::path artifact_root)
    : artifact_root_(std::move(artifact_root)) {}

TrainingRunOutput TrainingRunner::run(const domain::config::TrainConfig& input_config) {
    auto config = input_config;
    if (config.run_id.empty()) {
        config.run_id = common::make_run_id("train");
    }
    domain::config::validate_train_config_or_throw(config);

    const auto started_at = common::now_utc_iso8601();
    const auto layout = infrastructure::artifacts::make_layout(artifact_root_, config.run_id);

    infrastructure::persistence::SQLiteExperimentStore db(artifact_root_ / "experiments.sqlite");
    db.initialize();

    db.insert_run_start(
        {
            config.run_id,
            "train",
            config.environment,
            config.trainer.seed,
            started_at,
            "running",
            layout.run_dir.string(),
            domain::config::to_json(config)
        }
    );

    std::string summary_json;
    try {
        domain::env::EnvironmentSpec env_spec;
        env_spec.kind = domain::env::parse_environment_kind_or_throw(config.environment);
        env_spec.seed = static_cast<uint64_t>(config.trainer.seed);
        env_spec.mujoco_model_path = config.mujoco_model_path;
        env_spec.point_mass_reward = config.point_mass_reward;

        common::configure_determinism(static_cast<uint64_t>(config.trainer.seed), config.trainer.torch_num_threads);
        auto env_pack = domain::env::make_environment_pack(env_spec, config.trainer.num_envs);
        domain::ppo::PPOTrainer trainer(config.trainer, std::move(env_pack));

        if (config.resume_checkpoint.has_value()) {
            infrastructure::artifacts::load_policy_checkpoint(*config.resume_checkpoint, trainer.model());
            db.insert_event(
                {
                    config.run_id,
                    "info",
                    "resume_checkpoint",
                    "training resumed from checkpoint",
                    "{\"checkpoint\":\"" + common::json_escape(config.resume_checkpoint->string()) + "\"}",
                    common::now_utc_iso8601()
                }
            );
        }

        infrastructure::reporting::MetricsCsvWriter metrics_writer(layout.train_metrics_csv);
        const auto metrics = trainer.train();
        for (const auto& metric : metrics) {
            metrics_writer.append(metric);
        }

        const auto completed_episodes = trainer.consume_completed_episodes();
        const auto telemetry_timestamp = common::now_utc_iso8601();
        for (const auto& episode : completed_episodes) {
            db.insert_episode(
                {
                    config.run_id,
                    "train",
                    episode.episode_index,
                    episode.env_steps,
                    episode.episode_return,
                    episode.episode_length,
                    episode.success,
                    telemetry_timestamp
                }
            );
        }

        const auto live_steps = trainer.run_live_episode(
            config.live_rollout_steps,
            config.deterministic_live_rollout,
            nullptr
        );
        infrastructure::reporting::write_live_rollout_csv(layout.live_rollout_csv, live_steps);

        const auto final_metrics = metrics.empty() ? domain::ppo::TrainingMetrics{} : metrics.back();
        const auto finished_at = common::now_utc_iso8601();

        infrastructure::artifacts::CheckpointMetadata checkpoint_meta;
        checkpoint_meta.run_id = config.run_id;
        checkpoint_meta.environment = config.environment;
        checkpoint_meta.observation_dim = trainer.observation_dim();
        checkpoint_meta.action_dim = trainer.action_dim();
        checkpoint_meta.hidden_dim = config.trainer.hidden_dim;
        checkpoint_meta.seed = config.trainer.seed;
        checkpoint_meta.created_at = finished_at;

        infrastructure::artifacts::save_policy_checkpoint(
            layout.run_checkpoint_model,
            layout.run_checkpoint_meta,
            trainer.model(),
            checkpoint_meta
        );

        std::filesystem::copy_file(
            layout.run_checkpoint_model,
            layout.global_checkpoint_model,
            std::filesystem::copy_options::overwrite_existing
        );
        std::filesystem::copy_file(
            layout.run_checkpoint_meta,
            layout.global_checkpoint_meta,
            std::filesystem::copy_options::overwrite_existing
        );

        summary_json = training_summary_json(
            config.run_id,
            config.environment,
            final_metrics,
            static_cast<int64_t>(completed_episodes.size()),
            layout.run_checkpoint_model.string()
        );
        infrastructure::artifacts::write_text_file(layout.train_summary_json, summary_json);
        infrastructure::artifacts::write_text_file(
            layout.reports_dir / (config.run_id + "_training_summary.json"),
            summary_json
        );

        const auto manifest = infrastructure::artifacts::render_run_manifest_json(
            {
                config.run_id,
                "train",
                config.environment,
                started_at,
                finished_at,
                "completed",
                domain::config::to_json(config),
                {
                    {"run_dir", layout.run_dir},
                    {"training_metrics_csv", layout.train_metrics_csv},
                    {"training_summary_json", layout.train_summary_json},
                    {"live_rollout_csv", layout.live_rollout_csv},
                    {"checkpoint_model", layout.run_checkpoint_model},
                    {"checkpoint_meta", layout.run_checkpoint_meta}
                },
                layout.run_checkpoint_model.string(),
                std::nullopt
            }
        );
        infrastructure::artifacts::write_text_file(layout.run_manifest_json, manifest);

        infrastructure::artifacts::refresh_latest_snapshot(
            layout,
            {
                layout.run_manifest_json,
                layout.train_metrics_csv,
                layout.train_summary_json,
                layout.live_rollout_csv
            },
            layout.run_checkpoint_model,
            layout.run_checkpoint_meta
        );

        db.finalize_run(config.run_id, "completed", finished_at, summary_json);

        std::cout << "Run completed: " << config.run_id << '\n';
        std::cout << "Training summary: " << layout.train_summary_json << '\n';
        std::cout << "Checkpoint: " << layout.run_checkpoint_model << '\n';

        return {
            config.run_id,
            layout.run_dir,
            layout.run_manifest_json,
            layout.run_checkpoint_model,
            layout.run_checkpoint_meta,
            layout.train_summary_json,
            final_metrics,
            static_cast<int64_t>(completed_episodes.size())
        };
    } catch (const std::exception& error) {
        const auto failed_at = common::now_utc_iso8601();
        const std::string error_summary = "{\"status\":\"failed\",\"run_id\":\"" +
            common::json_escape(config.run_id) + "\",\"error\":\"" + common::json_escape(error.what()) + "\"}";

        try {
            const auto failed_manifest = infrastructure::artifacts::render_run_manifest_json(
                {
                    config.run_id,
                    "train",
                    config.environment,
                    started_at,
                    failed_at,
                    "failed",
                    domain::config::to_json(config),
                    {
                        {"run_dir", layout.run_dir},
                        {"training_metrics_csv", layout.train_metrics_csv},
                        {"training_summary_json", layout.train_summary_json},
                        {"live_rollout_csv", layout.live_rollout_csv},
                        {"checkpoint_model", layout.run_checkpoint_model},
                        {"checkpoint_meta", layout.run_checkpoint_meta}
                    },
                    layout.run_checkpoint_model.string(),
                    error.what()
                }
            );
            infrastructure::artifacts::write_text_file(layout.run_manifest_json, failed_manifest);
            db.insert_event(
                {
                    config.run_id,
                    "error",
                    "run_failed",
                    error.what(),
                    "{}",
                    failed_at
                }
            );
            db.finalize_run(config.run_id, "failed", failed_at, error_summary);
        } catch (...) {
        }
        throw;
    } catch (...) {
        const auto failed_at = common::now_utc_iso8601();
        const std::string error_summary = "{\"status\":\"failed\",\"run_id\":\"" + common::json_escape(config.run_id) +
            "\",\"error\":\"unknown\"}";
        try {
            const auto failed_manifest = infrastructure::artifacts::render_run_manifest_json(
                {
                    config.run_id,
                    "train",
                    config.environment,
                    started_at,
                    failed_at,
                    "failed",
                    domain::config::to_json(config),
                    {
                        {"run_dir", layout.run_dir},
                        {"training_metrics_csv", layout.train_metrics_csv},
                        {"training_summary_json", layout.train_summary_json},
                        {"live_rollout_csv", layout.live_rollout_csv},
                        {"checkpoint_model", layout.run_checkpoint_model},
                        {"checkpoint_meta", layout.run_checkpoint_meta}
                    },
                    layout.run_checkpoint_model.string(),
                    "unknown"
                }
            );
            infrastructure::artifacts::write_text_file(layout.run_manifest_json, failed_manifest);
            db.finalize_run(config.run_id, "failed", failed_at, error_summary);
        } catch (...) {
        }
        throw;
    }
}

}  // namespace nmc::application
