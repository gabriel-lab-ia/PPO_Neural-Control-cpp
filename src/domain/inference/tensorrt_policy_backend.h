#pragma once

#include <filesystem>
#include <string>

#include "domain/inference/policy_inference_backend.h"
#include "domain/inference/tensorrt_policy_backend_stub.h"

namespace nmc::domain::inference {

// TensorRT backend front-controller.
// Current baseline keeps full CLI compatibility by using emulation mode for
// .pt checkpoints while exposing explicit capability/runtime metadata.
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
    TensorRtPolicyBackendStub emulated_backend_;
    InferencePrecision precision_ = InferencePrecision::kFp32;
    bool native_runtime_compiled_ = false;
    bool native_runtime_selected_ = false;
};

bool tensor_rt_native_runtime_compiled();

}  // namespace nmc::domain::inference
