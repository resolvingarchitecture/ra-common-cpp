#pragma once

#include <stdexcept>
#include <string>

namespace ra::common {

/// Package-wide error type. Replaces the family of checked exception classes
/// in ra-common-java (ServiceNotFoundException, FileCreationFailedException, ...).
enum class RaErrorKind {
    Decode,
    Crypto,
    Invalid,
    ServiceNotFound,
    ServiceNotAccessible,
    ServiceNotSupported,
    ServiceAlreadyRegistered,
    FileCreationFailed,
    Io,
};

inline const char* ToString(RaErrorKind kind) {
    switch (kind) {
        case RaErrorKind::Decode: return "decode";
        case RaErrorKind::Crypto: return "crypto";
        case RaErrorKind::Invalid: return "invalid";
        case RaErrorKind::ServiceNotFound: return "service-not-found";
        case RaErrorKind::ServiceNotAccessible: return "service-not-accessible";
        case RaErrorKind::ServiceNotSupported: return "service-not-supported";
        case RaErrorKind::ServiceAlreadyRegistered: return "service-already-registered";
        case RaErrorKind::FileCreationFailed: return "file-creation-failed";
        case RaErrorKind::Io: return "io";
    }
    return "unknown";
}

class RaException : public std::runtime_error {
public:
    RaException(RaErrorKind kind, const std::string& message)
        : std::runtime_error(std::string(ToString(kind)) + ": " + message), kind_(kind) {}

    RaErrorKind kind() const { return kind_; }

    static RaException Decode(const std::string& message) { return {RaErrorKind::Decode, message}; }
    static RaException Crypto(const std::string& message) { return {RaErrorKind::Crypto, message}; }
    static RaException Invalid(const std::string& message) { return {RaErrorKind::Invalid, message}; }

private:
    RaErrorKind kind_;
};

}  // namespace ra::common
