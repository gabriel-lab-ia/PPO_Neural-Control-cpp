#pragma once

#include <filesystem>
#include <string>

#include <torch/torch.h>

namespace nmc::domain::inference {

struct InferenceOutput {
    torch::Tensor action;
    torch::Tensor value;
};

class PolicyInferenceBackend {
public:
    virtual ~PolicyInferenceBackend() = default;

    virtual std::string backend_name() const = 0;
    virtual void load_checkpoint(const std::filesystem::path& checkpoint_path) = 0;
    virtual InferenceOutput infer(const torch::Tensor& observation, bool deterministic) = 0;
};

}  // namespace nmc::domain::inference
