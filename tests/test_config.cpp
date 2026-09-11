#include "doctest/doctest.h"
#include "ra_common/config.hpp"

using namespace ra::common;

TEST_CASE("config: properties, env, args, system dirs") {
    auto props = ParseProperties("# comment\nfoo=bar\nbaz: qux\n\nempty=\n");
    CHECK(props["foo"] == "bar");
    CHECK(props["baz"] == "qux");
    CHECK(props["empty"] == "");

    auto env = LoadFromEnv();
    CHECK_FALSE(env.empty());

    auto args = LoadFromArgs({"a=1", "b=2", "noeq"});
    CHECK(args["a"] == "1");
    CHECK(args["b"] == "2");
    CHECK(args.count("noeq") == 0);

    CHECK(system_settings::UserHomeDir().has_value());
    CHECK(system_settings::UserDataDir().has_value());
}
