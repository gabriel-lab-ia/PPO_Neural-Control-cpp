#include "domain/inference/tensorrt_policy_backend.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>

namespace nmc::domain::inference {
namespace {

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool has_native_tensorrt_extension(const std::filesystem::path& model_path) {
    std::string extension = model_path.extension().string();
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
    : fallback_backend_(observation_dim, action_dim, hidden_dim),
      precision_(precision),
#if defined(NMC_WITH_TENSORRT)
      native_runtime_compiled_(true),
#else
      native_runtime_compiled_(false),
#endif
      native_runtime_selected_(false) {
#if defined(NMC_WITH_TENSORRT)
    native_backend_ = std::make_unique<TensorRtNativeBackend>(
        observation_dim,
        action_dim,
        precision
    );
#endif
}

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
    if (native_runtime_selected_ && native_backend_ != nullptr && native_backend_->is_loaded()) {
        auto native_caps = native_backend_->capabilities();
        native_caps.runtime = runtime_name(native_runtime_compiled_, true);
        return native_caps;
    }

    auto fallback_caps = fallback_backend_.capabilities();
    fallback_caps.supports_fp16 = true;
    fallback_caps.supports_int8 = true;
    fallback_caps.uses_cuda = false;
    fallback_caps.is_emulated = true;
    fallback_caps.runtime = fallback_runtime_;
    fallback_caps.configured_precision = precision_;
    return fallback_caps;
}

std::optional<std::filesystem::path> TensorRtPolicyBackend::resolve_libtorch_fallback_checkpoint(
    const std::filesystem::path& model_path
) const {
    const auto extension = lower_copy(model_path.extension().string());
    if (extension == ".pt") {
        return model_path;
    }

    const auto stem_pt = model_path.parent_path() / (model_path.stem().string() + ".pt");
    if (std::filesystem::exists(stem_pt)) {
        return stem_pt;
    }

    const auto checkpoint_pt = model_path.parent_path() / "checkpoint.pt";
    if (std::filesystem::exists(checkpoint_pt)) {
        return checkpoint_pt;
    }

    return std::nullopt;
}

void TensorRtPolicyBackend::load_native_or_throw(const std::filesystem::path& model_path) {
    if (!native_runtime_compiled_ || native_backend_ == nullptr) {
        throw std::runtime_error(
            "TensorRT native backend unavailable in this build. Rebuild with NMC_ENABLE_TENSORRT=ON."
        );
    }

    auto options = native_backend_->default_build_options();
    options.requested_precision = precision_;

    if (has_native_tensorrt_extension(model_path)) {
        native_backend_->load_from_onnx_or_engine(model_path, options);
        native_runtime_selected_ = true;
        return;
    }

    const auto extension = lower_copy(model_path.extension().string());
    if (extension == ".pt") {
        const auto sidecar_onnx = model_path.parent_path() / (model_path.stem().string() + ".onnx");
        if (std::filesystem::exists(sidecar_onnx)) {
            native_backend_->load_from_onnx_or_engine(sidecar_onnx, options);
            native_runtime_selected_ = true;
            return;
        }

        const auto checkpoint_onnx = model_path.parent_path() / "checkpoint.onnx";
        if (std::filesystem::exists(checkpoint_onnx)) {
            native_backend_->load_from_onnx_or_engine(checkpoint_onnx, options);
            native_runtime_selected_ = true;
            return;
        }
    }

    throw std::runtime_error(
        "no TensorRT-native artifact found. Provide .onnx/.engine/.plan or checkpoint sidecar .onnx"
    );
}

void TensorRtPolicyBackend::load_libtorch_fallback_or_throw(const std::filesystem::path& model_path) {
    const auto fallback_checkpoint = resolve_libtorch_fallback_checkpoint(model_path);
    if (!fallback_checkpoint.has_value()) {
        throw std::runtime_error(
            "TensorRT fallback to LibTorch failed: no .pt checkpoint found near " + model_path.string()
        );
    }

    fallback_backend_.load_checkpoint(*fallback_checkpoint);
    native_runtime_selected_ = false;
    fallback_runtime_ = "tensorrt_fallback_libtorch";
}

void TensorRtPolicyBackend::load_checkpoint(const std::filesystem::path& checkpoint_path) {
    native_runtime_selected_ = false;

    const auto extension = lower_copy(checkpoint_path.extension().string());
    bool should_try_native = has_native_tensorrt_extension(checkpoint_path);

    if (!should_try_native && extension == ".pt" && native_runtime_compiled_) {
        const auto sidecar_onnx = checkpoint_path.parent_path() / (checkpoint_path.stem().string() + ".onnx");
        const auto checkpoint_onnx = checkpoint_path.parent_path() / "checkpoint.onnx";
        should_try_native = std::filesystem::exists(sidecar_onnx) || std::filesystem::exists(checkpoint_onnx);
    }

    if (should_try_native) {
        try {
            load_native_or_throw(checkpoint_path);
            return;
        } catch (const std::exception& native_error) {
            std::cerr << "[TensorRT] native initialization failed, fallback -> LibTorch: "
                      << native_error.what() << '\n';
        }
    }

    load_libtorch_fallback_or_throw(checkpoint_path);
}

InferenceOutput TensorRtPolicyBackend::infer(const torch::Tensor& observation, const bool deterministic) {
    if (native_runtime_selected_ && native_backend_ != nullptr && native_backend_->is_loaded()) {
        return native_backend_->infer(observation, deterministic);
    }
    return fallback_backend_.infer(observation, deterministic);
}

bool tensor_rt_native_runtime_compiled() {
#if defined(NMC_WITH_TENSORRT)
    return true;
#else
    return false;
#endif
}

}  // namespace nmc::domain::inference
