#include "domain/inference/inference_backend_factory.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "domain/inference/libtorch_policy_backend.h"
#include "domain/inference/tensorrt_policy_backend_stub.h"

namespace nmc::domain::inference {

std::unique_ptr<PolicyInferenceBackend> make_inference_backend(
    const std::string& backend,
    const int64_t observation_dim,
    const int64_t action_dim,
    const int64_t hidden_dim
) {
    std::string normalized = backend;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (normalized.empty() || normalized == "libtorch") {
        return std::make_unique<LibTorchPolicyBackend>(observation_dim, action_dim, hidden_dim);
    }

    if (normalized == "tensorrt" || normalized == "tensorrt_stub") {
        return std::make_unique<TensorRtPolicyBackendStub>();
    }

    throw std::runtime_error("unsupported inference backend: " + backend);
}

}  // namespace nmc::domain::inference
