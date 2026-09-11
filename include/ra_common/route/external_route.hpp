#pragma once

#include <optional>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/network.hpp"
#include "ra_common/route/route.hpp"

namespace ra::common::route {

class SimpleExternalRoute : public Route {
public:
    explicit SimpleExternalRoute(RouteMeta meta = RouteMeta::New()) : meta_(std::move(meta)) {}

    std::optional<NetworkPeer> origination;
    std::optional<NetworkPeer> destination;
    bool send_content_only = false;
    int status_code = 0;

    static SimpleExternalRoute Of(const std::string& service, const std::string& operation) {
        SimpleExternalRoute r(RouteMeta::New(service, operation));
        r.send_content_only = true;
        return r;
    }

    std::string Type() const override { return "simple_external"; }
    RouteMeta& Meta() override { return meta_; }
    const RouteMeta& Meta() const override { return meta_; }

    nlohmann::json ToJson() const override {
        nlohmann::json obj = {{"type", Type()}, {"meta", meta_.ToJson()}};
        if (origination) obj["origination"] = origination->ToJson();
        if (destination) obj["destination"] = destination->ToJson();
        obj["send_content_only"] = send_content_only;
        obj["status_code"] = status_code;
        return obj;
    }

    static SimpleExternalRoute FromJson(const nlohmann::json& data) {
        SimpleExternalRoute r(data.contains("meta") ? RouteMeta::FromJson(data.at("meta")) : RouteMeta::New());
        if (data.contains("origination")) r.origination = NetworkPeer::FromJson(data.at("origination"));
        if (data.contains("destination")) r.destination = NetworkPeer::FromJson(data.at("destination"));
        r.send_content_only = data.value("send_content_only", false);
        r.status_code = data.value("status_code", 0);
        return r;
    }

private:
    RouteMeta meta_;
};

class RelayedExternalRoute : public Route {
public:
    explicit RelayedExternalRoute(SimpleExternalRoute base = SimpleExternalRoute()) : base_(std::move(base)) {}

    std::optional<NetworkPeer> from_peer;
    std::optional<NetworkPeer> to_peer;
    bool delayed = false;
    int min_delay = 0;
    int max_delay = 0;
    bool copy = false;
    int min_copies = 0;
    int max_copies = 0;
    int sensitivity = 0;

    std::string Type() const override { return "relayed_external"; }
    RouteMeta& Meta() override { return base_.Meta(); }
    const RouteMeta& Meta() const override { return base_.Meta(); }
    const SimpleExternalRoute& base() const { return base_; }

    nlohmann::json ToJson() const override {
        nlohmann::json obj = {{"type", Type()}, {"base", base_.ToJson()}};
        if (from_peer) obj["from_peer"] = from_peer->ToJson();
        if (to_peer) obj["to_peer"] = to_peer->ToJson();
        obj["delayed"] = delayed;
        obj["min_delay"] = min_delay;
        obj["max_delay"] = max_delay;
        obj["copy"] = copy;
        obj["min_copies"] = min_copies;
        obj["max_copies"] = max_copies;
        obj["sensitivity"] = sensitivity;
        return obj;
    }

    static RelayedExternalRoute FromJson(const nlohmann::json& data) {
        RelayedExternalRoute r(data.contains("base") ? SimpleExternalRoute::FromJson(data.at("base"))
                                                       : SimpleExternalRoute());
        if (data.contains("from_peer")) r.from_peer = NetworkPeer::FromJson(data.at("from_peer"));
        if (data.contains("to_peer")) r.to_peer = NetworkPeer::FromJson(data.at("to_peer"));
        r.delayed = data.value("delayed", false);
        r.min_delay = data.value("min_delay", 0);
        r.max_delay = data.value("max_delay", 0);
        r.copy = data.value("copy", false);
        r.min_copies = data.value("min_copies", 0);
        r.max_copies = data.value("max_copies", 0);
        r.sensitivity = data.value("sensitivity", 0);
        return r;
    }

private:
    SimpleExternalRoute base_;
};

struct ExternalStatus {
    static constexpr int kDestinationPeerRequired = 2;
    static constexpr int kDestinationPeerWrongNetwork = 3;
    static constexpr int kDestinationPeerNotFound = 4;
    static constexpr int kNoService = 7;
    static constexpr int kNoOperation = 8;
    static constexpr int kNoAddress = 9;
    static constexpr int kNoFingerprint = 10;
    static constexpr int kNoPort = 11;
};

}  // namespace ra::common::route
