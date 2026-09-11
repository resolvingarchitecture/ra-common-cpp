#include <limits>

#include "doctest/doctest.h"
#include "ra_common/util/bytes_util.hpp"
#include "ra_common/util/nonce.hpp"
#include "ra_common/util/string_util.hpp"
#include "ra_common/util/unique_id.hpp"
#include "ra_common/util/version_comparator.hpp"

using namespace ra::common;

TEST_CASE("util: bytes, strings, version, nonce, unique id") {
    for (int32_t v : {0, 1, -1, 42, std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max()}) {
        auto packed = UnpackBigEndian(v);
        CHECK(PackBigEndian(packed.data()) == v);
    }

    CHECK(Capitalize("one two three") == "One Two Three");
    CHECK(VersionCompare("1.8", "1.11") == -1);
    CHECK(VersionCompare("2.0", "2.0.0") == -1);
    CHECK(VersionCompare("8ea", "8") == 0);
    CHECK(VersionCompare("8-ea", "8") == 1);

    Nonce nonce;
    CHECK(nonce.ContinueOn("1"));
    CHECK_FALSE(nonce.ContinueOn("1"));

    auto uid = UniqueId::Random();
    CHECK(uid.ToBase64().size() == 44);
    CHECK(UniqueId::FromBase64(uid.ToBase64()).Compare(uid) == 0);
}
