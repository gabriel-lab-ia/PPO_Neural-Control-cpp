#include "domain/inference/tensorrt_policy_backend_stub.h"

#include <stdexcept>

namespace nmc::domain::inference {

std::string TensorRtPolicyBackendStub::backend_name() const {
    return "tensorrt_stub";
}

void TensorRtPolicyBackendStub::load_checkpoint(const std::filesystem::path& checkpoint_path) {
    (void)checkpoint_path;
    throw std::runtime_error(
        "TensorRT backend is not enabled in this CPU-first baseline. "
        "Use --backend libtorch for now."
    );
}

InferenceOutput TensorRtPolicyBackendStub::infer(const torch::Tensor& observation, const bool deterministic) {
    (void)observation;
    (void)deterministic;
    throw std::runtime_error(
        "TensorRT backend is currently a placeholder. "
        "Integration point is ready, implementation is intentionally deferred."
    );
}

}  // namespace nmc::domain::inference
