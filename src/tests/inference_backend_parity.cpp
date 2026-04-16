#include <filesystem>
#include <iostream>

#include <torch/torch.h>

#include "common/determinism.h"
#include "domain/inference/inference_backend_factory.h"
#include "domain/ppo/policy_value_model.h"

namespace {

float max_abs_diff(const torch::Tensor& lhs, const torch::Tensor& rhs) {
    return torch::max(torch::abs(lhs - rhs)).item<float>();
}

bool check_with_tolerance(
    nmc::domain::inference::PolicyInferenceBackend& reference,
    nmc::domain::inference::PolicyInferenceBackend& candidate,
    const torch::Tensor& observation,
    const float tolerance
) {
    const auto out_ref = reference.infer(observation, true);
    const auto out_candidate = candidate.infer(observation, true);

    const float action_diff = max_abs_diff(out_ref.action, out_candidate.action);
    const float value_diff = max_abs_diff(out_ref.value, out_candidate.value);

    std::cout << candidate.backend_name() << " | action_diff=" << action_diff << " | value_diff=" << value_diff
              << " | tolerance=" << tolerance << '\n';

    return action_diff <= tolerance && value_diff <= tolerance;
}

}  // namespace

int main() {
    constexpr int64_t kObsDim = 4;
    constexpr int64_t kActDim = 1;
    constexpr int64_t kHidden = 32;

    nmc::common::configure_determinism(7, 1);

    auto model = nmc::domain::ppo::PolicyValueModel(kObsDim, kActDim, kHidden);
    const auto checkpoint = std::filesystem::temp_directory_path() / "nmc_backend_parity.pt";
    torch::save(model, checkpoint.string());

    auto libtorch = nmc::domain::inference::make_inference_backend(
        nmc::domain::inference::InferenceBackendKind::kLibTorch,
        kObsDim,
        kActDim,
        kHidden
    );
    auto tensorrt_fp16 = nmc::domain::inference::make_inference_backend(
        nmc::domain::inference::InferenceBackendKind::kTensorRtFp16,
        kObsDim,
        kActDim,
        kHidden
    );
    auto tensorrt_int8 = nmc::domain::inference::make_inference_backend(
        nmc::domain::inference::InferenceBackendKind::kTensorRtInt8,
        kObsDim,
        kActDim,
        kHidden
    );

    libtorch->load_checkpoint(checkpoint);
    tensorrt_fp16->load_checkpoint(checkpoint);
    tensorrt_int8->load_checkpoint(checkpoint);

    const auto observation = torch::tensor({0.12f, -0.35f, 0.44f, -0.05f}, torch::TensorOptions().dtype(torch::kFloat32));

    bool ok = true;
    ok = ok && check_with_tolerance(*libtorch, *tensorrt_fp16, observation, 2.0e-2f);
    ok = ok && check_with_tolerance(*libtorch, *tensorrt_int8, observation, 8.0e-2f);

    std::error_code ec;
    std::filesystem::remove(checkpoint, ec);

    if (!ok) {
        std::cerr << "inference backend parity check failed" << '\n';
        return 1;
    }

    std::cout << "inference backend parity check passed" << '\n';
    return 0;
}
