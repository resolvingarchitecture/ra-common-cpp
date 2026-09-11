#pragma once

#include <memory>
#include <stdexcept>

#include "ra_common/messaging/command_message.hpp"
#include "ra_common/messaging/document_message.hpp"
#include "ra_common/messaging/event_message.hpp"
#include "ra_common/messaging/message.hpp"
#include "ra_common/messaging/text_message.hpp"

namespace ra::common::messaging {

/// Defines the dispatcher declared in message.hpp - split out so message.hpp
/// (needed by every concrete message class) doesn't have to see all of them.
inline std::unique_ptr<Message> MessageFromJson(const nlohmann::json& data) {
    std::string kind = data.at("kind").get<std::string>();
    if (kind == "document") return std::make_unique<DocumentMessage>(DocumentMessage::FromJson(data));
    if (kind == "command") return std::make_unique<CommandMessage>(CommandMessage::FromJson(data));
    if (kind == "event") return std::make_unique<EventMessage>(EventMessage::FromJson(data));
    if (kind == "text") return std::make_unique<TextMessage>(TextMessage::FromJson(data));
    throw std::runtime_error("unknown message kind: " + kind);
}

}  // namespace ra::common::messaging
