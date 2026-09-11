#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ra_common/encoding.hpp"
#include "ra_common/exception.hpp"
#include "ra_common/util/random_util.hpp"

namespace ra::common {

/// A fixed 32-byte identifier, rendered as standard padded base64 (44 chars).
class UniqueId {
public:
    static constexpr size_t kLength = 32;

    explicit UniqueId(std::vector<uint8_t> bytes) : bytes_(std::move(bytes)) {
        if (bytes_.size() != kLength) {
            throw RaException::Invalid("UniqueId must be " + std::to_string(kLength) + " bytes, got " +
                                        std::to_string(bytes_.size()));
        }
    }

    static UniqueId Random() { return UniqueId(RandomBytesOf(kLength)); }

    static UniqueId FromSlice(const std::vector<uint8_t>& src, size_t offset = 0) {
        if (src.size() < offset + kLength) throw RaException::Invalid("not enough bytes for UniqueId");
        return UniqueId(std::vector<uint8_t>(src.begin() + static_cast<long>(offset),
                                              src.begin() + static_cast<long>(offset + kLength)));
    }

    static UniqueId FromBase64(const std::string& text) {
        auto raw = Base64Decode(text);
        if (raw.size() != kLength) throw RaException::Decode("UniqueId must be 32 bytes");
        return UniqueId(std::move(raw));
    }

    std::string ToBase64() const { return Base64Encode(bytes_); }

    const std::vector<uint8_t>& bytes() const { return bytes_; }

    int Compare(const UniqueId& other) const {
        for (size_t i = 0; i < kLength; i++) {
            if (bytes_[i] != other.bytes_[i]) return static_cast<int>(bytes_[i]) - static_cast<int>(other.bytes_[i]);
        }
        return 0;
    }

private:
    std::vector<uint8_t> bytes_;
};

}  // namespace ra::common
