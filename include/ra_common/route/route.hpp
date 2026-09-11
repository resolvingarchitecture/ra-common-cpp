#pragma once

#include <memory>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/route/route_meta.hpp"

// Any route variant. Ports the ra.common.route package. The Java abstract
// base + reflective Class.forName polymorphism becomes a small class
// hierarchy with a `type` tag on the wire (simple, routing_slip,
// simple_external, relayed_external), rebuilt by RouteFromJson.

namespace ra::common::route {

class Route {
public:
    virtual ~Route() = default;
    virtual std::string Type() const = 0;
    virtual RouteMeta& Meta() = 0;
    virtual const RouteMeta& Meta() const = 0;
    virtual nlohmann::json ToJson() const = 0;

    /// An independently-owned copy - Envelope::GetRoute() needs one since it
    /// caches the current route in its own unique_ptr, separate from the
    /// slip's. Deliberately NOT ToJson()-then-FromJson(): that round trip
    /// was the original implementation, and it made every GetRoute() call
    /// (i.e. every Bus::Publish, via TargetService) build and immediately
    /// discard a JSON tree just to copy a handful of scalar fields. Found
    /// the same way as random_util.hpp's fix: seda-bus-compare's benchmark
    /// showed seda-bus-cpp's full publish path running at ~11% of its own
    /// envelope-construction-only throughput, which a JSON round trip on
    /// the hot path fully explains and a plain field copy does not.
    virtual std::unique_ptr<Route> Clone() const = 0;

    std::optional<std::string> service() const { return Meta().service; }
    std::optional<std::string> operation() const { return Meta().operation; }
    bool routed() const { return Meta().routed; }
    void set_routed(bool v) { Meta().routed = v; }
    int64_t route_id() const { return Meta().route_id; }
    void set_route_id(int64_t v) { Meta().route_id = v; }
};

class SimpleRoute : public Route {
public:
    explicit SimpleRoute(RouteMeta meta = RouteMeta::New()) : meta_(std::move(meta)) {}

    static SimpleRoute Of(const std::string& service, const std::string& operation) {
        return SimpleRoute(RouteMeta::New(service, operation));
    }

    std::string Type() const override { return "simple"; }
    RouteMeta& Meta() override { return meta_; }
    const RouteMeta& Meta() const override { return meta_; }

    std::unique_ptr<Route> Clone() const override { return std::make_unique<SimpleRoute>(meta_); }

    nlohmann::json ToJson() const override { return {{"type", Type()}, {"meta", meta_.ToJson()}}; }

    static SimpleRoute FromJson(const nlohmann::json& data) {
        return SimpleRoute(data.contains("meta") ? RouteMeta::FromJson(data.at("meta")) : RouteMeta::New());
    }

private:
    RouteMeta meta_;
};

}  // namespace ra::common::route
