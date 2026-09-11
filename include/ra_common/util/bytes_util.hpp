#pragma once

#include <array>
#include <cstdint>
#include <cstring>

namespace ra::common {

/// Interpret the first four bytes of b as a big-endian signed 32-bit int.
inline int32_t PackBigEndian(const uint8_t* b) {
    return static_cast<int32_t>((static_cast<uint32_t>(b[0]) << 24) | (static_cast<uint32_t>(b[1]) << 16) |
                                 (static_cast<uint32_t>(b[2]) << 8) | static_cast<uint32_t>(b[3]));
}

/// Encode x as four big-endian bytes.
inline std::array<uint8_t, 4> UnpackBigEndian(int32_t x) {
    auto ux = static_cast<uint32_t>(x);
    return {static_cast<uint8_t>(ux >> 24), static_cast<uint8_t>(ux >> 16), static_cast<uint8_t>(ux >> 8),
            static_cast<uint8_t>(ux)};
}

/// Interpret the first four bytes of b as a little-endian signed 32-bit int.
inline int32_t PackLittleEndian(const uint8_t* b) {
    return static_cast<int32_t>((static_cast<uint32_t>(b[3]) << 24) | (static_cast<uint32_t>(b[2]) << 16) |
                                 (static_cast<uint32_t>(b[1]) << 8) | static_cast<uint32_t>(b[0]));
}

/// Encode x as four little-endian bytes.
inline std::array<uint8_t, 4> UnpackLittleEndian(int32_t x) {
    auto ux = static_cast<uint32_t>(x);
    return {static_cast<uint8_t>(ux), static_cast<uint8_t>(ux >> 8), static_cast<uint8_t>(ux >> 16),
            static_cast<uint8_t>(ux >> 24)};
}

}  // namespace ra::common
