#include "domain/inference/inference_benchmark.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numeric>

namespace nmc::domain::inference {
namespace {

float percentile_approx(std::vector<float> samples, const float percentile) {
    if (samples.empty()) {
        return 0.0f;
    }

    const auto clamped = std::clamp(percentile, 0.0f, 1.0f);
    const auto index = static_cast<std::size_t>(std::ceil(
        clamped * static_cast<float>(samples.size() - 1U)
    ));
    std::nth_element(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(index), samples.end());
    return samples[index];
}

}  // namespace

InferenceLatencyStats benchmark_inference_latency(
    PolicyInferenceBackend& backend,
    const torch::Tensor& observation,
    const bool deterministic,
    const int64_t warmup_iterations,
    const int64_t measured_iterations
) {
    InferenceLatencyStats stats;
    stats.warmup_iterations = std::max<int64_t>(0, warmup_iterations);
    stats.measured_iterations = std::max<int64_t>(1, measured_iterations);

    for (int64_t i = 0; i < stats.warmup_iterations; ++i) {
        static_cast<void>(backend.infer(observation, deterministic));
    }

    std::vector<float> samples_ms;
    samples_ms.reserve(static_cast<std::size_t>(stats.measured_iterations));

    for (int64_t i = 0; i < stats.measured_iterations; ++i) {
        const auto start = std::chrono::steady_clock::now();
        static_cast<void>(backend.infer(observation, deterministic));
        const auto end = std::chrono::steady_clock::now();
        samples_ms.push_back(std::chrono::duration<float, std::milli>(end - start).count());
    }

    const auto sum_ms = std::accumulate(samples_ms.begin(), samples_ms.end(), 0.0f);
    stats.avg_ms = sum_ms / static_cast<float>(samples_ms.size());
    stats.p95_ms = percentile_approx(samples_ms, 0.95f);
    stats.min_ms = *std::min_element(samples_ms.begin(), samples_ms.end());
    stats.max_ms = *std::max_element(samples_ms.begin(), samples_ms.end());
    return stats;
}

InferenceLatencyComparison compare_backend_latency(
    PolicyInferenceBackend& baseline_backend,
    PolicyInferenceBackend& candidate_backend,
    const torch::Tensor& observation,
    const bool deterministic,
    const int64_t warmup_iterations,
    const int64_t measured_iterations
) {
    InferenceLatencyComparison result;
    result.baseline = benchmark_inference_latency(
        baseline_backend,
        observation,
        deterministic,
        warmup_iterations,
        measured_iterations
    );
    result.candidate = benchmark_inference_latency(
        candidate_backend,
        observation,
        deterministic,
        warmup_iterations,
        measured_iterations
    );

    if (result.candidate.avg_ms > std::numeric_limits<float>::epsilon()) {
        result.speedup_vs_baseline = result.baseline.avg_ms / result.candidate.avg_ms;
    } else {
        result.speedup_vs_baseline = 1.0f;
    }
    return result;
}

}  // namespace nmc::domain::inference
