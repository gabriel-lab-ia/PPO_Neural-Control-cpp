#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "domain/types.h"

namespace orbital::backend::telemetry {

class CsvTelemetryStore {
public:
    explicit CsvTelemetryStore(std::filesystem::path artifact_root);

    std::vector<domain::TelemetrySample> load_run_samples(const std::string& run_id) const;

private:
    [[nodiscard]] std::filesystem::path resolve_run_file(const std::string& run_id) const;

    std::filesystem::path artifact_root_;
};

std::vector<domain::TelemetrySample> clip_by_step_window(
    const std::vector<domain::TelemetrySample>& source,
    std::int64_t start_step,
    std::int64_t end_step
);

std::vector<domain::TelemetrySample> downsample_samples(
    const std::vector<domain::TelemetrySample>& source,
    std::int64_t max_points
);

}  // namespace orbital::backend::telemetry
