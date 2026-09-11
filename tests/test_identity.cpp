#include "doctest/doctest.h"
#include "ra_common/identity/did.hpp"
#include "ra_common/identity/public_key.hpp"
#include "ra_common/identity/signature.hpp"

using namespace ra::common::identity;
using namespace ra::common::crypto;

TEST_CASE("identity did + signature + public key") {
    auto d = Did::WithUsername("alice");
    d.passphrase = "secret";
    d.authenticated = true;
    d.ClearSensitive();
    CHECK(d.username == "");
    CHECK(d.status == DidStatus::Private);

    auto bob = Did::WithUsername("bob");
    bob.public_key = PublicKey::FromAddress("addr");
    bob.passphrase_hash = Hash("deadbeef", HashAlgorithm::Sha256);
    auto back = Did::FromJson(bob.ToJson());
    CHECK(back.username == "bob");
    CHECK(back.public_key.address_value == "addr");
    CHECK(back.passphrase_hash->hash_value() == "deadbeef");

    Signature a;
    Signature b;
    CHECK_FALSE(a.Equals(b));
    a.signed_by_address = "x";
    b.signed_by_address = "x";
    CHECK(a.Equals(b));

    auto pk = PublicKey::FromAddress("B32");
    Signature sig;
    sig.signed_by_address = "signer";
    pk.AddSignedAttribute("email", sig);
    CHECK(pk.signed_attributes["email"].size() == 1);
    pk.RemoveSignature("email", "signer");
    CHECK(pk.signed_attributes["email"].size() == 0);
}
