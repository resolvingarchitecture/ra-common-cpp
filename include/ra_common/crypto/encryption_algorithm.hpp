#pragma once

#include <string>

namespace ra::common::crypto {

enum class EncryptionAlgorithm {
    Cast5,
    Aes256,
    Aes512,
};

inline std::string ToString(EncryptionAlgorithm a) {
    switch (a) {
        case EncryptionAlgorithm::Cast5: return "Cast5";
        case EncryptionAlgorithm::Aes256: return "Aes256";
        case EncryptionAlgorithm::Aes512: return "Aes512";
    }
    return "Aes256";
}

inline EncryptionAlgorithm EncryptionAlgorithmFromString(const std::string& s) {
    if (s == "Cast5") return EncryptionAlgorithm::Cast5;
    if (s == "Aes512") return EncryptionAlgorithm::Aes512;
    return EncryptionAlgorithm::Aes256;
}

inline std::string Name(EncryptionAlgorithm a) {
    switch (a) {
        case EncryptionAlgorithm::Cast5: return "CAST-5";
        case EncryptionAlgorithm::Aes256: return "AES-256";
        case EncryptionAlgorithm::Aes512: return "AES-512";
    }
    return "AES-256";
}

}  // namespace ra::common::crypto
