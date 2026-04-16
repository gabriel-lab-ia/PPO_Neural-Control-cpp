#pragma once

#include <filesystem>
#include <string>

#include "domain/ppo/policy_value_model.h"
#include "domain/inference/policy_inference_backend.h"

namespace nmc::domain::inference {

class TensorRtPolicyBackendStub final : public PolicyInferenceBackend {
public:
    TensorRtPolicyBackendStub(
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
    static torch::Tensor emulate_precision(torch::Tensor value, InferencePrecision precision);

    torch::Device device_{torch::kCPU};
    ppo::PolicyValueModel model_{nullptr};
    InferencePrecision precision_ = InferencePrecision::kFp32;
};

}  // namespace nmc::domain::inference
