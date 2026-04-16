#pragma once

#include <string_view>

namespace orbital::backend::common {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

void log(LogLevel level, std::string_view message, std::string_view correlation_id = "");

}  // namespace orbital::backend::common
