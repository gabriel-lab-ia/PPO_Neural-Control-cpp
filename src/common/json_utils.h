#pragma once

#include <string>
#include <string_view>

namespace nmc::common {

std::string json_escape(std::string_view raw);

}  // namespace nmc::common
