#pragma once

#include <map>
#include <string>

namespace ra::common {

class Envelope;  // forward declaration; see envelope.hpp

/// Coarse run state of a component. Ports ra.common.Status.
enum class Status {
    Initialized,
    Starting,
    Running,
    Paused,
    Stopping,
    Stopped,
    Errored,
};

/// Start / pause / restart / shutdown contract. Ports ra.common.LifeCycle.
/// Every method returns true on success, matching the Java API. Unpause is
/// named as such (rather than Resume) to mirror the Java note about the
/// Thread.resume clash.
class ILifeCycle {
public:
    virtual ~ILifeCycle() = default;
    virtual bool Start(const std::map<std::string, std::string>& properties) = 0;
    virtual bool Pause() { return false; }
    virtual bool Unpause() { return false; }
    virtual bool Restart() { return false; }
    virtual bool Shutdown() = 0;
    virtual bool GracefulShutdown() { return Shutdown(); }
};

/// A caller that a service can send a reply Envelope back to.
class IClient {
public:
    virtual ~IClient() = default;
    virtual void Reply(Envelope& envelope) = 0;
};

}  // namespace ra::common
