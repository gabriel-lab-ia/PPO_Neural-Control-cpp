#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "domain/inference/policy_inference_backend.h"

namespace nmc::domain::inference {

enum class InferenceBackendKind {
    kLibTorch,
    kTensorRtFp32,
    kTensorRtFp16,
    kTensorRtInt8
};

std::optional<InferenceBackendKind> try_parse_inference_backend(std::string_view backend);
InferenceBackendKind parse_inference_backend_or_throw(std::string_view backend);
std::string inference_backend_to_string(InferenceBackendKind backend);
std::string supported_inference_backends();

std::unique_ptr<PolicyInferenceBackend> make_inference_backend(
    InferenceBackendKind backend,
    int64_t observation_dim,
    int64_t action_dim,
    int64_t hidden_dim
);

}  // namespace nmc::domain::inference
