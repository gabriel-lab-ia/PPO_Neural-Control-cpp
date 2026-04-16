#include "domain/inference/libtorch_policy_backend.h"

#include <stdexcept>

namespace nmc::domain::inference {

LibTorchPolicyBackend::LibTorchPolicyBackend(
    const int64_t observation_dim,
    const int64_t action_dim,
    const int64_t hidden_dim
) {
    model_ = ppo::PolicyValueModel(observation_dim, action_dim, hidden_dim);
    model_->to(device_);
    model_->eval();
}

std::string LibTorchPolicyBackend::backend_name() const {
    return "libtorch";
}

InferenceBackendCapabilities LibTorchPolicyBackend::capabilities() const {
    return {
        .supports_dynamic_shapes = true,
        .supports_fp16 = false,
        .supports_int8 = false,
        .configured_precision = InferencePrecision::kFp32
    };
}

void LibTorchPolicyBackend::load_checkpoint(const std::filesystem::path& checkpoint_path) {
    if (!std::filesystem::exists(checkpoint_path)) {
        throw std::runtime_error("checkpoint not found: " + checkpoint_path.string());
    }

    torch::load(model_, checkpoint_path.string());
    model_->to(device_);
    model_->eval();
}

InferenceOutput LibTorchPolicyBackend::infer(const torch::Tensor& observation, const bool deterministic) {
    torch::NoGradGuard no_grad;
    auto batch_observation = observation;
    if (batch_observation.dim() == 1) {
        batch_observation = batch_observation.unsqueeze(0);
    }

    const auto output = model_->act(batch_observation.to(device_), deterministic);
    return {
        output.action.to(torch::kCPU),
        output.value.to(torch::kCPU)
    };
}

}  // namespace nmc::domain::inference
