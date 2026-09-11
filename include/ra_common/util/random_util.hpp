#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

#include "ra_common/exception.hpp"

namespace ra::common {

/// Fills `out` with cryptographically-strong random bytes from /dev/urandom.
/// POSIX-only (Linux/macOS) - there is no portable CSPRNG in the C++ standard
/// library, unlike the other ports which use their runtime's built-in one
/// (node:crypto / System.Security.Cryptography / Python's os.urandom).
/// A Windows backend (BCryptGenRandom) is not implemented; see TODO.md.
///
/// The device file is opened once per process and kept open (guarded by a
/// mutex for concurrent callers), not reopened on every call - reopening was
/// the original implementation, and it made every ID/route-id generation a
/// fopen+fread+fclose cycle. Found via seda-bus-compare's cross-language
/// throughput benchmark: it made seda-bus-cpp's numbers 20-30x slower than
/// seda-bus-rust's for identical work, entirely due to this, not anything
/// about C++ or the bus itself - every other port's random source is a
/// single syscall (or, for Python's non-cryptographic RNG, no syscall at
/// all), never a re-opened file handle.
inline void SecureRandomBytes(uint8_t* out, size_t count) {
    static std::mutex urandom_mutex;
    static std::FILE* urandom = [] {
        std::FILE* f = std::fopen("/dev/urandom", "rb");
        if (f == nullptr) throw RaException::Crypto("could not open /dev/urandom");
        return f;
    }();
    std::lock_guard<std::mutex> lock(urandom_mutex);
    size_t read = std::fread(out, 1, count, urandom);
    if (read != count) throw RaException::Crypto("short read from /dev/urandom");
}

inline std::vector<uint8_t> RandomBytesOf(size_t count) {
    std::vector<uint8_t> out(count);
    if (count > 0) SecureRandomBytes(out.data(), count);
    return out;
}

inline std::string RandomAlphanumeric(size_t length) {
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    constexpr size_t kAlphabetLen = sizeof(kAlphabet) - 1;
    auto bytes = RandomBytesOf(length);
    std::string out(length, '\0');
    for (size_t i = 0; i < length; i++) out[i] = kAlphabet[bytes[i] % kAlphabetLen];
    return out;
}

/// A random signed 32-bit int in [lower, upper).
inline int32_t NextIntIn(int32_t lower, int32_t upper) {
    uint8_t buf[4];
    SecureRandomBytes(buf, 4);
    uint32_t raw;
    std::memcpy(&raw, buf, 4);
    auto span = static_cast<uint32_t>(upper - lower);
    return lower + static_cast<int32_t>(raw % span);
}

/// A random 64-bit correlation id (used for routing-slip route ids).
inline int64_t NextLong() {
    uint8_t buf[8];
    SecureRandomBytes(buf, 8);
    int64_t v;
    std::memcpy(&v, buf, 8);
    return v;
}

}  // namespace ra::common
