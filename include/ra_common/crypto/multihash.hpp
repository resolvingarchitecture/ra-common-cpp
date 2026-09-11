#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "ra_common/encoding.hpp"
#include "ra_common/exception.hpp"

namespace ra::common::crypto {

enum class MultihashType {
    Sha1,
    Sha2_256,
    Sha2_512,
    Sha3,
    Blake2b,
    Blake2s,
};

inline std::string ToString(MultihashType t) {
    switch (t) {
        case MultihashType::Sha1: return "Sha1";
        case MultihashType::Sha2_256: return "Sha2_256";
        case MultihashType::Sha2_512: return "Sha2_512";
        case MultihashType::Sha3: return "Sha3";
        case MultihashType::Blake2b: return "Blake2b";
        case MultihashType::Blake2s: return "Blake2s";
    }
    return "Sha2_256";
}

inline MultihashType MultihashTypeFromString(const std::string& s) {
    if (s == "Sha1") return MultihashType::Sha1;
    if (s == "Sha2_256") return MultihashType::Sha2_256;
    if (s == "Sha2_512") return MultihashType::Sha2_512;
    if (s == "Sha3") return MultihashType::Sha3;
    if (s == "Blake2b") return MultihashType::Blake2b;
    if (s == "Blake2s") return MultihashType::Blake2s;
    throw RaException::Invalid("unknown multihash type: " + s);
}

inline int MultihashCode(MultihashType t) {
    switch (t) {
        case MultihashType::Sha1: return 0x11;
        case MultihashType::Sha2_256: return 0x12;
        case MultihashType::Sha2_512: return 0x13;
        case MultihashType::Sha3: return 0x14;
        case MultihashType::Blake2b: return 0x40;
        case MultihashType::Blake2s: return 0x41;
    }
    return 0x12;
}

inline size_t MultihashLength(MultihashType t) {
    switch (t) {
        case MultihashType::Sha1: return 20;
        case MultihashType::Sha2_256: return 32;
        case MultihashType::Sha2_512: return 64;
        case MultihashType::Sha3: return 64;
        case MultihashType::Blake2b: return 64;
        case MultihashType::Blake2s: return 32;
    }
    return 32;
}

inline MultihashType MultihashTypeFromCode(int code) {
    for (auto t : {MultihashType::Sha1, MultihashType::Sha2_256, MultihashType::Sha2_512, MultihashType::Sha3,
                   MultihashType::Blake2b, MultihashType::Blake2s}) {
        if (MultihashCode(t) == code) return t;
    }
    std::ostringstream os;
    os << "unknown multihash type: 0x" << std::hex << code;
    throw RaException::Invalid(os.str());
}

class Multihash {
public:
    Multihash(MultihashType kind, std::vector<uint8_t> digest) : kind_(kind), digest_(std::move(digest)) {
        if (digest_.size() > 127) throw RaException::Invalid("unsupported hash size: " + std::to_string(digest_.size()));
        if (digest_.size() != MultihashLength(kind)) {
            throw RaException::Invalid("incorrect hash length: " + std::to_string(digest_.size()) +
                                        " != " + std::to_string(MultihashLength(kind)));
        }
    }

    MultihashType kind() const { return kind_; }
    const std::vector<uint8_t>& digest() const { return digest_; }

    std::vector<uint8_t> ToBytes() const {
        std::vector<uint8_t> out;
        out.push_back(static_cast<uint8_t>(MultihashCode(kind_)));
        out.push_back(static_cast<uint8_t>(digest_.size()));
        out.insert(out.end(), digest_.begin(), digest_.end());
        return out;
    }

    static Multihash FromBytes(const std::vector<uint8_t>& data) {
        if (data.size() < 2) throw RaException::Invalid("multihash too short");
        auto kind = MultihashTypeFromCode(data[0]);
        size_t length = data[1];
        if (data.size() < 2 + length) throw RaException::Invalid("multihash truncated");
        return Multihash(kind, std::vector<uint8_t>(data.begin() + 2, data.begin() + 2 + static_cast<long>(length)));
    }

    std::string ToHexString() const {
        static constexpr char kHex[] = "0123456789abcdef";
        auto bytes = ToBytes();
        std::string out;
        out.reserve(bytes.size() * 2);
        for (uint8_t b : bytes) {
            out += kHex[b >> 4];
            out += kHex[b & 0xf];
        }
        return out;
    }

    static Multihash FromHexString(const std::string& text) {
        std::vector<uint8_t> bytes(text.size() / 2);
        for (size_t i = 0; i < bytes.size(); i++) {
            bytes[i] = static_cast<uint8_t>(std::stoi(text.substr(i * 2, 2), nullptr, 16));
        }
        return FromBytes(bytes);
    }

    std::string ToBase58() const { return Base58Encode(ToBytes()); }

    static Multihash FromBase58(const std::string& text) { return FromBytes(Base58Decode(text)); }

    bool Equals(const Multihash& other) const { return ToHexString() == other.ToHexString(); }

    nlohmann::json ToJson() const {
        return {{"type", ToString(kind_)}, {"hash", digest_}};
    }

    static Multihash FromJson(const nlohmann::json& data) {
        return Multihash(MultihashTypeFromString(data.at("type").get<std::string>()),
                          data.at("hash").get<std::vector<uint8_t>>());
    }

private:
    MultihashType kind_;
    std::vector<uint8_t> digest_;
};

}  // namespace ra::common::crypto
