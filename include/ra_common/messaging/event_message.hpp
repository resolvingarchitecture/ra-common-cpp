#pragma once

#include <optional>
#include <sstream>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/messaging/message.hpp"
#include "ra_common/util/random_util.hpp"

namespace ra::common::messaging {

struct EventType {
    static constexpr const char* kError = "ERROR";
    static constexpr const char* kException = "EXCEPTION";
    static constexpr const char* kBusStatus = "BUS_STATUS";
    static constexpr const char* kPeerStatus = "PEER_STATUS";
    static constexpr const char* kServiceStatus = "SERVICE_STATUS";
    static constexpr const char* kDidStatus = "DID_STATUS";
    static constexpr const char* kNetworkStateUpdate = "NETWORK_STATE_UPDATE";
    static constexpr const char* kPriceChange = "PRICE_CHANGE";
};

namespace detail {
inline std::string RandomUuidLike() {
    // Not RFC 4122 (no version/variant bits) - just a unique-enough id for
    // matching request/response events, same bar the other ports' `id` field needs.
    auto bytes = RandomBytesOf(16);
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    for (size_t i = 0; i < bytes.size(); i++) {
        if (i == 4 || i == 6 || i == 8 || i == 10) out += '-';
        out += kHex[bytes[i] >> 4];
        out += kHex[bytes[i] & 0xf];
    }
    return out;
}
}  // namespace detail

class EventMessage : public Message {
public:
    explicit EventMessage(std::string event_type, std::optional<std::string> id = std::nullopt)
        : id_(id.value_or(detail::RandomUuidLike())), event_type_(std::move(event_type)) {}

    static EventMessage Of(const std::string& event_type) { return EventMessage(event_type); }

    std::string Kind() const override { return "event"; }
    const std::string& id() const { return id_; }
    const std::string& event_type() const { return event_type_; }
    void set_event_type(std::string t) { event_type_ = std::move(t); }

    std::optional<std::string> name;
    nlohmann::json message_value;

    nlohmann::json ToJson() const override {
        nlohmann::json obj = {{"kind", Kind()}, {"id", id_}, {"event_type", event_type_}};
        if (name) obj["name"] = *name;
        if (!message_value.is_null()) obj["message"] = message_value;
        return WithErrors(obj);
    }

    static EventMessage FromJson(const nlohmann::json& data) {
        EventMessage m(data.at("event_type").get<std::string>(),
                        data.contains("id") ? std::optional<std::string>(data.at("id").get<std::string>()) : std::nullopt);
        if (data.contains("name")) m.name = data.at("name").get<std::string>();
        if (data.contains("message")) m.message_value = data.at("message");
        m.error_messages = ErrorsFromJson(data);
        return m;
    }

private:
    std::string id_;
    std::string event_type_;
};

}  // namespace ra::common::messaging
