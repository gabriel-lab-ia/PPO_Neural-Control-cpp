#include "common/time_utils.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <random>
#include <sstream>

namespace orbital::backend::common {

std::string now_utc_iso8601() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm utc_time{};
#if defined(_WIN32)
    gmtime_s(&utc_time, &now_time);
#else
    gmtime_r(&now_time, &utc_time);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%SZ");
    return stream.str();
}

std::string make_id(const std::string& prefix) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<int> dist(0, 15);

    std::ostringstream stream;
    stream << prefix << '_' << now_utc_iso8601() << '_';
    for (int i = 0; i < 8; ++i) {
        stream << std::hex << dist(rng);
    }
    return stream.str();
}

}  // namespace orbital::backend::common
