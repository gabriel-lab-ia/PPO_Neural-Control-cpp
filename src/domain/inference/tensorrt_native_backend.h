#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

#include "domain/inference/policy_inference_backend.h"

namespace nmc::domain::inference {

struct TensorRtBuildOptions {
    InferencePrecision requested_precision = InferencePrecision::kFp16;
    int32_t max_batch_size = 16;
    std::size_t workspace_size_bytes = (1ULL << 30U);
    bool enable_dynamic_shapes = true;
    bool allow_tf32 = true;
    bool force_rebuild = false;
    bool enable_engine_cache = true;
    std::filesystem::path engine_cache_dir;
    int32_t calibration_batch_size = 8;
    int32_t calibration_batches = 32;
    uint64_t calibration_seed = 7;
    std::filesystem::path calibration_cache_path;
};

class TensorRtNativeBackend {
public:
    TensorRtNativeBackend(int64_t observation_dim, int64_t action_dim, InferencePrecision requested_precision);
    ~TensorRtNativeBackend();

    TensorRtNativeBackend(const TensorRtNativeBackend&) = delete;
    TensorRtNativeBackend& operator=(const TensorRtNativeBackend&) = delete;
    TensorRtNativeBackend(TensorRtNativeBackend&&) noexcept;
    TensorRtNativeBackend& operator=(TensorRtNativeBackend&&) noexcept;

    TensorRtBuildOptions default_build_options() const;
    void load_from_onnx_or_engine(const std::filesystem::path& model_path, const TensorRtBuildOptions& options);
    InferenceOutput infer(const torch::Tensor& observation, bool deterministic);
    InferenceBackendCapabilities capabilities() const;

    bool is_loaded() const noexcept;
    std::filesystem::path active_engine_path() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

std::filesystem::path default_engine_path_for_onnx(
    const std::filesystem::path& onnx_path,
    InferencePrecision precision
);

}  // namespace nmc::domain::inference
