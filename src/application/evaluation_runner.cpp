#include "application/evaluation_runner.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "common/json_utils.h"
#include "common/time_utils.h"
#include "common/determinism.h"
#include "domain/config/config_validation.h"
#include "domain/env/environment_factory.h"
#include "domain/inference/inference_backend_factory.h"
#include "infrastructure/artifacts/artifact_layout.h"
#include "infrastructure/artifacts/checkpoint_manager.h"
#include "infrastructure/artifacts/run_manifest.h"
#include "infrastructure/persistence/sqlite_experiment_store.h"

namespace nmc::application {
namespace {

struct EpisodeEvaluation {
    int64_t episode_index = 0;
    int64_t env_steps = 0;
    float episode_return = 0.0f;
    int64_t episode_length = 0;
    float success = 0.0f;
};

float mean_float(const std::vector<float>& values) {
    if (values.empty()) {
        return 0.0f;
    }
    const float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / static_cast<float>(values.size());
}

float mean_int(const std::vector<int64_t>& values) {
    if (values.empty()) {
        return 0.0f;
    }
    const auto sum = std::accumulate(values.begin(), values.end(), int64_t{0});
    return static_cast<float>(sum) / static_cast<float>(values.size());
}

float percentile_approx(std::vector<float> values, const float percentile) {
    if (values.empty()) {
        return 0.0f;
    }

    const float clamped = std::clamp(percentile, 0.0f, 1.0f);
    const auto index = static_cast<std::size_t>(
        std::ceil(clamped * static_cast<float>(values.size() - 1))
    );
    std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(index), values.end());
    return values[index];
}

std::filesystem::path resolve_checkpoint_path(
    const domain::config::EvalConfig& config,
    const std::filesystem::path& artifact_root
) {
    if (!config.checkpoint_path.empty()) {
        return config.checkpoint_path;
    }
    return artifact_root / "latest" / "checkpoint.pt";
}

std::filesystem::path resolve_checkpoint_metadata_path(const std::filesystem::path& checkpoint_path) {
    const auto explicit_meta = checkpoint_path.parent_path() / "checkpoint.meta";
    if (std::filesystem::exists(explicit_meta)) {
        return explicit_meta;
    }

    const auto sibling_meta = checkpoint_path.parent_path() /
        (checkpoint_path.stem().string() + ".meta");
    if (std::filesystem::exists(sibling_meta)) {
        return sibling_meta;
    }

    const auto fallback = checkpoint_path.string() + ".meta";
    return fallback;
}

std::string evaluation_summary_json(
    const std::string& run_id,
    const std::string& environment,
    const std::string& backend,
    const std::filesystem::path& checkpoint_path,
    const domain::inference::InferenceBackendCapabilities& capabilities,
    const float avg_episode_return,
    const float avg_episode_length,
    const float success_rate,
    const float avg_inference_latency_ms,
    const float p95_inference_latency_ms,
    const int64_t episodes
) {
    std::ostringstream stream;
    stream << "{\n";
    stream << "  \"run_id\": \"" << common::json_escape(run_id) << "\",\n";
    stream << "  \"environment\": \"" << common::json_escape(environment) << "\",\n";
    stream << "  \"backend\": \"" << common::json_escape(backend) << "\",\n";
    stream << "  \"backend_capabilities\": {\n";
    stream << "    \"supports_dynamic_shapes\": " << (capabilities.supports_dynamic_shapes ? "true" : "false") << ",\n";
    stream << "    \"supports_fp16\": " << (capabilities.supports_fp16 ? "true" : "false") << ",\n";
    stream << "    \"supports_int8\": " << (capabilities.supports_int8 ? "true" : "false") << ",\n";
    stream << "    \"uses_cuda\": " << (capabilities.uses_cuda ? "true" : "false") << ",\n";
    stream << "    \"is_emulated\": " << (capabilities.is_emulated ? "true" : "false") << ",\n";
    stream << "    \"runtime\": \"" << common::json_escape(capabilities.runtime) << "\"\n";
    stream << "  },\n";
    stream << "  \"checkpoint_path\": \"" << common::json_escape(checkpoint_path.string()) << "\",\n";
    stream << "  \"episodes\": " << episodes << ",\n";
    stream << "  \"avg_episode_return\": " << avg_episode_return << ",\n";
    stream << "  \"avg_episode_length\": " << avg_episode_length << ",\n";
    stream << "  \"success_rate\": " << success_rate << ",\n";
    stream << "  \"avg_inference_latency_ms\": " << avg_inference_latency_ms << ",\n";
    stream << "  \"p95_inference_latency_ms\": " << p95_inference_latency_ms << "\n";
    stream << "}\n";
    return stream.str();
}

}  // namespace

