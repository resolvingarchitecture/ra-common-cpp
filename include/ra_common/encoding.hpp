#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "ra_common/exception.hpp"

// Base32 and Base58 string codecs. Ports ra.common.Base32 and ra.common.Base58.
// Since this port is not wire-compatible we use standard implementations:
// base32 is RFC 4648, uppercase A-Z2-7, no padding; base58 is the Bitcoin alphabet.

namespace ra::common {

inline std::string Base32Encode(const std::vector<uint8_t>& data) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int bits = 0;
    int value = 0;
    std::string out;
    for (uint8_t byte : data) {
        value = (value << 8) | byte;
        bits += 8;
        while (bits >= 5) {
            out += kAlphabet[(value >> (bits - 5)) & 0x1f];
            bits -= 5;
        }
    }
    if (bits > 0) out += kAlphabet[(value << (5 - bits)) & 0x1f];
    return out;
}

inline std::vector<uint8_t> Base32Decode(const std::string& text) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int bits = 0;
    int value = 0;
    std::vector<uint8_t> out;
    for (char ch : text) {
        const char* pos = std::strchr(kAlphabet, ch);
        if (pos == nullptr || ch == '\0') throw RaException::Decode(std::string("invalid base32 character: ") + ch);
        int idx = static_cast<int>(pos - kAlphabet);
        value = (value << 5) | idx;
        bits += 5;
        if (bits >= 8) {
            out.push_back(static_cast<uint8_t>((value >> (bits - 8)) & 0xff));
            bits -= 8;
        }
    }
    return out;
}

inline std::string Base58Encode(const std::vector<uint8_t>& data) {
    static constexpr char kAlphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    std::vector<uint8_t> digits;  // base-58 digits, most significant first, grown as needed
    for (uint8_t byte : data) {
        int carry = byte;
        for (auto& d : digits) {
            int x = d * 256 + carry;
            d = static_cast<uint8_t>(x % 58);
            carry = x / 58;
        }
        while (carry > 0) {
            digits.push_back(static_cast<uint8_t>(carry % 58));
            carry /= 58;
        }
    }
    std::string out;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) out += kAlphabet[*it];

    size_t pad = 0;
    for (uint8_t byte : data) {
        if (byte == 0) pad++;
        else break;
    }
    return std::string(pad, kAlphabet[0]) + out;
}

inline std::vector<uint8_t> Base58Decode(const std::string& text) {
    static constexpr char kAlphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    std::vector<uint8_t> bytes;  // little-endian accumulator, grown as needed
    for (char ch : text) {
        const char* pos = std::strchr(kAlphabet, ch);
        if (pos == nullptr || ch == '\0') throw RaException::Decode(std::string("invalid base58 character: ") + ch);
        int carry = static_cast<int>(pos - kAlphabet);
        for (auto& b : bytes) {
            int x = b * 58 + carry;
            b = static_cast<uint8_t>(x & 0xff);
            carry = x >> 8;
        }
        while (carry > 0) {
            bytes.push_back(static_cast<uint8_t>(carry & 0xff));
            carry >>= 8;
        }
    }
    std::vector<uint8_t> out(bytes.rbegin(), bytes.rend());

    size_t pad = 0;
    for (char ch : text) {
        if (ch == kAlphabet[0]) pad++;
        else break;
    }
    std::vector<uint8_t> result(pad, 0);
    result.insert(result.end(), out.begin(), out.end());
    return result;
}

// Standard (RFC 4648 §4) base64 with '=' padding. C++ has no stdlib base64,
// unlike the other ports which lean on their runtime's built-in codec
// (Buffer/Convert/base64 module) - this is the one encoding every other port
// gets for free that had to be hand-rolled here too.

inline std::string Base64Encode(const std::vector<uint8_t>& data) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 3 <= data.size()) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16) | (static_cast<uint32_t>(data[i + 1]) << 8) | data[i + 2];
        out += kAlphabet[(n >> 18) & 0x3f];
        out += kAlphabet[(n >> 12) & 0x3f];
        out += kAlphabet[(n >> 6) & 0x3f];
        out += kAlphabet[n & 0x3f];
        i += 3;
    }
    size_t rem = data.size() - i;
    if (rem == 1) {
        uint32_t n = static_cast<uint32_t>(data[i]) << 16;
        out += kAlphabet[(n >> 18) & 0x3f];
        out += kAlphabet[(n >> 12) & 0x3f];
        out += "==";
    } else if (rem == 2) {
        uint32_t n = (static_cast<uint32_t>(data[i]) << 16) | (static_cast<uint32_t>(data[i + 1]) << 8);
        out += kAlphabet[(n >> 18) & 0x3f];
        out += kAlphabet[(n >> 12) & 0x3f];
        out += kAlphabet[(n >> 6) & 0x3f];
        out += "=";
    }
    return out;
}

inline std::vector<uint8_t> Base64Decode(const std::string& text) {
    auto decode_char = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    std::vector<uint8_t> out;
    int buffer = 0;
    int bits = 0;
    for (char ch : text) {
        if (ch == '=' || ch == '\n' || ch == '\r') continue;
        int val = decode_char(ch);
        if (val < 0) throw RaException::Decode(std::string("invalid base64 character: ") + ch);
        buffer = (buffer << 6) | val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((buffer >> bits) & 0xff));
        }
    }
    return out;
}

}  // namespace ra::common
