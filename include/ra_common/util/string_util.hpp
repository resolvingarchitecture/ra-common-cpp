#pragma once

#include <cctype>
#include <string>

namespace ra::common {

/// Uppercase the first character, leave the rest untouched.
inline std::string CapitalizeFirst(const std::string& text) {
    if (text.empty()) return "";
    std::string out = text;
    out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
    return out;
}

/// Uppercase the first character and every character following a space.
inline std::string Capitalize(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    bool capitalize_next = true;
    for (char ch : text) {
        if (capitalize_next && ch != ' ') {
            out += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            capitalize_next = false;
        } else {
            out += ch;
            capitalize_next = ch == ' ';
        }
    }
    return out;
}

}  // namespace ra::common
