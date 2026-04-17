#include "domain/inference/tensorrt_policy_backend_stub.h"

#include <stdexcept>

namespace nmc::domain::inference {

TensorRtPolicyBackendStub::TensorRtPolicyBackendStub(
    const int64_t observation_dim,
    const int64_t action_dim,
    const int64_t hidden_dim,
    const InferencePrecision precision
)
    : precision_(precision) {
    model_ = ppo::PolicyValueModel(observation_dim, action_dim, hidden_dim);
    model_->to(device_);
    model_->eval();
}

std::string TensorRtPolicyBackendStub::backend_name() const {
    switch (precision_) {
        case InferencePrecision::kFp16:
            return "tensorrt_fp16";
        case InferencePrecision::kInt8:
            return "tensorrt_int8";
        case InferencePrecision::kFp32:
            return "tensorrt";
    }
    return "tensorrt";
}

InferenceBackendCapabilities TensorRtPolicyBackendStub::capabilities() const {
    return {
        .supports_dynamic_shapes = true,
        .supports_fp16 = true,
        .supports_int8 = true,
        .uses_cuda = false,
        .is_emulated = true,
        .runtime = "tensorrt_stub_emulation",
        .configured_precision = precision_
    };
}

void TensorRtPolicyBackendStub::load_checkpoint(const std::filesystem::path& checkpoint_path) {
    if (!std::filesystem::exists(checkpoint_path)) {
        throw std::runtime_error("checkpoint not found: " + checkpoint_path.string());
    }

    torch::load(model_, checkpoint_path.string());
    model_->to(device_);
    model_->eval();
}

InferenceOutput TensorRtPolicyBackendStub::infer(const torch::Tensor& observation, const bool deterministic) {
    torch::NoGradGuard no_grad;

    auto batch_observation = observation;
    if (batch_observation.dim() == 1) {
        batch_observation = batch_observation.unsqueeze(0);
    }

    const auto output = model_->act(batch_observation.to(device_), deterministic);

    return {
        emulate_precision(output.action.to(torch::kCPU), precision_),
        emulate_precision(output.value.to(torch::kCPU), precision_)
    };
}

torch::Tensor TensorRtPolicyBackendStub::emulate_precision(torch::Tensor value, const InferencePrecision precision) {
    switch (precision) {
        case InferencePrecision::kFp16:
            return value.to(torch::kFloat16).to(torch::kFloat32);
        case InferencePrecision::kInt8:
            return torch::round(value * 127.0f).clamp(-127.0f, 127.0f) / 127.0f;
        case InferencePrecision::kFp32:
            return value;
    }
    return value;
}

}  // namespace nmc::domain::inference
