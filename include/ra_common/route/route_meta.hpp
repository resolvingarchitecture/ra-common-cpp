#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/util/random_util.hpp"

namespace ra::common::route {

struct RouteMeta {
    std::optional<std::string> service;
    std::optional<std::string> operation;
    bool routed = false;
    int64_t route_id = 0;

    static RouteMeta New(std::optional<std::string> service = std::nullopt,
                          std::optional<std::string> operation = std::nullopt) {
        return RouteMeta{std::move(service), std::move(operation), false, NextLong()};
    }

    nlohmann::json ToJson() const {
        nlohmann::json obj = nlohmann::json::object();
        if (service) obj["service"] = *service;
        if (operation) obj["operation"] = *operation;
        obj["routed"] = routed;
        obj["route_id"] = std::to_string(route_id);
        return obj;
    }

    static RouteMeta FromJson(const nlohmann::json& data) {
        RouteMeta m;
        if (data.contains("service")) m.service = data.at("service").get<std::string>();
        if (data.contains("operation")) m.operation = data.at("operation").get<std::string>();
        m.routed = data.value("routed", false);
        m.route_id = data.contains("route_id") ? std::stoll(data.at("route_id").get<std::string>()) : 0;
        return m;
    }
};

}  // namespace ra::common::route
