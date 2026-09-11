#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

// Messages carried inside an Envelope. Ports the ra.common.messaging package.
// The Java Message interface + BaseMessage + concrete subclasses become a
// small class hierarchy tagged with `kind` on the wire.

namespace ra::common::messaging {

inline constexpr const char* kContent = "CONTENT";
inline constexpr const char* kEntity = "ENTITY";
inline constexpr const char* kExceptions = "EXCEPTIONS";

class Message {
public:
    virtual ~Message() = default;
    virtual std::string Kind() const = 0;
    virtual nlohmann::json ToJson() const = 0;

    std::vector<std::string> error_messages;

    void AddErrorMessage(const std::string& msg) { error_messages.push_back(msg); }
    void ClearErrorMessages() { error_messages.clear(); }

protected:
    nlohmann::json WithErrors(nlohmann::json obj) const {
        if (!error_messages.empty()) obj["error_messages"] = error_messages;
        return obj;
    }

    static std::vector<std::string> ErrorsFromJson(const nlohmann::json& data) {
        return data.contains("error_messages") ? data.at("error_messages").get<std::vector<std::string>>()
                                                : std::vector<std::string>();
    }
};

/// Rebuilds a polymorphic Message from its `kind` tag. Defined after the
/// concrete message classes in message_json.hpp (needs to see all of them).
inline std::unique_ptr<Message> MessageFromJson(const nlohmann::json& data);

}  // namespace ra::common::messaging
