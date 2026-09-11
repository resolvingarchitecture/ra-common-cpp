#pragma once

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "ra_common/crypto/addressable.hpp"
#include "ra_common/identity/signature.hpp"

namespace ra::common::identity {

class PublicKey : public crypto::Addressable {
public:
    std::optional<std::string> alias;
    std::optional<std::string> fingerprint_value;
    std::optional<std::string> address_value;
    std::optional<std::string> key_type;
    bool is_identity_key = false;
    bool is_encryption_key = false;
    bool is_base64_encoded = false;
    bool is_base58_encoded = false;
    bool is_pem = false;
    bool is_hex = false;
    nlohmann::json attributes = nlohmann::json::object();
    std::map<std::string, std::vector<Signature>> signed_attributes;

    std::optional<std::string> fingerprint() const override { return fingerprint_value; }
    std::optional<std::string> address() const override { return address_value; }

    static PublicKey FromAddress(const std::string& address) {
        PublicKey pk;
        pk.address_value = address;
        return pk;
    }

    void AddAttribute(const std::string& name, nlohmann::json value) { attributes[name] = std::move(value); }

    nlohmann::json Attribute(const std::string& name) const {
        return attributes.contains(name) ? attributes.at(name) : nlohmann::json();
    }

    void AddSignedAttribute(const std::string& name, const Signature& signature) {
        signed_attributes[name].push_back(signature);
    }

    void RemoveSignature(const std::string& name, const std::string& signed_by_address) {
        auto it = signed_attributes.find(name);
        if (it == signed_attributes.end()) return;
        auto& sigs = it->second;
        sigs.erase(std::remove_if(sigs.begin(), sigs.end(),
                                   [&](const Signature& s) { return s.signed_by_address == signed_by_address; }),
                   sigs.end());
    }

    nlohmann::json ToJson() const {
        nlohmann::json obj = nlohmann::json::object();
        if (alias) obj["alias"] = *alias;
        if (fingerprint_value) obj["fingerprint"] = *fingerprint_value;
        if (address_value) obj["address"] = *address_value;
        if (key_type) obj["type"] = *key_type;
        if (is_identity_key) obj["is_identity_key"] = true;
        if (is_encryption_key) obj["is_encryption_key"] = true;
        if (is_base64_encoded) obj["is_base64_encoded"] = true;
        if (is_base58_encoded) obj["is_base58_encoded"] = true;
        if (is_pem) obj["is_pem"] = true;
        if (is_hex) obj["is_hex"] = true;
        if (!attributes.empty()) obj["attributes"] = attributes;
        if (!signed_attributes.empty()) {
            nlohmann::json signed_json = nlohmann::json::object();
            for (const auto& [key, sigs] : signed_attributes) {
                nlohmann::json arr = nlohmann::json::array();
                for (const auto& s : sigs) arr.push_back(s.ToJson());
                signed_json[key] = arr;
            }
            obj["signed_attributes"] = signed_json;
        }
        return obj;
    }

    static PublicKey FromJson(const nlohmann::json& data) {
        PublicKey pk;
        if (data.contains("alias")) pk.alias = data.at("alias").get<std::string>();
        if (data.contains("fingerprint")) pk.fingerprint_value = data.at("fingerprint").get<std::string>();
        if (data.contains("address")) pk.address_value = data.at("address").get<std::string>();
        if (data.contains("type")) pk.key_type = data.at("type").get<std::string>();
        pk.is_identity_key = data.value("is_identity_key", false);
        pk.is_encryption_key = data.value("is_encryption_key", false);
        pk.is_base64_encoded = data.value("is_base64_encoded", false);
        pk.is_base58_encoded = data.value("is_base58_encoded", false);
        pk.is_pem = data.value("is_pem", false);
        pk.is_hex = data.value("is_hex", false);
        if (data.contains("attributes")) pk.attributes = data.at("attributes");
        if (data.contains("signed_attributes")) {
            for (auto& [key, arr] : data.at("signed_attributes").items()) {
                std::vector<Signature> sigs;
                for (const auto& s : arr) sigs.push_back(Signature::FromJson(s));
                pk.signed_attributes[key] = std::move(sigs);
            }
        }
        return pk;
    }
};

}  // namespace ra::common::identity
