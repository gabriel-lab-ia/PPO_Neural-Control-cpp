#include "common/determinism.h"

#include <ATen/Context.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>

#include <torch/torch.h>

namespace nmc::common {

namespace {

std::atomic<bool> g_torch_runtime_configured{false};

}  // namespace

void configure_determinism(const std::uint64_t seed, const int64_t torch_num_threads) {
    std::srand(static_cast<unsigned int>(seed));
    torch::manual_seed(seed);

    // PyTorch threadpool settings are process-global and must be configured
    // before parallel work starts. We set them once, then only reseed per run.
    if (!g_torch_runtime_configured.exchange(true)) {
        at::globalContext().setDeterministicAlgorithms(true, false);
        at::globalContext().setBenchmarkCuDNN(false);

        if (torch_num_threads > 0) {
            const int threads = static_cast<int>(std::clamp<int64_t>(torch_num_threads, 1, 64));
            torch::set_num_threads(threads);
            torch::set_num_interop_threads(std::min(threads, 4));
        }
    }
}

}  // namespace nmc::common
