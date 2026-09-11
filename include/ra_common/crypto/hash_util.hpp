#pragma once

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "ra_common/crypto/hash_algorithm.hpp"
#include "ra_common/crypto/pbkdf2.hpp"
#include "ra_common/crypto/sha.hpp"
#include "ra_common/encoding.hpp"
#include "ra_common/exception.hpp"
#include "ra_common/util/random_util.hpp"

namespace ra::common::crypto {

namespace hash_util {

inline constexpr int kPbkdf2Iterations = 1000;
inline constexpr size_t kPbkdf2KeyLen = 64;
inline constexpr size_t kSaltLen = 16;

inline std::vector<uint8_t> Salt() { return RandomBytesOf(kSaltLen); }

inline std::vector<uint8_t> Digest(const std::vector<uint8_t>& data, HashAlgorithm algorithm) {
    switch (algorithm) {
        case HashAlgorithm::Sha1: return detail::Sha1(data.data(), data.size());
        case HashAlgorithm::Sha256: return detail::Sha256(data.data(), data.size());
        case HashAlgorithm::Sha512: return detail::Sha512(data.data(), data.size());
        default: throw RaException::Crypto("PBKDF2 is not a plain digest");
    }
}

/// Uppercase hex of `data`, grouped into blocks of four chars separated by ':'.
inline std::string ToHex(const std::vector<uint8_t>& data) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string hex;
    hex.reserve(data.size() * 2);
    for (uint8_t b : data) {
        hex += kHex[b >> 4];
        hex += kHex[b & 0xf];
    }
    std::string grouped;
    for (size_t i = 0; i < hex.size(); i += 4) grouped += (i == 0 ? "" : ":") + hex.substr(i, 4);
    return grouped;
}

inline std::vector<uint8_t> FromHex(std::string text) {
    text.erase(std::remove(text.begin(), text.end(), ':'), text.end());
    std::vector<uint8_t> out(text.size() / 2);
    for (size_t i = 0; i < out.size(); i++) out[i] = static_cast<uint8_t>(std::stoi(text.substr(i * 2, 2), nullptr, 16));
    return out;
}

inline std::string GenerateFingerprint(const std::vector<uint8_t>& data, HashAlgorithm algorithm) {
    return ToHex(Digest(data, algorithm));
}

inline std::vector<uint8_t> Concat(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    std::vector<uint8_t> out(a);
    out.insert(out.end(), b.begin(), b.end());
    return out;
}

inline bool ConstantTimeEquals(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) return false;
    uint8_t diff = 0;
    for (size_t i = 0; i < a.size(); i++) diff |= static_cast<uint8_t>(a[i] ^ b[i]);
    return diff == 0;
}

inline std::vector<std::string> SplitOn(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) parts.push_back(item);
    return parts;
}

inline std::string GeneratePasswordHashWithSalt(const std::string& password, const std::vector<uint8_t>& salt) {
    auto derived = detail::Pbkdf2HmacSha1(password, salt, kPbkdf2Iterations, kPbkdf2KeyLen);
    return std::to_string(kPbkdf2Iterations) + "_" + Base64Encode(salt) + "_" + Base64Encode(derived);
}

inline std::string GeneratePasswordHash(const std::string& password) {
    return GeneratePasswordHashWithSalt(password, Salt());
}

inline bool VerifyPasswordHash(const std::string& password, const std::string& hash_to_verify) {
    auto parts = SplitOn(hash_to_verify, '_');
    if (parts.size() != 3) return false;
    int iterations;
    try {
        size_t consumed;
        iterations = std::stoi(parts[0], &consumed);
        if (consumed != parts[0].size()) return false;
    } catch (...) {
        return false;
    }
    std::vector<uint8_t> salt, expected;
    try {
        salt = Base64Decode(parts[1]);
        expected = Base64Decode(parts[2]);
    } catch (...) {
        return false;
    }
    auto actual = detail::Pbkdf2HmacSha1(password, salt, iterations, expected.size());
    return ConstantTimeEquals(actual, expected);
}

inline std::string GenerateHash(const std::vector<uint8_t>& content, HashAlgorithm algorithm) {
    if (algorithm == HashAlgorithm::Pbkdf2HmacSha1) {
        return GeneratePasswordHash(std::string(content.begin(), content.end()));
    }
    auto salt = Salt();
    auto h = Digest(Concat(salt, content), algorithm);
    return Base64Encode(h) + "_" + Base64Encode(salt);
}

inline bool VerifyHash(const std::vector<uint8_t>& content, const std::string& hash_to_verify,
                        HashAlgorithm algorithm) {
    if (algorithm == HashAlgorithm::Pbkdf2HmacSha1) {
        return VerifyPasswordHash(std::string(content.begin(), content.end()), hash_to_verify);
    }
    auto parts = SplitOn(hash_to_verify, '_');
    if (parts.size() != 2) throw RaException::Invalid("malformed hash");
    auto expected = Base64Decode(parts[0]);
    auto salt = Base64Decode(parts[1]);
    return ConstantTimeEquals(Digest(Concat(salt, content), algorithm), expected);
}

}  // namespace hash_util

}  // namespace ra::common::crypto
