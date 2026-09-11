#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "ra_common/identity/did.hpp"

// Minimal network identity types needed by route and envelope. Ports
// ra.common.network.Network, NetworkStatus and NetworkPeer. The full network
// service layer is deferred to a later phase.

namespace ra::common {

enum class Network {
    Card,
    Nfc,
    Http,
    Tor,
    I2p,
    WiFi,
    Bluetooth,
    Satellite,
    FsRadio,
    LiFi,
};

inline std::string ToString(Network n) {
    switch (n) {
        case Network::Card: return "Card";
        case Network::Nfc: return "Nfc";
        case Network::Http: return "Http";
        case Network::Tor: return "Tor";
        case Network::I2p: return "I2p";
        case Network::WiFi: return "WiFi";
        case Network::Bluetooth: return "Bluetooth";
        case Network::Satellite: return "Satellite";
        case Network::FsRadio: return "FsRadio";
        case Network::LiFi: return "LiFi";
    }
    return "Http";
}

inline Network NetworkFromString(const std::string& s) {
    if (s == "Card") return Network::Card;
    if (s == "Nfc") return Network::Nfc;
    if (s == "Tor") return Network::Tor;
    if (s == "I2p") return Network::I2p;
    if (s == "WiFi") return Network::WiFi;
    if (s == "Bluetooth") return Network::Bluetooth;
    if (s == "Satellite") return Network::Satellite;
    if (s == "FsRadio") return Network::FsRadio;
    if (s == "LiFi") return Network::LiFi;
    return Network::Http;
}

enum class NetworkStatus {
    NotInstalled,
    Closed,
    Error,
    PortConflict,
    Waiting,
    Warmup,
    Connecting,
    Connected,
    Verified,
    Hanging,
    Failed,
    Blocked,
    Disconnected,
};

/// A peer in a peer-to-peer network. Equality follows the Java version: two
/// peers are equal iff both have a public-key address and fingerprint and both match.
class NetworkPeer {
public:
    Network network_value;
    identity::Did did;
    std::optional<std::string> id;
    std::optional<int> port;
    std::vector<std::string> services;

    explicit NetworkPeer(Network network = Network::Http, identity::Did did_value = identity::Did())
        : network_value(network), did(std::move(did_value)) {}

    static NetworkPeer WithCredentials(Network network, const std::string& username,
                                        std::optional<std::string> passphrase = std::nullopt) {
        auto did = identity::Did::WithUsername(username);
        did.passphrase = passphrase;
        return NetworkPeer(network, did);
    }

    bool Equals(const NetworkPeer& other) const {
        auto key = [](const NetworkPeer& p) -> std::optional<std::string> {
            if (p.did.public_key.address_value && p.did.public_key.fingerprint_value) {
                return *p.did.public_key.address_value + " " + *p.did.public_key.fingerprint_value;
            }
            return std::nullopt;
        };
        auto a = key(*this);
        return a.has_value() && a == key(other);
    }

    nlohmann::json ToJson() const {
        nlohmann::json obj = nlohmann::json::object();
        if (id) obj["id"] = *id;
        if (port) obj["port"] = *port;
        obj["network"] = ToString(network_value);
        obj["did"] = did.ToJson();
        if (!services.empty()) obj["services"] = services;
        return obj;
    }

    static NetworkPeer FromJson(const nlohmann::json& data) {
        NetworkPeer peer(data.contains("network") ? NetworkFromString(data.at("network").get<std::string>()) : Network::Http,
                          data.contains("did") ? identity::Did::FromJson(data.at("did")) : identity::Did());
        if (data.contains("id")) peer.id = data.at("id").get<std::string>();
        if (data.contains("port")) peer.port = data.at("port").get<int>();
        if (data.contains("services")) peer.services = data.at("services").get<std::vector<std::string>>();
        return peer;
    }
};

}  // namespace ra::common
