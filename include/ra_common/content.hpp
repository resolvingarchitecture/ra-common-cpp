#pragma once

#include <cctype>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "ra_common/crypto/encryption_algorithm.hpp"
#include "ra_common/crypto/hash.hpp"
#include "ra_common/crypto/hash_algorithm.hpp"
#include "ra_common/crypto/hash_util.hpp"
#include "ra_common/encoding.hpp"
#include "ra_common/exception.hpp"
#include "ra_common/util/random_util.hpp"

// Typed content submitted to the network for dissemination. Ports
// ra.common.content.Content and its subclasses (Text, HTML, JSON, Binary,
// Image, Audio, Video). The Java class hierarchy collapses into one Content
// class tagged with a ContentKind.

namespace ra::common {

enum class ContentKind {
    Text,
    Html,
    Json,
    Image,
    Audio,
    Video,
    Binary,
};

inline std::string ToString(ContentKind k) {
    switch (k) {
        case ContentKind::Text: return "Text";
        case ContentKind::Html: return "Html";
        case ContentKind::Json: return "Json";
        case ContentKind::Image: return "Image";
        case ContentKind::Audio: return "Audio";
        case ContentKind::Video: return "Video";
        case ContentKind::Binary: return "Binary";
    }
    return "Binary";
}

inline ContentKind ContentKindFromString(const std::string& s) {
    if (s == "Text") return ContentKind::Text;
    if (s == "Html") return ContentKind::Html;
    if (s == "Json") return ContentKind::Json;
    if (s == "Image") return ContentKind::Image;
    if (s == "Audio") return ContentKind::Audio;
    if (s == "Video") return ContentKind::Video;
    return ContentKind::Binary;
}

inline std::optional<ContentKind> ContentKindForType(const std::string& content_type) {
    auto starts_with = [&](const char* prefix) { return content_type.rfind(prefix, 0) == 0; };
    if (starts_with("text/plain")) return ContentKind::Text;
    if (starts_with("text/html")) return ContentKind::Html;
    if (starts_with("application/json")) return ContentKind::Json;
    if (starts_with("image/")) return ContentKind::Image;
    if (starts_with("audio/")) return ContentKind::Audio;
    if (starts_with("video/")) return ContentKind::Video;
    return std::nullopt;
}

inline bool ContentKindIsText(ContentKind k) {
    return k == ContentKind::Text || k == ContentKind::Html || k == ContentKind::Json;
}

class Content {
public:
    Content(ContentKind kind, std::string content_type) : kind_(kind), content_type_(std::move(content_type)) {}

    ContentKind kind() const { return kind_; }
    void set_kind(ContentKind k) { kind_ = k; }
    const std::string& content_type() const { return content_type_; }
    int version() const { return version_; }
    std::optional<std::string> id;
    std::optional<std::string> label;
    std::optional<std::string> name;
    std::optional<std::string> location;
    size_t size() const { return size_; }
    std::optional<std::string> author_alias;
    std::optional<std::string> author_address;
    const std::optional<std::vector<uint8_t>>& body() const { return body_; }
    std::optional<std::string> body_encoding;
    bool body_base64_encoded = false;
    std::optional<int64_t> created_at;
    std::optional<crypto::Hash> hash;
    crypto::HashAlgorithm hash_algorithm = crypto::HashAlgorithm::Sha256;
    std::optional<crypto::Hash> fingerprint;
    crypto::HashAlgorithm fingerprint_algorithm = crypto::HashAlgorithm::Sha1;
    std::vector<Content> children;
    bool encrypted = false;
    std::optional<crypto::EncryptionAlgorithm> encryption_algorithm;
    std::optional<std::string> encryption_passphrase;
    bool encryption_passphrase_encrypted = false;
    std::optional<crypto::EncryptionAlgorithm> encryption_passphrase_algorithm;
    std::optional<std::string> base64_encoded_iv;
    std::vector<std::string> keywords;
    bool readable = false;
    bool writeable = false;

    static Content Build(const std::vector<uint8_t>& body, const std::string& content_type,
                          std::optional<std::string> label = std::nullopt, std::optional<std::string> name = std::nullopt,
                          bool generate_hash = false, bool generate_fingerprint = false) {
        auto kind = ContentKindForType(content_type);
        if (!kind) throw RaException::Invalid("unsupported content type: " + content_type);
        Content c(*kind, content_type);
        c.label = std::move(label);
        c.name = std::move(name);
        auto charset_pos = content_type.find("charset:");
        if (charset_pos != std::string::npos) c.body_encoding = content_type.substr(charset_pos + 8);
        c.SetBody(body, generate_hash, generate_fingerprint);
        c.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();
        c.id = RandomAlphanumeric(32);
        return c;
    }

    void SetBody(std::vector<uint8_t> body, bool generate_hash = false, bool generate_fingerprint = false) {
        size_ = body.size();
        if (generate_hash) {
            hash = crypto::Hash(crypto::hash_util::GenerateHash(body, hash_algorithm), hash_algorithm);
        }
        if (generate_fingerprint && hash) {
            const auto& hv = hash->hash_value();
            auto fp = crypto::hash_util::GenerateFingerprint(std::vector<uint8_t>(hv.begin(), hv.end()), fingerprint_algorithm);
            fingerprint = crypto::Hash(fp, fingerprint_algorithm);
        }
        body_ = std::move(body);
        version_ += 1;
    }

    bool MetaOnly() const { return !body_.has_value(); }

    void AddKeyword(const std::string& keyword) { keywords.push_back(keyword); }
    void AddChild(Content child) { children.push_back(std::move(child)); }

