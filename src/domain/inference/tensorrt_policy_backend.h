#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "domain/inference/libtorch_policy_backend.h"
#include "domain/inference/policy_inference_backend.h"
#include "domain/inference/tensorrt_native_backend.h"

namespace nmc::domain::inference {

// TensorRT backend front-controller.
// Native path:
//   .onnx -> build/load TensorRT engine
//   .engine/.plan -> load TensorRT engine
// Fallback path:
//   LibTorch backend is used automatically when TensorRT native runtime is
//   unavailable or cannot initialize from provided artifacts.
class TensorRtPolicyBackend final : public PolicyInferenceBackend {
public:
    TensorRtPolicyBackend(
        int64_t observation_dim,
        int64_t action_dim,
        int64_t hidden_dim,
        InferencePrecision precision
    );

    std::string backend_name() const override;
    InferenceBackendCapabilities capabilities() const override;
    void load_checkpoint(const std::filesystem::path& checkpoint_path) override;
    InferenceOutput infer(const torch::Tensor& observation, bool deterministic) override;

private:
    std::optional<std::filesystem::path> resolve_libtorch_fallback_checkpoint(
        const std::filesystem::path& model_path
    ) const;
    void load_native_or_throw(const std::filesystem::path& model_path);
    void load_libtorch_fallback_or_throw(const std::filesystem::path& model_path);

    LibTorchPolicyBackend fallback_backend_;
    std::unique_ptr<TensorRtNativeBackend> native_backend_;
    InferencePrecision precision_ = InferencePrecision::kFp32;
    bool native_runtime_compiled_ = false;
    bool native_runtime_selected_ = false;
    std::string fallback_runtime_ = "tensorrt_fallback_libtorch";
};

bool tensor_rt_native_runtime_compiled();

}  // namespace nmc::domain::inference
