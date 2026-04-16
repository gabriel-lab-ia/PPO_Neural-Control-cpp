#include "replay/replay_service.h"

namespace orbital::backend::replay {

ReplayService::ReplayService(const persistence::SQLiteStore& store, const telemetry::CsvTelemetryStore& telemetry_store)
    : store_(store), telemetry_store_(telemetry_store) {}

ReplayWindow ReplayService::build_window(
    const std::string& run_id,
    const std::int64_t start_step,
    const std::int64_t end_step,
    const std::int64_t downsample_points,
    const std::int64_t event_limit
) const {
    ReplayWindow window;
    window.run_id = run_id;
    window.run = store_.get_run(run_id);

    const auto all_samples = telemetry_store_.load_run_samples(run_id);
    const auto clipped = telemetry::clip_by_step_window(all_samples, start_step, end_step);
    window.samples = telemetry::downsample_samples(clipped, downsample_points);

    const std::int64_t safe_limit = event_limit <= 0 ? 512 : event_limit;
    window.events = store_.list_events(run_id, safe_limit, 0);

    return window;
}

}  // namespace orbital::backend::replay
