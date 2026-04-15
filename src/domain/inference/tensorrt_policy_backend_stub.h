#pragma once

#include <filesystem>
#include <string>

#include "domain/inference/policy_inference_backend.h"

namespace nmc::domain::inference {

class TensorRtPolicyBackendStub final : public PolicyInferenceBackend {
public:
    std::string backend_name() const override;
    void load_checkpoint(const std::filesystem::path& checkpoint_path) override;
    InferenceOutput infer(const torch::Tensor& observation, bool deterministic) override;
};

}  // namespace nmc::domain::inference
