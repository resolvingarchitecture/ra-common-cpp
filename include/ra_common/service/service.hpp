#pragma once

#include <map>
#include <string>

#include "ra_common/envelope.hpp"
#include "ra_common/lifecycle.hpp"
#include "ra_common/service/service_core.hpp"

namespace ra::common::service {

/// A message-driven service. Ports ra.common.service.Service + the reusable
/// parts of BaseService. Concrete services hold a ServiceCore.
class Service : public ILifeCycle {
public:
    virtual ServiceCore& Core() = 0;
    virtual const ServiceCore& Core() const = 0;

    bool Pause() override { return false; }
    bool Unpause() override { return false; }
    bool Restart() override { return false; }
    bool GracefulShutdown() override { return Shutdown(); }

    virtual void HandleDocument(Envelope& envelope) { (void)envelope; }
    virtual void HandleEvent(Envelope& envelope) { (void)envelope; }

    virtual void HandleCommand(Envelope& envelope) {
        auto* cmd_msg = dynamic_cast<messaging::CommandMessage*>(envelope.message.get());
        if (cmd_msg == nullptr || !cmd_msg->command()) return;
        auto config = Core().config;
        switch (*cmd_msg->command()) {
            case messaging::Command::Start:
                Start(config);
                break;
            case messaging::Command::Pause:
                Pause();
                break;
            case messaging::Command::Unpause:
                Unpause();
                break;
            case messaging::Command::Restart:
                Restart();
                break;
            case messaging::Command::Shutdown:
                Shutdown();
                break;
            case messaging::Command::GracefullyShutdown:
                GracefulShutdown();
                break;
            case messaging::Command::Report:
                envelope.SetHeader("result", Report().ToJson());
                break;
            default:
                break;
        }
    }

    virtual void HandleHeaders(Envelope& envelope) { (void)envelope; }

    ServiceStatus ServiceStatusValue() const { return Core().status(); }
    ServiceReport Report() const { return Core().Report(); }

    Envelope& Handle(Envelope& envelope) {
        if (dynamic_cast<messaging::DocumentMessage*>(envelope.message.get()) != nullptr) {
            HandleDocument(envelope);
        } else if (dynamic_cast<messaging::EventMessage*>(envelope.message.get()) != nullptr) {
            HandleEvent(envelope);
        } else if (dynamic_cast<messaging::CommandMessage*>(envelope.message.get()) != nullptr) {
            HandleCommand(envelope);
        } else {
            HandleHeaders(envelope);
        }
        return envelope;
    }
};

}  // namespace ra::common::service
