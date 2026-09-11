#include "doctest/doctest.h"
#include "ra_common/envelope.hpp"
#include "ra_common/service/service.hpp"

using namespace ra::common;
using namespace ra::common::messaging;
using namespace ra::common::service;

namespace {
class Toy : public Service {
public:
    ServiceCore core{"ra.test.Toy"};
    bool started = false;

    ServiceCore& Core() override { return core; }
    const ServiceCore& Core() const override { return core; }

    bool Start(const std::map<std::string, std::string>&) override {
        started = true;
        core.UpdateStatus(ServiceStatus::Running);
        return true;
    }
    bool Shutdown() override {
        started = false;
        return true;
    }
};
}  // namespace

TEST_CASE("service command drives lifecycle") {
    Toy toy;
    auto e = Envelope::Command();
    e.message = std::make_unique<CommandMessage>(Command::Start);
    toy.Handle(e);
    CHECK(toy.started);
    CHECK(toy.ServiceStatusValue() == ServiceStatus::Running);

    auto r = Envelope::Command();
    r.message = std::make_unique<CommandMessage>(Command::Report);
    toy.Handle(r);
    CHECK(r.HeaderExists("result"));
}