    std::optional<std::string> MagnetLink() const {
        std::vector<std::string> parts;
        if (body_) parts.push_back("xl=" + std::to_string(body_->size()));
        if (hash) {
            std::string jca = crypto::JcaName(hash->algorithm());
            for (auto& c : jca) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            parts.push_back("xt=urn:" + jca + ":" + hash->hash_value());
        }
        if (!keywords.empty()) {
            std::string joined;
            for (size_t i = 0; i < keywords.size(); i++) joined += (i == 0 ? "" : "+") + keywords[i];
            parts.push_back("kt=" + joined);
        }
        if (parts.empty()) return std::nullopt;
        std::string joined;
        for (size_t i = 0; i < parts.size(); i++) joined += (i == 0 ? "" : "&") + parts[i];
        return "magnet:?" + joined;
    }

    nlohmann::json ToJson() const {
        nlohmann::json obj = {
            {"type", ra::common::ToString(kind_)}, {"content_type", content_type_}, {"version", version_},
        };
        if (id) obj["id"] = *id;
        if (label) obj["label"] = *label;
        if (name) obj["name"] = *name;
        if (location) obj["location"] = *location;
        if (author_alias) obj["author_alias"] = *author_alias;
        if (author_address) obj["author_address"] = *author_address;
        if (body_) obj["body"] = Base64Encode(*body_);
        if (body_encoding) obj["body_encoding"] = *body_encoding;
        if (created_at) obj["created_at"] = *created_at;
        if (hash) obj["hash"] = hash->ToJson();
        if (fingerprint) obj["fingerprint"] = fingerprint->ToJson();
        if (encryption_algorithm) obj["encryption_algorithm"] = crypto::ToString(*encryption_algorithm);
        if (encryption_passphrase) obj["encryption_passphrase"] = *encryption_passphrase;
        if (encryption_passphrase_algorithm) {
            obj["encryption_passphrase_algorithm"] = crypto::ToString(*encryption_passphrase_algorithm);
        }
        if (base64_encoded_iv) obj["base64_encoded_iv"] = *base64_encoded_iv;
        obj["size"] = size_;
        obj["body_base64_encoded"] = body_base64_encoded;
        obj["hash_algorithm"] = crypto::ToString(hash_algorithm);
        obj["fingerprint_algorithm"] = crypto::ToString(fingerprint_algorithm);
        obj["encrypted"] = encrypted;
        obj["encryption_passphrase_encrypted"] = encryption_passphrase_encrypted;
        obj["readable"] = readable;
        obj["writeable"] = writeable;
        if (!children.empty()) {
            nlohmann::json kids = nlohmann::json::array();
            for (const auto& c : children) kids.push_back(c.ToJson());
            obj["children"] = kids;
        }
        if (!keywords.empty()) obj["keywords"] = keywords;
        return obj;
    }

    static Content FromJson(const nlohmann::json& data) {
        Content c(ContentKindFromString(data.at("type").get<std::string>()), data.at("content_type").get<std::string>());
        c.version_ = data.value("version", 0);
        if (data.contains("id")) c.id = data.at("id").get<std::string>();
        if (data.contains("label")) c.label = data.at("label").get<std::string>();
        if (data.contains("name")) c.name = data.at("name").get<std::string>();
        if (data.contains("location")) c.location = data.at("location").get<std::string>();
        c.size_ = data.value("size", static_cast<size_t>(0));
        if (data.contains("author_alias")) c.author_alias = data.at("author_alias").get<std::string>();
        if (data.contains("author_address")) c.author_address = data.at("author_address").get<std::string>();
        if (data.contains("body")) c.body_ = Base64Decode(data.at("body").get<std::string>());
        if (data.contains("body_encoding")) c.body_encoding = data.at("body_encoding").get<std::string>();
        c.body_base64_encoded = data.value("body_base64_encoded", false);
        if (data.contains("created_at")) c.created_at = data.at("created_at").get<int64_t>();
        if (data.contains("hash")) c.hash = crypto::Hash::FromJson(data.at("hash"));
        c.hash_algorithm = data.contains("hash_algorithm") ? crypto::ParseHashAlgorithm(data.at("hash_algorithm").get<std::string>())
                                                            : crypto::HashAlgorithm::Sha256;
        if (data.contains("fingerprint")) c.fingerprint = crypto::Hash::FromJson(data.at("fingerprint"));
        c.fingerprint_algorithm = data.contains("fingerprint_algorithm")
                                       ? crypto::ParseHashAlgorithm(data.at("fingerprint_algorithm").get<std::string>())
                                       : crypto::HashAlgorithm::Sha1;
        if (data.contains("children")) {
            for (const auto& child : data.at("children")) c.children.push_back(Content::FromJson(child));
        }
        c.encrypted = data.value("encrypted", false);
        if (data.contains("encryption_algorithm")) {
            c.encryption_algorithm = crypto::EncryptionAlgorithmFromString(data.at("encryption_algorithm").get<std::string>());
        }
        if (data.contains("encryption_passphrase")) c.encryption_passphrase = data.at("encryption_passphrase").get<std::string>();
        c.encryption_passphrase_encrypted = data.value("encryption_passphrase_encrypted", false);
        if (data.contains("encryption_passphrase_algorithm")) {
            c.encryption_passphrase_algorithm =
                crypto::EncryptionAlgorithmFromString(data.at("encryption_passphrase_algorithm").get<std::string>());
        }
        if (data.contains("base64_encoded_iv")) c.base64_encoded_iv = data.at("base64_encoded_iv").get<std::string>();
        if (data.contains("keywords")) c.keywords = data.at("keywords").get<std::vector<std::string>>();
        c.readable = data.value("readable", false);
        c.writeable = data.value("writeable", false);
        return c;
    }

private:
    ContentKind kind_;
    std::string content_type_;
    int version_ = 0;
    size_t size_ = 0;
    std::optional<std::vector<uint8_t>> body_;
};

}  // namespace ra::common
