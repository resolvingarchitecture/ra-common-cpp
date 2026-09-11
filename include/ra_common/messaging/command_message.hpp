#pragma once

#include <optional>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/messaging/message.hpp"

namespace ra::common::messaging {

enum class Command {
    Start,
    Shutdown,
    GracefullyShutdown,
    Restart,
    Pause,
    Unpause,
    NetState,
    Report,
    RegisterStateChangeListener,
    UnregisterStateChangeListener,
};

inline std::string ToString(Command c) {
    switch (c) {
        case Command::Start: return "Start";
        case Command::Shutdown: return "Shutdown";
        case Command::GracefullyShutdown: return "GracefullyShutdown";
        case Command::Restart: return "Restart";
        case Command::Pause: return "Pause";
        case Command::Unpause: return "Unpause";
        case Command::NetState: return "NetState";
        case Command::Report: return "Report";
        case Command::RegisterStateChangeListener: return "RegisterStateChangeListener";
        case Command::UnregisterStateChangeListener: return "UnregisterStateChangeListener";
    }
    return "Start";
}

inline Command CommandFromString(const std::string& s) {
    if (s == "Shutdown") return Command::Shutdown;
    if (s == "GracefullyShutdown") return Command::GracefullyShutdown;
    if (s == "Restart") return Command::Restart;
    if (s == "Pause") return Command::Pause;
    if (s == "Unpause") return Command::Unpause;
    if (s == "NetState") return Command::NetState;
    if (s == "Report") return Command::Report;
    if (s == "RegisterStateChangeListener") return Command::RegisterStateChangeListener;
    if (s == "UnregisterStateChangeListener") return Command::UnregisterStateChangeListener;
    return Command::Start;
}

class CommandMessage : public Message {
public:
    explicit CommandMessage(std::optional<Command> command = std::nullopt) : command_(command) {}

    std::string Kind() const override { return "command"; }
    std::optional<Command> command() const { return command_; }
    void set_command(std::optional<Command> c) { command_ = c; }

    nlohmann::json ToJson() const override {
        nlohmann::json obj = {{"kind", Kind()}};
        if (command_) obj["command"] = ToString(*command_);
        return WithErrors(obj);
    }

    static CommandMessage FromJson(const nlohmann::json& data) {
        CommandMessage m(data.contains("command") ? std::optional<Command>(CommandFromString(data.at("command").get<std::string>()))
                                                    : std::nullopt);
        m.error_messages = ErrorsFromJson(data);
        return m;
    }

private:
    std::optional<Command> command_;
};

}  // namespace ra::common::messaging
