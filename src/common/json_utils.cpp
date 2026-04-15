#include "common/json_utils.h"

namespace nmc::common {

std::string json_escape(std::string_view raw) {
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

}  // namespace nmc::common
