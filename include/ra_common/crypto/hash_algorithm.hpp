#pragma once

#include <string>

#include "ra_common/exception.hpp"

namespace ra::common::crypto {

enum class HashAlgorithm {
    Sha1,
    Sha256,
    Sha512,
    Pbkdf2HmacSha1,
};

inline std::string ToString(HashAlgorithm a) {
    switch (a) {
        case HashAlgorithm::Sha1: return "Sha1";
        case HashAlgorithm::Sha256: return "Sha256";
        case HashAlgorithm::Sha512: return "Sha512";
        case HashAlgorithm::Pbkdf2HmacSha1: return "Pbkdf2HmacSha1";
    }
    return "Sha256";
}

inline std::string JcaName(HashAlgorithm a) {
    switch (a) {
        case HashAlgorithm::Sha1: return "SHA-1";
        case HashAlgorithm::Sha256: return "SHA-256";
        case HashAlgorithm::Sha512: return "SHA-512";
        case HashAlgorithm::Pbkdf2HmacSha1: return "PBKDF2WithHmacSHA1";
    }
    return "SHA-256";
}

inline HashAlgorithm ParseHashAlgorithm(const std::string& text) {
    if (text == "SHA-1" || text == "SHA1" || text == "Sha1") return HashAlgorithm::Sha1;
    if (text == "SHA-256" || text == "SHA256" || text == "Sha256") return HashAlgorithm::Sha256;
    if (text == "SHA-512" || text == "SHA512" || text == "Sha512") return HashAlgorithm::Sha512;
    if (text == "PBKDF2WithHmacSHA1" || text == "Pbkdf2HmacSha1") return HashAlgorithm::Pbkdf2HmacSha1;
    throw RaException::Invalid("unknown hash algorithm: " + text);
}

}  // namespace ra::common::crypto
