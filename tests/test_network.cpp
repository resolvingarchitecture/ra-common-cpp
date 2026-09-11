#include "doctest/doctest.h"
#include "ra_common/network.hpp"

using namespace ra::common;

TEST_CASE("network peer equality") {
    NetworkPeer a(Network::Tor);
    NetworkPeer b(Network::Tor);
    CHECK_FALSE(a.Equals(b));

    for (auto* p : {&a, &b}) {
        p->did.public_key.address_value = "addr";
        p->did.public_key.fingerprint_value = "fp";
    }
    CHECK(a.Equals(b));
    CHECK(NetworkPeer::FromJson(a.ToJson()).network_value == Network::Tor);
}
