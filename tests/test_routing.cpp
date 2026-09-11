#include "doctest/doctest.h"
#include "ra_common/route/dynamic_routing_slip.hpp"

using namespace ra::common::route;

TEST_CASE("route slip LIFO + round trip") {
    DynamicRoutingSlip slip;
    slip.AddRoute(std::make_unique<SimpleRoute>(SimpleRoute::Of("a", "op")));
    slip.AddRoute(std::make_unique<SimpleRoute>(SimpleRoute::Of("b", "op")));
    slip.AddRoute(std::make_unique<SimpleRoute>(SimpleRoute::Of("c", "op")));
    CHECK(slip.NumberRemainingRoutes() == 3);
    CHECK(slip.NextRoute()->service() == "c");
    CHECK(slip.NextRoute()->service() == "b");
    CHECK(slip.PeekAtNextRoute()->service() == "a");

    auto json = nlohmann::json::parse(slip.ToJson().dump());
    auto back = DynamicRoutingSlip::FromJson(json);
    CHECK(back.NumberRemainingRoutes() == 1);

    auto rebuilt = RouteFromJson(slip.ToJson());
    CHECK(dynamic_cast<DynamicRoutingSlip*>(rebuilt.get()) != nullptr);
}
