#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

// Service status / level enums and report shape. A leaf module so Envelope
// can depend on ServiceLevel without pulling in the full Service framework
// (which depends on Envelope). Ports ra.common.service.{ServiceStatus,
// ServiceLevel, ServiceReport, ServiceMessage, ServiceStatusObserver}.

namespace ra::common::service {

enum class ServiceLevel {
    AtMostOnce,
    AtLeastOnce,
    ExactlyOnce,
};

inline std::string ToString(ServiceLevel l) {
    switch (l) {
        case ServiceLevel::AtMostOnce: return "AtMostOnce";
        case ServiceLevel::AtLeastOnce: return "AtLeastOnce";
        case ServiceLevel::ExactlyOnce: return "ExactlyOnce";
    }
    return "AtLeastOnce";
}

inline ServiceLevel ServiceLevelFromString(const std::string& s) {
    if (s == "AtMostOnce") return ServiceLevel::AtMostOnce;
    if (s == "ExactlyOnce") return ServiceLevel::ExactlyOnce;
    return ServiceLevel::AtLeastOnce;
}

enum class ServiceStatus {
    NotInitialized,
    Initializing,
    Waiting,
    Starting,
    Running,
    Verified,
    PartiallyRunning,
    DegradedRunning,
    Unstable,
    Pausing,
    Paused,
    Unpausing,
    ShuttingDown,
    GracefullyShuttingDown,
    Shutdown,
    GracefullyShutdown,
    Restarting,
    Unavailable,
    Error,
};

inline std::string ToString(ServiceStatus s) {
    switch (s) {
        case ServiceStatus::NotInitialized: return "NotInitialized";
        case ServiceStatus::Initializing: return "Initializing";
        case ServiceStatus::Waiting: return "Waiting";
        case ServiceStatus::Starting: return "Starting";
        case ServiceStatus::Running: return "Running";
        case ServiceStatus::Verified: return "Verified";
        case ServiceStatus::PartiallyRunning: return "PartiallyRunning";
        case ServiceStatus::DegradedRunning: return "DegradedRunning";
        case ServiceStatus::Unstable: return "Unstable";
        case ServiceStatus::Pausing: return "Pausing";
        case ServiceStatus::Paused: return "Paused";
        case ServiceStatus::Unpausing: return "Unpausing";
        case ServiceStatus::ShuttingDown: return "ShuttingDown";
        case ServiceStatus::GracefullyShuttingDown: return "GracefullyShuttingDown";
        case ServiceStatus::Shutdown: return "Shutdown";
        case ServiceStatus::GracefullyShutdown: return "GracefullyShutdown";
        case ServiceStatus::Restarting: return "Restarting";
        case ServiceStatus::Unavailable: return "Unavailable";
        case ServiceStatus::Error: return "Error";
    }
    return "NotInitialized";
}

inline bool ServiceStatusIsRunning(ServiceStatus status) {
    static const std::set<ServiceStatus> kRunning = {ServiceStatus::Running, ServiceStatus::Verified,
                                                       ServiceStatus::PartiallyRunning, ServiceStatus::DegradedRunning};
    return kRunning.count(status) > 0;
}

inline constexpr int kNoError = -1;
inline constexpr int kRequestRequired = 0;
inline constexpr const char* kRaServiceImpl = "ra.service.impl";

struct ServiceMessage {
    int status_code = 0;
    std::optional<std::string> error_message;
    std::optional<std::string> exception;
    std::optional<std::string> type;
};

struct ServiceReport {
    std::string service_class_name;
    ServiceStatus service_status_value;
    bool registered = false;
    bool running = false;
    std::optional<std::string> version;
    std::vector<std::string> services_dependent_upon;

    nlohmann::json ToJson() const {
        nlohmann::json obj = {
            {"service_class_name", service_class_name},
            {"service_status", ra::common::service::ToString(service_status_value)},
            {"registered", registered},
            {"running", running},
        };
        if (version) obj["version"] = *version;
        if (!services_dependent_upon.empty()) obj["services_dependent_upon"] = services_dependent_upon;
        return obj;
    }
};

class IServiceStatusObserver {
public:
    virtual ~IServiceStatusObserver() = default;
    virtual void ServiceStatusChanged(const std::string& service_full_name, ServiceStatus status) = 0;
};

}  // namespace ra::common::service
