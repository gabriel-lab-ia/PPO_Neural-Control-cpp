#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace nmc::infrastructure::artifacts {

struct ArtifactLayout {
    std::filesystem::path root;
    std::filesystem::path runs_dir;
    std::filesystem::path checkpoints_dir;
    std::filesystem::path reports_dir;
    std::filesystem::path benchmarks_dir;
    std::filesystem::path latest_dir;

    std::filesystem::path run_dir;
    std::filesystem::path run_manifest_json;
    std::filesystem::path train_metrics_csv;
    std::filesystem::path train_summary_json;
    std::filesystem::path eval_summary_json;
    std::filesystem::path live_rollout_csv;

    std::filesystem::path run_checkpoint_dir;
    std::filesystem::path run_checkpoint_model;
    std::filesystem::path run_checkpoint_meta;

    std::filesystem::path global_checkpoint_model;
    std::filesystem::path global_checkpoint_meta;
};

ArtifactLayout make_layout(const std::filesystem::path& root, const std::string& run_id);
void write_text_file(const std::filesystem::path& path, const std::string& content);
bool is_readable_file(const std::filesystem::path& path);
void refresh_latest_snapshot(
    const ArtifactLayout& layout,
    const std::vector<std::filesystem::path>& files_to_copy,
    const std::filesystem::path& checkpoint_model,
    const std::filesystem::path& checkpoint_meta
);

}  // namespace nmc::infrastructure::artifacts
