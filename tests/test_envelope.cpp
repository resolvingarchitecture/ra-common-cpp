#include "doctest/doctest.h"
#include "ra_common/envelope.hpp"

using namespace ra::common;
using namespace ra::common::messaging;

TEST_CASE("factories set message kind") {
    CHECK(dynamic_cast<DocumentMessage*>(Envelope::Document().message.get()) != nullptr);
    CHECK(dynamic_cast<CommandMessage*>(Envelope::Command().message.get()) != nullptr);
    CHECK(dynamic_cast<EventMessage*>(Envelope::Event(EventType::kBusStatus).message.get()) != nullptr);
    CHECK(Envelope::HeadersOnly().message == nullptr);
}

TEST_CASE("content round-trips through a document") {
    auto e = Envelope::Document();
    CHECK(e.AddContent("hello"));
    CHECK(e.Content() == "hello");
    CHECK_FALSE(Envelope::Command().AddContent(nullptr));
}

TEST_CASE("exceptions accumulate") {
    auto e = Envelope::Document();
    e.AddException("first");
    e.AddException("second");
    std::vector<std::string> expected = {"first", "second"};
    CHECK(e.Exceptions() == expected);
}

TEST_CASE("ratchet walks the slip LIFO") {
    auto e = Envelope::Document();
    e.AddRoute("ra.a.ServiceA", "OP");
    e.AddRoute("ra.b.ServiceB", "OP");
    e.Ratchet();
    CHECK(e.GetRoute()->service() == "ra.b.ServiceB");
    e.Ratchet();
    CHECK(e.GetRoute()->service() == "ra.a.ServiceA");
}

TEST_CASE("json round trip") {
    auto e = Envelope::Document();
    e.SetContentType(HeaderNames::kContentTypeJson);
    e.AddContent(42);
    e.AddRoute("ra.x.Svc", "DO");
    e.Mark("seen");
    auto back = Envelope::FromJsonString(e.ToJsonString());
    CHECK(back.id == e.id);
    CHECK(back.ContentType() == HeaderNames::kContentTypeJson);
    CHECK(back.Content() == 42);
    CHECK(back.MarkerPresent("seen"));
    CHECK(back.dynamic_routing_slip.NumberRemainingRoutes() == 1);
}

TEST_CASE("equality is by id") {
    CHECK(Envelope::DocumentWithId("same").Equals(Envelope::DocumentWithId("same")));
}
