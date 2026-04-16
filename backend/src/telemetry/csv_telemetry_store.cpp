#include "telemetry/csv_telemetry_store.h"

#include "common/time_utils.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string_view>

namespace orbital::backend::telemetry {
namespace {

constexpr double kNominalRadiusKm = 6800.0;
constexpr double kThetaStep = 0.0275;

std::vector<std::string> split_csv_line(std::string_view line) {
    std::vector<std::string> fields;
    fields.reserve(16);

    std::string current;
    current.reserve(line.size());

    bool in_quotes = false;
    for (const char ch : line) {
        if (ch == '"') {
            in_quotes = !in_quotes;
            continue;
        }

        if (ch == ',' && !in_quotes) {
            fields.emplace_back(std::move(current));
            current.clear();
            continue;
        }

        current.push_back(ch);
    }

    fields.emplace_back(std::move(current));
    return fields;
}

double parse_double_or(const std::string& raw, const double fallback) {
    if (raw.empty()) {
        return fallback;
    }
    try {
        return std::stod(raw);
    } catch (...) {
        return fallback;
    }
}

std::int64_t parse_int_or(const std::string& raw, const std::int64_t fallback) {
    if (raw.empty()) {
        return fallback;
    }
    try {
        return static_cast<std::int64_t>(std::stoll(raw));
    } catch (...) {
        return fallback;
    }
}

domain::TelemetrySample from_rollout_row(const std::vector<std::string>& fields) {
    domain::TelemetrySample sample;

    sample.step = parse_int_or(fields.size() > 0 ? fields[0] : "", 0);
    sample.reward = parse_double_or(fields.size() > 1 ? fields[1] : "", 0.0);
    sample.action = parse_double_or(fields.size() > 2 ? fields[2] : "", 0.0);
    sample.value = parse_double_or(fields.size() > 3 ? fields[3] : "", 0.0);

    const double obs0 = parse_double_or(fields.size() > 4 ? fields[4] : "", 0.0);
    const double obs1 = parse_double_or(fields.size() > 5 ? fields[5] : "", 0.0);
    const double obs2 = parse_double_or(fields.size() > 6 ? fields[6] : "", 0.0);
    const double obs3 = parse_double_or(fields.size() > 7 ? fields[7] : "", 0.0);

    sample.terminated = parse_int_or(fields.size() > 8 ? fields[8] : "", 0) != 0;
    sample.truncated = parse_int_or(fields.size() > 9 ? fields[9] : "", 0) != 0;

    sample.mission_time_s = static_cast<double>(sample.step);
    sample.control_magnitude = std::abs(sample.action);
    sample.policy_std = 0.10 + 0.06 * std::abs(obs3);

    const double theta = static_cast<double>(sample.step) * kThetaStep;
    const double radius_km = kNominalRadiusKm + obs0 * 120.0 + obs1 * 35.0;
    const double altitude_bias_km = obs2 * 90.0;

    sample.position_km = {
        radius_km * std::cos(theta),
        radius_km * std::sin(theta),
        altitude_bias_km,
    };

    const double omega = 7.6 / std::max(1.0, radius_km / kNominalRadiusKm);
    sample.velocity_kmps = {
        -omega * std::sin(theta),
        omega * std::cos(theta),
        0.08 * std::cos(theta * 0.4),
    };

    sample.velocity_magnitude_kmps = std::sqrt(
        sample.velocity_kmps[0] * sample.velocity_kmps[0] +
        sample.velocity_kmps[1] * sample.velocity_kmps[1] +
        sample.velocity_kmps[2] * sample.velocity_kmps[2]
    );

    sample.orbital_error_km = radius_km - kNominalRadiusKm;
    sample.control_vector = {
        sample.action * std::cos(theta),
        sample.action * std::sin(theta),
        0.5 * sample.action * std::cos(theta * 0.7),
    };
    sample.timestamp = common::now_utc_iso8601();

    return sample;
}

}  // namespace

CsvTelemetryStore::CsvTelemetryStore(std::filesystem::path artifact_root)
    : artifact_root_(std::move(artifact_root)) {}

std::filesystem::path CsvTelemetryStore::resolve_run_file(const std::string& run_id) const {
    const auto run_csv = artifact_root_ / "runs" / run_id / "live_rollout.csv";
    if (std::filesystem::exists(run_csv)) {
        return run_csv;
    }

    const auto latest_csv = artifact_root_ / "latest" / "live_rollout.csv";
    if (std::filesystem::exists(latest_csv)) {
        return latest_csv;
    }

    return run_csv;
}

std::vector<domain::TelemetrySample> CsvTelemetryStore::load_run_samples(const std::string& run_id) const {
    const std::filesystem::path source = resolve_run_file(run_id);
    std::ifstream stream(source);
    if (!stream.is_open()) {
        return {};
    }

    std::vector<domain::TelemetrySample> samples;
    samples.reserve(2048);

    std::string line;
    bool skip_header = true;
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        if (skip_header) {
            skip_header = false;
            continue;
        }

        auto fields = split_csv_line(line);
        if (fields.size() < 4) {
            continue;
        }
        samples.emplace_back(from_rollout_row(fields));
    }

    return samples;
}

std::vector<domain::TelemetrySample> clip_by_step_window(
    const std::vector<domain::TelemetrySample>& source,
    const std::int64_t start_step,
    const std::int64_t end_step
) {
    if (source.empty()) {
        return {};
    }

    std::vector<domain::TelemetrySample> clipped;
    clipped.reserve(source.size());
    for (const auto& sample : source) {
        if (sample.step < start_step) {
            continue;
        }
        if (end_step >= 0 && sample.step > end_step) {
            continue;
        }
        clipped.push_back(sample);
    }
    return clipped;
}

std::vector<domain::TelemetrySample> downsample_samples(
    const std::vector<domain::TelemetrySample>& source,
    const std::int64_t max_points
) {
    if (source.empty() || max_points <= 0 || static_cast<std::int64_t>(source.size()) <= max_points) {
        return source;
    }

    const double stride = static_cast<double>(source.size() - 1U) / static_cast<double>(max_points - 1);

    std::vector<domain::TelemetrySample> sampled;
    sampled.reserve(static_cast<std::size_t>(max_points));
    for (std::int64_t i = 0; i < max_points; ++i) {
        const std::size_t index = static_cast<std::size_t>(std::round(stride * static_cast<double>(i)));
        sampled.push_back(source.at(std::min(index, source.size() - 1U)));
    }
    return sampled;
}

}  // namespace orbital::backend::telemetry
