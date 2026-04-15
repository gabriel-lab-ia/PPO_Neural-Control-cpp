#pragma once

#include <memory>
#include <string>

#include "domain/inference/policy_inference_backend.h"

namespace nmc::domain::inference {

std::unique_ptr<PolicyInferenceBackend> make_inference_backend(
    const std::string& backend,
    int64_t observation_dim,
    int64_t action_dim,
    int64_t hidden_dim
);

}  // namespace nmc::domain::inference
