#pragma once

#include <optional>
#include <string>

#include "nlohmann/json.hpp"

namespace ra::common::identity {

/// A detached signature over some value. This package does not sign or
/// verify; Signature is a metadata record carried inside PublicKey. Equality
/// is on signed_by_address.
class Signature {
public:
    std::optional<std::string> value_signed;
    std::optional<std::string> algorithm;
    std::optional<std::string> signed_date;  // ISO-8601, stored as text (no calendar arithmetic needed here)
    std::optional<std::string> signed_by_username;
    std::optional<std::string> signed_by_fingerprint;
    std::optional<std::string> signed_by_address;

    bool Equals(const Signature& other) const {
        return signed_by_address.has_value() && signed_by_address == other.signed_by_address;
    }

    nlohmann::json ToJson() const {
        nlohmann::json obj = nlohmann::json::object();
        if (value_signed) obj["value_signed"] = *value_signed;
        if (algorithm) obj["algorithm"] = *algorithm;
        if (signed_date) obj["signed_date"] = *signed_date;
        if (signed_by_username) obj["signed_by_username"] = *signed_by_username;
        if (signed_by_fingerprint) obj["signed_by_fingerprint"] = *signed_by_fingerprint;
        if (signed_by_address) obj["signed_by_address"] = *signed_by_address;
        return obj;
    }

    static Signature FromJson(const nlohmann::json& data) {
        Signature s;
        if (data.contains("value_signed")) s.value_signed = data.at("value_signed").get<std::string>();
        if (data.contains("algorithm")) s.algorithm = data.at("algorithm").get<std::string>();
        if (data.contains("signed_date")) s.signed_date = data.at("signed_date").get<std::string>();
        if (data.contains("signed_by_username")) s.signed_by_username = data.at("signed_by_username").get<std::string>();
        if (data.contains("signed_by_fingerprint")) {
            s.signed_by_fingerprint = data.at("signed_by_fingerprint").get<std::string>();
        }
        if (data.contains("signed_by_address")) s.signed_by_address = data.at("signed_by_address").get<std::string>();
        return s;
    }
};

}  // namespace ra::common::identity
