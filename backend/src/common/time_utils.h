#pragma once

#include <string>

namespace orbital::backend::common {

std::string now_utc_iso8601();
std::string make_id(const std::string& prefix);

}  // namespace orbital::backend::common
