#pragma once

#include <cstdint>
#include <vector>

#include <torch/torch.h>

#include "domain/inference/policy_inference_backend.h"

namespace nmc::domain::inference {

struct InferenceLatencyStats {
    int64_t warmup_iterations = 0;
    int64_t measured_iterations = 0;
    float avg_ms = 0.0f;
    float p95_ms = 0.0f;
    float min_ms = 0.0f;
    float max_ms = 0.0f;
};

InferenceLatencyStats benchmark_inference_latency(
    PolicyInferenceBackend& backend,
    const torch::Tensor& observation,
    bool deterministic,
    int64_t warmup_iterations,
    int64_t measured_iterations
);

struct InferenceLatencyComparison {
    InferenceLatencyStats baseline{};
    InferenceLatencyStats candidate{};
    float speedup_vs_baseline = 1.0f;
};

InferenceLatencyComparison compare_backend_latency(
    PolicyInferenceBackend& baseline_backend,
    PolicyInferenceBackend& candidate_backend,
    const torch::Tensor& observation,
    bool deterministic,
    int64_t warmup_iterations,
    int64_t measured_iterations
);

}  // namespace nmc::domain::inference
