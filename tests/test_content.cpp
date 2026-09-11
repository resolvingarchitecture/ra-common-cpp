#include "doctest/doctest.h"
#include "ra_common/content.hpp"

using namespace ra::common;

TEST_CASE("content build + magnet") {
    std::string hello = "hello";
    std::vector<uint8_t> body(hello.begin(), hello.end());
    auto c = Content::Build(body, "text/plain", std::nullopt, std::string("greeting"), true, true);
    CHECK(c.kind() == ContentKind::Text);
    CHECK(c.size() == 5);
    CHECK(c.id.has_value());
    CHECK(c.hash.has_value());
    CHECK(c.fingerprint.has_value());

    auto back = Content::FromJson(nlohmann::json::parse(c.ToJson().dump()));
    CHECK(back.kind() == ContentKind::Text);
    CHECK(back.body().value() == body);

    Content b(ContentKind::Binary, "application/octet-stream");
    b.SetBody({1, 2, 3, 4}, true);
    b.AddKeyword("alpha");
    b.AddKeyword("beta");
    auto link = b.MagnetLink();
    CHECK(link.has_value());
    CHECK(link->rfind("magnet:?xl=4", 0) == 0);
    CHECK(link->find("kt=alpha+beta") != std::string::npos);
}
