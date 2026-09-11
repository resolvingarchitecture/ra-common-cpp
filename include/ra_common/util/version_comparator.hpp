#pragma once

#include <string>

namespace ra::common {

namespace detail {

inline bool IsSeparator(char c) { return c == '.' || c == '-' || c == '_'; }

inline size_t NextSeparator(const std::string& s, size_t start) {
    size_t i = start;
    while (i < s.size() && !IsSeparator(s[i])) i++;
    return i;
}

inline long ParseLong(const std::string& s, size_t start, size_t end) {
    long rv = 0;
    bool parsed_any = false;
    for (size_t i = start; i < end && rv >= 0; i++) {
        char c = s[i];
        if (c >= '0' && c <= '9') {
            parsed_any = true;
            rv = rv * 10 + (c - '0');
        }
    }
    return parsed_any ? rv : -1;
}

}  // namespace detail

/// Compare two version strings loosely. Returns -1 / 0 / 1.
inline int VersionCompare(const std::string& left, const std::string& right) {
    if (left == right) return 0;
    size_t ll = left.size();
    size_t rl = right.size();
    size_t il = 0;
    size_t ir = 0;

    for (;;) {
        if (il >= ll) return ir >= rl ? 0 : -1;
        if (ir >= rl) return 1;

        long lv = -1;
        while (lv == -1 && il < ll) {
            size_t nl = detail::NextSeparator(left, il);
            lv = detail::ParseLong(left, il, nl);
            il = nl + 1;
        }
        long rv = -1;
        while (rv == -1 && ir < rl) {
            size_t nr = detail::NextSeparator(right, ir);
            rv = detail::ParseLong(right, ir, nr);
            ir = nr + 1;
        }
        if (lv < rv) return -1;
        if (lv > rv) return 1;
    }
}

}  // namespace ra::common
