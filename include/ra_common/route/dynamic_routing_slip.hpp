#pragma once

#include <deque>
#include <memory>
#include <stdexcept>

#include "nlohmann/json.hpp"
#include "ra_common/route/external_route.hpp"
#include "ra_common/route/route.hpp"

namespace ra::common::route {

class DynamicRoutingSlip;

/// Rebuilds a polymorphic Route from its `type` tag.
inline std::unique_ptr<Route> RouteFromJson(const nlohmann::json& data);

/// A LIFO stack of routes walked one hop at a time.
class DynamicRoutingSlip : public Route {
public:
    explicit DynamicRoutingSlip(RouteMeta meta = RouteMeta::New()) : meta_(std::move(meta)) {}

    std::string Type() const override { return "routing_slip"; }
    RouteMeta& Meta() override { return meta_; }
    const RouteMeta& Meta() const override { return meta_; }

    std::unique_ptr<Route> Clone() const override {
        auto slip = std::make_unique<DynamicRoutingSlip>(meta_);
        for (const auto& r : routes_) slip->routes_.push_back(r->Clone());
        if (current_) slip->current_ = current_->Clone();
        return slip;
    }

    /// Push `route` onto the stack, stamping it with this slip's route id.
    void AddRoute(std::unique_ptr<Route> route) {
        route->set_route_id(meta_.route_id);
        routes_.push_front(std::move(route));
    }

    size_t NumberRemainingRoutes() const { return routes_.size(); }

    Route* CurrentRoute() {
        if (current_ == nullptr) NextRoute();
        return current_.get();
    }

    Route* NextRoute() {
        if (routes_.empty()) {
            current_.reset();
            return nullptr;
        }
        current_ = std::move(routes_.front());
        routes_.pop_front();
        return current_.get();
    }

    Route* PeekAtNextRoute() const { return routes_.empty() ? nullptr : routes_.front().get(); }

    nlohmann::json ToJson() const override {
        nlohmann::json routes_json = nlohmann::json::array();
        for (const auto& r : routes_) routes_json.push_back(r->ToJson());
        nlohmann::json obj = {{"type", Type()}, {"meta", meta_.ToJson()}, {"routes", routes_json}};
        if (current_) obj["current_route"] = current_->ToJson();
        return obj;
    }

    static DynamicRoutingSlip FromJson(const nlohmann::json& data) {
        DynamicRoutingSlip slip(data.contains("meta") ? RouteMeta::FromJson(data.at("meta")) : RouteMeta::New());
        if (data.contains("routes")) {
            for (const auto& r : data.at("routes")) slip.routes_.push_back(RouteFromJson(r));
        }
        if (data.contains("current_route")) slip.current_ = RouteFromJson(data.at("current_route"));
        return slip;
    }

private:
    RouteMeta meta_;
    std::deque<std::unique_ptr<Route>> routes_;
    std::unique_ptr<Route> current_;
};

inline std::unique_ptr<Route> RouteFromJson(const nlohmann::json& data) {
    std::string type = data.at("type").get<std::string>();
    if (type == "simple") return std::make_unique<SimpleRoute>(SimpleRoute::FromJson(data));
    if (type == "routing_slip") return std::make_unique<DynamicRoutingSlip>(DynamicRoutingSlip::FromJson(data));
    if (type == "simple_external") return std::make_unique<SimpleExternalRoute>(SimpleExternalRoute::FromJson(data));
    if (type == "relayed_external") return std::make_unique<RelayedExternalRoute>(RelayedExternalRoute::FromJson(data));
    throw std::runtime_error("unknown route type: " + type);
}

}  // namespace ra::common::route
