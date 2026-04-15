#pragma once

#include <filesystem>
#include <string>

#include "domain/inference/policy_inference_backend.h"
#include "domain/ppo/policy_value_model.h"

namespace nmc::domain::inference {

class LibTorchPolicyBackend final : public PolicyInferenceBackend {
public:
    LibTorchPolicyBackend(int64_t observation_dim, int64_t action_dim, int64_t hidden_dim);

    std::string backend_name() const override;
    void load_checkpoint(const std::filesystem::path& checkpoint_path) override;
    InferenceOutput infer(const torch::Tensor& observation, bool deterministic) override;

private:
    torch::Device device_{torch::kCPU};
    ppo::PolicyValueModel model_{nullptr};
};

}  // namespace nmc::domain::inference
