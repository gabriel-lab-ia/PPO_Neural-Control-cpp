#pragma once

#include <optional>
#include <string>
#include <vector>

#include "domain/types.h"
#include "persistence/sqlite_store.h"
#include "telemetry/csv_telemetry_store.h"

namespace orbital::backend::replay {

struct ReplayWindow {
    std::string run_id;
    std::vector<domain::TelemetrySample> samples;
    std::vector<domain::EventRecord> events;
    std::optional<domain::RunRecord> run;
};

class ReplayService {
public:
    ReplayService(const persistence::SQLiteStore& store, const telemetry::CsvTelemetryStore& telemetry_store);

    [[nodiscard]] ReplayWindow build_window(
        const std::string& run_id,
        std::int64_t start_step,
        std::int64_t end_step,
        std::int64_t downsample_points,
        std::int64_t event_limit
    ) const;

private:
    const persistence::SQLiteStore& store_;
    const telemetry::CsvTelemetryStore& telemetry_store_;
};

}  // namespace orbital::backend::replay
