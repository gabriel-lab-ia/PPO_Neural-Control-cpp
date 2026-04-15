#pragma once

#include <string>
#include <string_view>

namespace nmc::common {

std::string now_utc_iso8601();
std::string make_run_id(std::string_view prefix);

}  // namespace nmc::common
