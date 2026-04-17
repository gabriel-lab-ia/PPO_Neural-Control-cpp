#pragma once

#include <filesystem>
#include <string>

#include <torch/torch.h>

namespace nmc::domain::inference {

enum class InferencePrecision {
    kFp32,
    kFp16,
    kInt8
};

struct InferenceOutput {
    torch::Tensor action;
    torch::Tensor value;
};

struct InferenceBackendCapabilities {
    bool supports_dynamic_shapes = false;
    bool supports_fp16 = false;
    bool supports_int8 = false;
    bool uses_cuda = false;
    bool is_emulated = false;
    std::string runtime = "unknown";
    InferencePrecision configured_precision = InferencePrecision::kFp32;
};

class PolicyInferenceBackend {
public:
    virtual ~PolicyInferenceBackend() = default;

    virtual std::string backend_name() const = 0;
    virtual InferenceBackendCapabilities capabilities() const = 0;
    virtual void load_checkpoint(const std::filesystem::path& checkpoint_path) = 0;
    virtual InferenceOutput infer(const torch::Tensor& observation, bool deterministic) = 0;
};

}  // namespace nmc::domain::inference
