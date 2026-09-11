#include "doctest/doctest.h"
#include "ra_common/encoding.hpp"

using namespace ra::common;

TEST_CASE("encoding round trips") {
    std::string text = "resolving architecture";
    std::vector<uint8_t> data(text.begin(), text.end());
    CHECK(Base32Decode(Base32Encode(data)) == data);

    std::string hw = "Hello World!";
    std::vector<uint8_t> hw_bytes(hw.begin(), hw.end());
    CHECK(Base58Encode(hw_bytes) == "2NEpo7TZRRrLZSi2U");
    CHECK(Base58Decode("2NEpo7TZRRrLZSi2U") == hw_bytes);

    CHECK(Base64Decode(Base64Encode(data)) == data);
}
