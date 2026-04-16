#pragma once

#include <sstream>
#include <string>
#include <string_view>

namespace orbital::backend::common {

inline std::string json_escape(const std::string_view raw) {
    std::string escaped;
    escaped.reserve(raw.size());
    for (const char ch : raw) {
        switch (ch) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped.push_back(ch);
                break;
        }
    }
    return escaped;
}

inline std::string json_string(const std::string_view value) {
    return std::string{"\""} + json_escape(value) + "\"";
}

inline std::string ws_envelope(
    const std::string_view message_type,
    const std::string_view schema_version,
    const std::string_view timestamp,
    const std::string_view source,
    const std::string_view run_id,
    const std::string_view payload_json
) {
    std::ostringstream stream;
    stream << '{'
           << "\"type\":" << json_string(message_type) << ','
           << "\"schema_version\":" << json_string(schema_version) << ','
           << "\"timestamp\":" << json_string(timestamp) << ','
           << "\"source\":" << json_string(source) << ','
           << "\"run_id\":" << json_string(run_id) << ','
           << "\"payload\":" << payload_json
           << '}';
    return stream.str();
}

}  // namespace orbital::backend::common
