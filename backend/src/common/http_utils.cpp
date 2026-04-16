#include "common/http_utils.h"

#include <charconv>

namespace orbital::backend::common {
namespace {

std::string percent_decode(std::string_view value) {
    std::string decoded;
    decoded.reserve(value.size());

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (ch == '%' && i + 2 < value.size()) {
            const char hex[2] = {value[i + 1], value[i + 2]};
            unsigned int code = 0;
            if (std::from_chars(hex, hex + 2, code, 16).ec == std::errc{}) {
                decoded.push_back(static_cast<char>(code));
                i += 2;
                continue;
            }
        }

        if (ch == '+') {
            decoded.push_back(' ');
        } else {
            decoded.push_back(ch);
        }
    }

    return decoded;
}

}  // namespace

RequestTarget parse_target(const std::string_view target) {
    RequestTarget parsed;

    const std::size_t query_pos = target.find('?');
    parsed.path = std::string(target.substr(0, query_pos));

    if (query_pos == std::string_view::npos || query_pos + 1 >= target.size()) {
        return parsed;
    }

    std::string_view raw_query = target.substr(query_pos + 1);
    while (!raw_query.empty()) {
        const std::size_t amp_pos = raw_query.find('&');
        const std::string_view pair = raw_query.substr(0, amp_pos);

        const std::size_t equals_pos = pair.find('=');
        if (equals_pos != std::string_view::npos) {
            const auto key = percent_decode(pair.substr(0, equals_pos));
            const auto value = percent_decode(pair.substr(equals_pos + 1));
            parsed.query.insert_or_assign(key, value);
        } else {
            parsed.query.insert_or_assign(percent_decode(pair), "");
        }

        if (amp_pos == std::string_view::npos) {
            break;
        }
        raw_query.remove_prefix(amp_pos + 1);
    }

    return parsed;
}

std::vector<std::string> split_path(const std::string_view path) {
    std::vector<std::string> segments;
    std::size_t cursor = 0;
    while (cursor < path.size()) {
        while (cursor < path.size() && path[cursor] == '/') {
            ++cursor;
        }
        if (cursor >= path.size()) {
            break;
        }

        std::size_t end = cursor;
        while (end < path.size() && path[end] != '/') {
            ++end;
        }

        segments.emplace_back(path.substr(cursor, end - cursor));
        cursor = end;
    }
    return segments;
}

std::int64_t parse_int_or(const std::string_view raw, const std::int64_t fallback) {
    if (raw.empty()) {
        return fallback;
    }

    std::int64_t parsed = fallback;
    const auto [_, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), parsed);
    return ec == std::errc{} ? parsed : fallback;
}

double parse_double_or(const std::string_view raw, const double fallback) {
    if (raw.empty()) {
        return fallback;
    }

    try {
        return std::stod(std::string(raw));
    } catch (...) {
        return fallback;
    }
}

}  // namespace orbital::backend::common