EvaluationRunner::EvaluationRunner(std::filesystem::path artifact_root)
    : artifact_root_(std::move(artifact_root)) {}

EvaluationRunOutput EvaluationRunner::run(const domain::config::EvalConfig& input_config) {
    auto config = input_config;
    if (config.run_id.empty()) {
        config.run_id = common::make_run_id("eval");
    }
    domain::config::validate_eval_config_or_throw(config);

    const auto started_at = common::now_utc_iso8601();
    const auto layout = infrastructure::artifacts::make_layout(artifact_root_, config.run_id);

    infrastructure::persistence::SQLiteExperimentStore db(artifact_root_ / "experiments.sqlite");
    db.initialize();

    const auto checkpoint_path = resolve_checkpoint_path(config, artifact_root_);
    const auto checkpoint_meta_path = resolve_checkpoint_metadata_path(checkpoint_path);

    if (!std::filesystem::exists(checkpoint_path)) {
        throw std::runtime_error("checkpoint not found: " + checkpoint_path.string());
    }

    const auto checkpoint_meta = infrastructure::artifacts::load_checkpoint_metadata(checkpoint_meta_path);
    if (config.environment.empty()) {
        config.environment = checkpoint_meta.environment;
    }
    domain::config::validate_eval_config_or_throw(config);

    db.insert_run_start(
        {
            config.run_id,
            "eval",
            config.environment,
            config.seed,
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
        env_spec.seed = static_cast<uint64_t>(config.seed);
        env_spec.mujoco_model_path = config.mujoco_model_path;
        env_spec.point_mass_reward = config.point_mass_reward;

        common::configure_determinism(static_cast<uint64_t>(config.seed), 1);
        auto env_pack = domain::env::make_environment_pack(env_spec, 1);
        auto environment = std::move(env_pack.environments.front());

        const int64_t observation_dim = checkpoint_meta.observation_dim > 0 ? checkpoint_meta.observation_dim : env_pack.observation_dim;
        const int64_t action_dim = checkpoint_meta.action_dim > 0 ? checkpoint_meta.action_dim : env_pack.action_dim;
        const int64_t hidden_dim = checkpoint_meta.hidden_dim > 0 ? checkpoint_meta.hidden_dim : 96;

        auto backend = domain::inference::make_inference_backend(
            domain::inference::parse_inference_backend_or_throw(config.inference_backend),
            observation_dim,
            action_dim,
            hidden_dim
        );
        backend->load_checkpoint(checkpoint_path);
        const auto capabilities = backend->capabilities();

        std::vector<EpisodeEvaluation> episodes;
        episodes.reserve(static_cast<std::size_t>(config.episodes));
        std::vector<float> inference_latencies_ms;
        inference_latencies_ms.reserve(static_cast<std::size_t>(config.episodes * config.max_steps));

        int64_t env_steps = 0;
        for (int64_t episode_index = 0; episode_index < config.episodes; ++episode_index) {
            auto observation = environment->reset();
            float episode_return = 0.0f;
            int64_t episode_length = 0;
            float success = 0.0f;

            for (int64_t step = 0; step < config.max_steps; ++step) {
                const auto inference_begin = std::chrono::steady_clock::now();
                const auto inference = backend->infer(observation, config.deterministic_policy);
                const auto inference_end = std::chrono::steady_clock::now();
                inference_latencies_ms.push_back(
                    std::chrono::duration<float, std::milli>(inference_end - inference_begin).count()
                );
                const auto action = inference.action.dim() == 2 ? inference.action[0] : inference.action;
                const auto result = environment->step(action.to(torch::kCPU));

                observation = result.observation;
                episode_return += result.reward;
                ++episode_length;
                ++env_steps;

                if (result.terminated || result.truncated) {
                    success = environment->success_signal(result);
                    break;
                }
            }

            episodes.push_back(
                {
                    episode_index + 1,
                    env_steps,
                    episode_return,
                    episode_length,
                    success
                }
            );
        }

        std::vector<float> returns;
        std::vector<int64_t> lengths;
        std::vector<float> successes;
        returns.reserve(episodes.size());
        lengths.reserve(episodes.size());
        successes.reserve(episodes.size());

        const auto telemetry_timestamp = common::now_utc_iso8601();
        for (const auto& episode : episodes) {
            returns.push_back(episode.episode_return);
            lengths.push_back(episode.episode_length);
            successes.push_back(episode.success);

            db.insert_episode(
                {
                    config.run_id,
                    "eval",
                    episode.episode_index,
                    episode.env_steps,
                    episode.episode_return,
                    episode.episode_length,
                    episode.success,
                    telemetry_timestamp
                }
            );
        }

        const auto avg_return = mean_float(returns);
        const auto avg_length = mean_int(lengths);
        const auto success_rate = mean_float(successes);
        const auto avg_inference_latency_ms = mean_float(inference_latencies_ms);
        const auto p95_inference_latency_ms = percentile_approx(inference_latencies_ms, 0.95f);

        summary_json = evaluation_summary_json(
            config.run_id,
            config.environment,
            backend->backend_name(),
            checkpoint_path,
            capabilities,
            avg_return,
            avg_length,
            success_rate,
            avg_inference_latency_ms,
            p95_inference_latency_ms,
            config.episodes
        );
        infrastructure::artifacts::write_text_file(layout.eval_summary_json, summary_json);
        infrastructure::artifacts::write_text_file(
            layout.reports_dir / (config.run_id + "_evaluation_summary.json"),
            summary_json
        );

        const auto finished_at = common::now_utc_iso8601();
        const auto manifest = infrastructure::artifacts::render_run_manifest_json(
            {
                config.run_id,
                "eval",
                config.environment,
                started_at,
                finished_at,
                "completed",
                domain::config::to_json(config),
                {
                    {"run_dir", layout.run_dir},
                    {"evaluation_summary_json", layout.eval_summary_json}
                },
                checkpoint_path.string(),
                std::nullopt
            }
        );
        infrastructure::artifacts::write_text_file(layout.run_manifest_json, manifest);

        infrastructure::artifacts::refresh_latest_snapshot(
            layout,
            {layout.run_manifest_json, layout.eval_summary_json},
            checkpoint_path,
            checkpoint_meta_path
        );

        db.finalize_run(config.run_id, "completed", finished_at, summary_json);

        std::cout << "Evaluation completed: " << config.run_id << '\n';
        std::cout << "Average return: " << avg_return << '\n';
        std::cout << "Summary: " << layout.eval_summary_json << '\n';

        return {
            config.run_id,
            layout.run_dir,
            layout.run_manifest_json,
            layout.eval_summary_json,
            capabilities.runtime,
            avg_return,
            avg_length,
            success_rate,
            avg_inference_latency_ms,
            p95_inference_latency_ms
        };
    } catch (const std::exception& error) {
        const auto failed_at = common::now_utc_iso8601();
        const std::string error_summary = "{\"status\":\"failed\",\"run_id\":\"" +
            common::json_escape(config.run_id) + "\",\"error\":\"" + common::json_escape(error.what()) + "\"}";
        try {
            const auto failed_manifest = infrastructure::artifacts::render_run_manifest_json(
                {
                    config.run_id,
                    "eval",
                    config.environment,
                    started_at,
                    failed_at,
                    "failed",
                    domain::config::to_json(config),
                    {
                        {"run_dir", layout.run_dir},
                        {"evaluation_summary_json", layout.eval_summary_json}
                    },
                    checkpoint_path.string(),
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
                    "eval",
                    config.environment,
                    started_at,
                    failed_at,
                    "failed",
                    domain::config::to_json(config),
                    {
                        {"run_dir", layout.run_dir},
                        {"evaluation_summary_json", layout.eval_summary_json}
                    },
                    checkpoint_path.string(),
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
