#pragma once

// HMAC-SHA1 and PBKDF2-HMAC-SHA1, built on sha.hpp. Needed for password
// hashing (HashUtil::GeneratePasswordHash), matching what every other port
// gets from its runtime (node:crypto's pbkdf2Sync, .NET's Rfc2898DeriveBytes,
// Python's hashlib.pbkdf2_hmac).

#include <cstdint>
#include <vector>

#include "ra_common/crypto/sha.hpp"

namespace ra::common::detail {

inline constexpr size_t kSha1BlockSize = 64;
inline constexpr size_t kSha1DigestSize = 20;

inline std::vector<uint8_t> HmacSha1(const std::vector<uint8_t>& key_in, const uint8_t* data, size_t len) {
    std::vector<uint8_t> key = key_in;
    if (key.size() > kSha1BlockSize) key = Sha1(key.data(), key.size());
    key.resize(kSha1BlockSize, 0x00);

    std::vector<uint8_t> ipad(kSha1BlockSize), opad(kSha1BlockSize);
    for (size_t i = 0; i < kSha1BlockSize; i++) {
        ipad[i] = key[i] ^ 0x36;
        opad[i] = key[i] ^ 0x5c;
    }

    std::vector<uint8_t> inner_input(ipad);
    inner_input.insert(inner_input.end(), data, data + len);
    auto inner = Sha1(inner_input.data(), inner_input.size());

    std::vector<uint8_t> outer_input(opad);
    outer_input.insert(outer_input.end(), inner.begin(), inner.end());
    return Sha1(outer_input.data(), outer_input.size());
}

/// PBKDF2 with HMAC-SHA1, matching the parameters `HashUtil` uses elsewhere
/// (iteration count and key length are caller-supplied).
inline std::vector<uint8_t> Pbkdf2HmacSha1(const std::string& password, const std::vector<uint8_t>& salt,
                                            int iterations, size_t key_len) {
    std::vector<uint8_t> pw(password.begin(), password.end());
    std::vector<uint8_t> out;
    out.reserve(key_len);

    uint32_t block_index = 1;
    while (out.size() < key_len) {
        std::vector<uint8_t> salt_block = salt;
        salt_block.push_back(static_cast<uint8_t>(block_index >> 24));
        salt_block.push_back(static_cast<uint8_t>(block_index >> 16));
        salt_block.push_back(static_cast<uint8_t>(block_index >> 8));
        salt_block.push_back(static_cast<uint8_t>(block_index));

        auto u = HmacSha1(pw, salt_block.data(), salt_block.size());
        std::vector<uint8_t> t = u;
        for (int i = 1; i < iterations; i++) {
            u = HmacSha1(pw, u.data(), u.size());
            for (size_t j = 0; j < t.size(); j++) t[j] ^= u[j];
        }
        out.insert(out.end(), t.begin(), t.end());
        block_index++;
    }
    out.resize(key_len);
    return out;
}

}  // namespace ra::common::detail
