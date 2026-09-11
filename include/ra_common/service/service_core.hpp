#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ra_common/envelope.hpp"
#include "ra_common/messaging/channel.hpp"
#include "ra_common/service/service_status.hpp"

namespace ra::common::service {

/// State shared by every service (ports the BaseService fields).
class ServiceCore {
public:
    explicit ServiceCore(std::string service_class_name) : service_class_name_(std::move(service_class_name)) {}

    const std::string& service_class_name() const { return service_class_name_; }
    ServiceStatus status() const { return status_; }
    bool registered = false;
    std::optional<std::string> version;
    std::vector<std::string> services_dependent_upon;
    std::map<std::string, std::string> config;
    messaging::IMessageProducer* producer = nullptr;   // non-owning; set by the hosting bus
    IServiceStatusObserver* observer = nullptr;         // non-owning

    void AddDependentService(const std::string& name) { services_dependent_upon.push_back(name); }

    bool Send(Envelope& envelope) { return producer != nullptr && producer->Send(envelope); }

    ServiceReport Report() const {
        return ServiceReport{service_class_name_, status_,      registered,
                              status_ == ServiceStatus::Running, version, services_dependent_upon};
    }

    void UpdateStatus(ServiceStatus status) {
        if (status_ == status) return;
        status_ = status;
        if (observer != nullptr) observer->ServiceStatusChanged(service_class_name_, status);
        if (producer != nullptr) {
            auto ev = messaging::EventMessage::Of(ServiceStatusEventType());
            ev.message_value = Report().ToJson();
            auto e = Envelope::Event(ServiceStatusEventType());
            e.message = std::make_unique<messaging::EventMessage>(std::move(ev));
            e.AddRoute("ra.notification.NotificationService", "PUBLISH");
            e.Ratchet();
            Send(e);
        }
    }

private:
    static const char* ServiceStatusEventType() { return messaging::EventType::kServiceStatus; }

    std::string service_class_name_;
    ServiceStatus status_ = ServiceStatus::NotInitialized;
};

}  // namespace ra::common::service
