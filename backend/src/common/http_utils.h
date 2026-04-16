#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace orbital::backend::common {

struct RequestTarget {
    std::string path;
    std::unordered_map<std::string, std::string> query;
};

RequestTarget parse_target(std::string_view target);
std::vector<std::string> split_path(std::string_view path);
std::int64_t parse_int_or(std::string_view raw, std::int64_t fallback);
double parse_double_or(std::string_view raw, double fallback);

}  // namespace orbital::backend::common
