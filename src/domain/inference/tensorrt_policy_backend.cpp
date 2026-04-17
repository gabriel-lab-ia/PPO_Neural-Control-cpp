#include "domain/inference/tensorrt_policy_backend.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace nmc::domain::inference {
namespace {

bool has_native_tensorrt_extension(const std::filesystem::path& checkpoint_path) {
    std::string extension = checkpoint_path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return extension == ".engine" || extension == ".plan" || extension == ".onnx";
}

std::string runtime_name(const bool native_runtime_compiled, const bool native_runtime_selected) {
    if (native_runtime_selected) {
        return "tensorrt_native";
    }
    if (native_runtime_compiled) {
        return "tensorrt_compiled_emulation";
    }
    return "tensorrt_stub_emulation";
}

}  // namespace

TensorRtPolicyBackend::TensorRtPolicyBackend(
    const int64_t observation_dim,
    const int64_t action_dim,
    const int64_t hidden_dim,
    const InferencePrecision precision
)
    : emulated_backend_(observation_dim, action_dim, hidden_dim, precision),
      precision_(precision),
#if defined(NMC_WITH_TENSORRT)
      native_runtime_compiled_(true),
#else
      native_runtime_compiled_(false),
#endif
      native_runtime_selected_(false) {}

std::string TensorRtPolicyBackend::backend_name() const {
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

InferenceBackendCapabilities TensorRtPolicyBackend::capabilities() const {
    auto caps = emulated_backend_.capabilities();
    caps.uses_cuda = native_runtime_selected_;
    caps.is_emulated = !native_runtime_selected_;
    caps.runtime = runtime_name(native_runtime_compiled_, native_runtime_selected_);
    caps.configured_precision = precision_;
    return caps;
}

void TensorRtPolicyBackend::load_checkpoint(const std::filesystem::path& checkpoint_path) {
    if (has_native_tensorrt_extension(checkpoint_path)) {
#if defined(NMC_WITH_TENSORRT)
        throw std::runtime_error(
            "native TensorRT artifact requested ('" + checkpoint_path.string() +
            "') but this baseline build currently supports .pt parity/emulation mode only. "
            "Use a .pt checkpoint for parity mode or enable a dedicated TensorRT engine path."
        );
#else
        throw std::runtime_error(
            "TensorRT native artifact requested ('" + checkpoint_path.string() +
            "') but this build has no TensorRT runtime support. Rebuild with NMC_ENABLE_TENSORRT=ON "
            "and TensorRT SDK available."
        );
#endif
    }

    native_runtime_selected_ = false;
    emulated_backend_.load_checkpoint(checkpoint_path);
}

InferenceOutput TensorRtPolicyBackend::infer(const torch::Tensor& observation, const bool deterministic) {
    return emulated_backend_.infer(observation, deterministic);
}

bool tensor_rt_native_runtime_compiled() {
#if defined(NMC_WITH_TENSORRT)
    return true;
#else
    return false;
#endif
}

}  // namespace nmc::domain::inference
