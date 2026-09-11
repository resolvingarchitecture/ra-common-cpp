#pragma once

#include <optional>
#include <string>

#include "ra_common/lifecycle.hpp"

namespace ra::common::messaging {

class IMessageProducer {
public:
    virtual ~IMessageProducer() = default;
    virtual bool Send(Envelope& envelope) = 0;
    virtual bool SendWithCallback(Envelope& envelope, IClient& callback) {
        (void)envelope;
        (void)callback;
        return false;
    }
    virtual bool DeadLetter(Envelope& envelope) {
        (void)envelope;
        return false;
    }
};

class IMessageConsumer {
public:
    virtual ~IMessageConsumer() = default;
    virtual bool Receive(Envelope& envelope) = 0;
};

class IMessageChannel : public IMessageProducer, public ILifeCycle {
public:
    virtual std::string Name() const = 0;
    virtual bool IsPubSub() const = 0;
    virtual int Queued() const = 0;
    virtual void Ack(Envelope& envelope) = 0;
};

class IMessageBus : public ILifeCycle {
public:
    virtual bool RegisterChannel(const std::string& name, std::optional<std::string> service_level = std::nullopt) = 0;
    virtual bool Publish(Envelope& envelope) = 0;
    virtual bool PublishWithCallback(Envelope& envelope, IClient& callback) {
        (void)envelope;
        (void)callback;
        return false;
    }
    virtual bool Completed(Envelope& envelope) = 0;
};

}  // namespace ra::common::messaging
