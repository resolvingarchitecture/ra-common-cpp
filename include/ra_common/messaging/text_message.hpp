#pragma once

#include <optional>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/identity/did.hpp"
#include "ra_common/messaging/message.hpp"

namespace ra::common::messaging {

class TextMessage : public Message {
public:
    std::optional<identity::Did> to;
    std::optional<identity::Did> from;
    std::optional<std::string> text;

    std::string Kind() const override { return "text"; }

    nlohmann::json ToJson() const override {
        nlohmann::json obj = {{"kind", Kind()}};
        if (to) obj["to"] = to->ToJson();
        if (from) obj["from"] = from->ToJson();
        if (text) obj["text"] = *text;
        return WithErrors(obj);
    }

    static TextMessage FromJson(const nlohmann::json& data) {
        TextMessage m;
        if (data.contains("to")) m.to = identity::Did::FromJson(data.at("to"));
        if (data.contains("from")) m.from = identity::Did::FromJson(data.at("from"));
        if (data.contains("text")) m.text = data.at("text").get<std::string>();
        m.error_messages = ErrorsFromJson(data);
        return m;
    }
};

}  // namespace ra::common::messaging
