#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <sys/random.h>
#include <vector>

#include "ra_common/exception.hpp"

namespace ra::common {

/// Fills `out` with cryptographically-strong random bytes via getentropy(2).
/// POSIX-only (Linux glibc >=2.25/macOS >=10.12) - there is no portable
/// CSPRNG in the C++ standard library, unlike the other ports which use
/// their runtime's built-in one (node:crypto / System.Security.Cryptography
/// / Python's os.urandom). A Windows backend (BCryptGenRandom) is not
/// implemented; see TODO.md.
///
/// This used to go through a process-wide /dev/urandom FILE* guarded by a
/// mutex, which serialized every concurrent caller on one lock - found via
/// seda-bus-compare's cross-language throughput benchmark to cap seda-bus-cpp's
/// parallel scaling (independent-channel throughput) far below Rust/Go's,
/// even after fixing the earlier reopen-per-call bug. getentropy() is a
/// direct syscall with no shared file descriptor or handle, so no lock is
/// needed at all - each thread just calls it independently. It caps out at
/// 256 bytes/call, which is not a constraint here (largest request is 16
/// bytes, for envelope ids).
inline void SecureRandomBytes(uint8_t* out, size_t count) {
    while (count > 0) {
        size_t chunk = count < 256 ? count : 256;
        if (getentropy(out, chunk) != 0) throw RaException::Crypto("getentropy failed");
        out += chunk;
        count -= chunk;
    }
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
