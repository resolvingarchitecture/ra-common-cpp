#pragma once

#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/crypto/hash_algorithm.hpp"

namespace ra::common::crypto {

/// A hash string with the algorithm used. Equality is on the hash string alone.
class Hash {
public:
    Hash(std::string hash_value, HashAlgorithm algorithm = HashAlgorithm::Sha256)
        : hash_value_(std::move(hash_value)), algorithm_(algorithm) {}

    const std::string& hash_value() const { return hash_value_; }
    void set_hash_value(std::string v) { hash_value_ = std::move(v); }
    HashAlgorithm algorithm() const { return algorithm_; }
    void set_algorithm(HashAlgorithm a) { algorithm_ = a; }

    bool Equals(const Hash& other) const { return hash_value_ == other.hash_value_; }

    nlohmann::json ToJson() const { return {{"hash", hash_value_}, {"algorithm", ToString(algorithm_)}}; }

    static Hash FromJson(const nlohmann::json& data) {
        return Hash(data.at("hash").get<std::string>(), ParseHashAlgorithm(data.at("algorithm").get<std::string>()));
    }

private:
    std::string hash_value_;
    HashAlgorithm algorithm_;
};

}  // namespace ra::common::crypto
