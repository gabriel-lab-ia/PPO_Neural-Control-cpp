#include "infrastructure/artifacts/artifact_layout.h"

#include <fstream>
#include <stdexcept>

namespace nmc::infrastructure::artifacts {
namespace {

bool is_same_path(const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
    std::error_code ec_lhs;
    std::error_code ec_rhs;
    const auto lhs_abs = std::filesystem::absolute(lhs, ec_lhs);
    const auto rhs_abs = std::filesystem::absolute(rhs, ec_rhs);
    if (ec_lhs || ec_rhs) {
        return false;
    }
    return lhs_abs.lexically_normal() == rhs_abs.lexically_normal();
}

}  // namespace

ArtifactLayout make_layout(const std::filesystem::path& root, const std::string& run_id) {
    ArtifactLayout layout;
    layout.root = root;
    layout.runs_dir = root / "runs";
    layout.checkpoints_dir = root / "checkpoints";
    layout.reports_dir = root / "reports";
    layout.benchmarks_dir = root / "benchmarks";
    layout.latest_dir = root / "latest";

    layout.run_dir = layout.runs_dir / run_id;
    layout.run_manifest_json = layout.run_dir / "manifest.json";
    layout.train_metrics_csv = layout.run_dir / "training_metrics.csv";
    layout.train_summary_json = layout.run_dir / "training_summary.json";
    layout.eval_summary_json = layout.run_dir / "evaluation_summary.json";
    layout.live_rollout_csv = layout.run_dir / "live_rollout.csv";

    layout.run_checkpoint_dir = layout.run_dir / "checkpoints";
    layout.run_checkpoint_model = layout.run_checkpoint_dir / "policy_last.pt";
    layout.run_checkpoint_meta = layout.run_checkpoint_dir / "policy_last.meta";

    layout.global_checkpoint_model = layout.checkpoints_dir / (run_id + "_policy_last.pt");
    layout.global_checkpoint_meta = layout.checkpoints_dir / (run_id + "_policy_last.meta");

    std::filesystem::create_directories(layout.runs_dir);
    std::filesystem::create_directories(layout.checkpoints_dir);
    std::filesystem::create_directories(layout.reports_dir);
    std::filesystem::create_directories(layout.benchmarks_dir);
    std::filesystem::create_directories(layout.latest_dir);
    std::filesystem::create_directories(layout.run_dir);
    std::filesystem::create_directories(layout.run_checkpoint_dir);

    return layout;
}

void write_text_file(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::out | std::ios::trunc);
    if (!stream.is_open()) {
        throw std::runtime_error("unable to open file: " + path.string());
    }
    stream << content;
}

bool is_readable_file(const std::filesystem::path& path) {
    std::ifstream stream(path);
    return stream.good();
}

void refresh_latest_snapshot(
    const ArtifactLayout& layout,
    const std::vector<std::filesystem::path>& files_to_copy,
    const std::filesystem::path& checkpoint_model,
    const std::filesystem::path& checkpoint_meta
) {
    std::filesystem::create_directories(layout.latest_dir);

    for (const auto& file : files_to_copy) {
        if (std::filesystem::exists(file)) {
            const auto destination = layout.latest_dir / file.filename();
            if (!is_same_path(file, destination)) {
                std::filesystem::copy_file(file, destination, std::filesystem::copy_options::overwrite_existing);
            }
        }
    }

    if (std::filesystem::exists(checkpoint_model)) {
        const auto destination = layout.latest_dir / "checkpoint.pt";
        if (!is_same_path(checkpoint_model, destination)) {
            std::filesystem::copy_file(
                checkpoint_model,
                destination,
                std::filesystem::copy_options::overwrite_existing
            );
        }
    }

    if (std::filesystem::exists(checkpoint_meta)) {
        const auto destination = layout.latest_dir / "checkpoint.meta";
        if (!is_same_path(checkpoint_meta, destination)) {
            std::filesystem::copy_file(
                checkpoint_meta,
                destination,
                std::filesystem::copy_options::overwrite_existing
            );
        }
    }
}

}  // namespace nmc::infrastructure::artifacts
