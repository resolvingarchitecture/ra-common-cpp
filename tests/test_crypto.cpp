#include "doctest/doctest.h"
#include "ra_common/crypto/hash.hpp"
#include "ra_common/crypto/hash_cash.hpp"
#include "ra_common/crypto/hash_util.hpp"
#include "ra_common/crypto/multihash.hpp"

using namespace ra::common;
using namespace ra::common::crypto;

TEST_CASE("crypto hash + password + hashcash + multihash") {
    CHECK(Hash("abc", HashAlgorithm::Sha256).Equals(Hash("abc", HashAlgorithm::Sha1)));

    std::vector<uint8_t> alice = {'A', 'l', 'i', 'c', 'e'};
    auto h = hash_util::GenerateHash(alice, HashAlgorithm::Sha256);
    CHECK(hash_util::VerifyHash(alice, h, HashAlgorithm::Sha256));
    std::vector<uint8_t> bob = {'B', 'o', 'b'};
    CHECK_FALSE(hash_util::VerifyHash(bob, h, HashAlgorithm::Sha256));

    auto pw = hash_util::GeneratePasswordHash("hunter2");
    CHECK(pw.rfind("1000_", 0) == 0);
    CHECK(hash_util::VerifyPasswordHash("hunter2", pw));
    CHECK_FALSE(hash_util::VerifyPasswordHash("hunter3", pw));

    Multihash m(MultihashType::Sha2_256, std::vector<uint8_t>(32, 0xab));
    CHECK(Multihash::FromBytes(m.ToBytes()).Equals(m));
    CHECK(Multihash::FromHexString(m.ToHexString()).Equals(m));
    CHECK(Multihash::FromBase58(m.ToBase58()).Equals(m));

    auto hc = HashCash::Mint("brian@resolvingarchitecture.io", 10);
    CHECK(hc.ComputedBits() >= 10);
    CHECK(hc.IsValidFor("brian@resolvingarchitecture.io", 10));
    CHECK_FALSE(hc.IsValidFor("nope", 10));
    CHECK(HashCash::Parse(hc.token()).resource() == hc.resource());

    CHECK(ra::common::detail::LeadingZeroBits({0, 0, 0x0f}) == 20);
    CHECK(ra::common::detail::LeadingZeroBits({0xff}) == 0);
}
