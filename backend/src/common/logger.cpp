#include "common/logger.h"

#include "common/time_utils.h"

#include <iostream>
#include <mutex>

namespace orbital::backend::common {
namespace {

std::mutex& log_mutex() {
    static std::mutex mutex;
    return mutex;
}

const char* to_string(const LogLevel level) {
    switch (level) {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "UNKNOWN";
}

}  // namespace

void log(const LogLevel level, const std::string_view message, const std::string_view correlation_id) {
    std::lock_guard<std::mutex> lock(log_mutex());
    std::cerr << '[' << now_utc_iso8601() << "] [" << to_string(level) << ']';
    if (!correlation_id.empty()) {
        std::cerr << " [run_id=" << correlation_id << ']';
    }
    std::cerr << ' ' << message << '\n';
}

}  // namespace orbital::backend::common
