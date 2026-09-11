#pragma once

#include <optional>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/crypto/hash.hpp"
#include "ra_common/crypto/hash_algorithm.hpp"
#include "ra_common/identity/pii_clearable.hpp"
#include "ra_common/identity/public_key.hpp"

namespace ra::common::identity {

enum class DidStatus {
    Inactive,
    Active,
    Suspended,
    Private,
};

inline std::string ToString(DidStatus s) {
    switch (s) {
        case DidStatus::Inactive: return "Inactive";
        case DidStatus::Active: return "Active";
        case DidStatus::Suspended: return "Suspended";
        case DidStatus::Private: return "Private";
    }
    return "Inactive";
}

inline DidStatus DidStatusFromString(const std::string& s) {
    if (s == "Active") return DidStatus::Active;
    if (s == "Suspended") return DidStatus::Suspended;
    if (s == "Private") return DidStatus::Private;
    return DidStatus::Inactive;
}

enum class DidType {
    Contact,
    Identity,
    Node,
};

inline std::string ToString(DidType t) {
    switch (t) {
        case DidType::Contact: return "Contact";
        case DidType::Identity: return "Identity";
        case DidType::Node: return "Node";
    }
    return "Identity";
}

inline DidType DidTypeFromString(const std::string& s) {
    if (s == "Contact") return DidType::Contact;
    if (s == "Node") return DidType::Node;
    return DidType::Identity;
}

/// A decentralized identity: a username, an optional passphrase (+ its hash),
/// and a PublicKey. Deliberately does not follow the W3C DID spec - RA models
/// each key as its own identity rather than grouping keys.
class Did : public PiiClearable {
public:
    std::string username = "Anon";
    std::optional<std::string> passphrase;
    std::optional<std::string> passphrase2;
    std::optional<crypto::Hash> passphrase_hash;
    crypto::HashAlgorithm passphrase_hash_algorithm = crypto::HashAlgorithm::Pbkdf2HmacSha1;
    std::string description;
    DidStatus status = DidStatus::Inactive;
    DidType did_type = DidType::Identity;
    bool verified = false;
    bool authenticated = false;
    PublicKey public_key;

    static Did WithUsername(const std::string& username) {
        Did d;
        d.username = username;
        return d;
    }

    crypto::HashAlgorithm EffectivePassphraseHashAlgorithm() const {
        return passphrase_hash ? passphrase_hash->algorithm() : passphrase_hash_algorithm;
    }

    void ClearSensitive() override {
        username = "";
        passphrase.reset();
        passphrase2.reset();
        description = "";
        status = DidStatus::Private;
        verified = false;
        authenticated = false;
    }

    nlohmann::json ToJson() const {
        nlohmann::json obj;
        obj["username"] = username;
        if (passphrase) obj["passphrase"] = *passphrase;
        if (passphrase2) obj["passphrase2"] = *passphrase2;
        if (passphrase_hash) obj["passphrase_hash"] = passphrase_hash->ToJson();
        obj["passphrase_hash_algorithm"] = ra::common::crypto::ToString(passphrase_hash_algorithm);
        obj["description"] = description;
        obj["status"] = ToString(status);
        obj["did_type"] = ToString(did_type);
        obj["verified"] = verified;
        obj["authenticated"] = authenticated;
        obj["public_key"] = public_key.ToJson();
        return obj;
    }

    static Did FromJson(const nlohmann::json& data) {
        Did d;
        d.username = data.value("username", std::string("Anon"));
        if (data.contains("passphrase")) d.passphrase = data.at("passphrase").get<std::string>();
        if (data.contains("passphrase2")) d.passphrase2 = data.at("passphrase2").get<std::string>();
        if (data.contains("passphrase_hash")) d.passphrase_hash = crypto::Hash::FromJson(data.at("passphrase_hash"));
        d.passphrase_hash_algorithm = data.contains("passphrase_hash_algorithm")
                                           ? crypto::ParseHashAlgorithm(data.at("passphrase_hash_algorithm").get<std::string>())
                                           : crypto::HashAlgorithm::Pbkdf2HmacSha1;
        d.description = data.value("description", std::string(""));
        d.status = data.contains("status") ? DidStatusFromString(data.at("status").get<std::string>()) : DidStatus::Inactive;
        d.did_type = data.contains("did_type") ? DidTypeFromString(data.at("did_type").get<std::string>()) : DidType::Identity;
        d.verified = data.value("verified", false);
        d.authenticated = data.value("authenticated", false);
        if (data.contains("public_key")) d.public_key = PublicKey::FromJson(data.at("public_key"));
        return d;
    }
};

}  // namespace ra::common::identity
