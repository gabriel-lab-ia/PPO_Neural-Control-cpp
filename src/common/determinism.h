#pragma once

#include <cstdint>

namespace nmc::common {

void configure_determinism(std::uint64_t seed, int64_t torch_num_threads);

}  // namespace nmc::common
