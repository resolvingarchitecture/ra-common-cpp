#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "ra_common/identity/did.hpp"
#include "ra_common/messaging/command_message.hpp"
#include "ra_common/messaging/document_message.hpp"
#include "ra_common/messaging/event_message.hpp"
#include "ra_common/messaging/message.hpp"
#include "ra_common/messaging/message_json.hpp"
#include "ra_common/messaging/text_message.hpp"
#include "ra_common/multipart.hpp"
#include "ra_common/route/dynamic_routing_slip.hpp"
#include "ra_common/route/external_route.hpp"
#include "ra_common/service/service_status.hpp"
#include "ra_common/util/random_util.hpp"

namespace ra::common {

struct HeaderNames {
    static constexpr const char* kAuthorization = "Authorization";
    static constexpr const char* kContentDisposition = "Content-Disposition";
    static constexpr const char* kContentTransferEncoding = "Content-Transfer-Encoding";
    static constexpr const char* kContentType = "Content-Type";
    static constexpr const char* kContentTypeJson = "application/json";
    static constexpr const char* kUserAgent = "User-Agent";
};

enum class MessageType {
    Document,
    Text,
    Event,
    Command,
    None,
};

enum class EnvelopeAction {
    Post,
    Put,
    Delete,
    Get,
};

inline std::string ToString(EnvelopeAction a) {
    switch (a) {
        case EnvelopeAction::Post: return "Post";
        case EnvelopeAction::Put: return "Put";
        case EnvelopeAction::Delete: return "Delete";
        case EnvelopeAction::Get: return "Get";
    }
    return "Post";
}

inline EnvelopeAction EnvelopeActionFromString(const std::string& s) {
    if (s == "Put") return EnvelopeAction::Put;
    if (s == "Delete") return EnvelopeAction::Delete;
    if (s == "Get") return EnvelopeAction::Get;
    return EnvelopeAction::Post;
}

namespace detail {
inline std::string RandomUuidLike() {
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

/// Wraps everything passed around the application so there is always a place
/// for header/routing metadata. Ports ra.common.Envelope (and folds in the
/// useful parts of the deprecated ra.common.DLC static helpers as methods).
/// Equality is by id. Move-only: it owns a polymorphic Message and a
/// polymorphic "current route" snapshot via unique_ptr.
class Envelope {
public:
    std::string id;
    route::DynamicRoutingSlip dynamic_routing_slip;
    std::unique_ptr<route::Route> route;
    std::vector<std::string> markers;
    identity::Did did;
    std::optional<std::string> client;
    bool reply_to_client = false;
    std::optional<std::string> client_reply_action;
    std::optional<std::string> url;
    std::optional<Multipart> multipart;
    std::optional<EnvelopeAction> action;
    std::optional<std::string> command_path;
    nlohmann::json headers = nlohmann::json::object();
    std::unique_ptr<messaging::Message> message;
    int sensitivity = 1;
    bool delayed = false;
    int min_delay = 0;
    int max_delay = 0;
    bool copy = false;
    int max_copies = 0;
    int min_copies = 0;
    service::ServiceLevel service_level = service::ServiceLevel::AtLeastOnce;

    explicit Envelope(std::optional<std::string> id_in = std::nullopt, std::unique_ptr<messaging::Message> message_in = nullptr)
        : id(id_in.value_or(detail::RandomUuidLike())), message(std::move(message_in)) {}

    Envelope(const Envelope&) = delete;
    Envelope& operator=(const Envelope&) = delete;
    Envelope(Envelope&&) = default;
    Envelope& operator=(Envelope&&) = default;

    static Envelope Command() { return Envelope(std::nullopt, std::make_unique<messaging::CommandMessage>()); }
    static Envelope Document() { return Envelope(std::nullopt, std::make_unique<messaging::DocumentMessage>()); }
    static Envelope DocumentWithId(const std::string& id) {
        return Envelope(id, std::make_unique<messaging::DocumentMessage>());
    }
    static Envelope HeadersOnly() { return Envelope(); }
    static Envelope Event(const std::string& event_type) {
        return Envelope(std::nullopt, std::make_unique<messaging::EventMessage>(messaging::EventMessage::Of(event_type)));
    }
    static Envelope Text() { return Envelope(std::nullopt, std::make_unique<messaging::TextMessage>()); }

    // ---- headers -------------------------------------------------

    void SetHeader(const std::string& name, nlohmann::json value) { headers[name] = std::move(value); }
    bool HeaderExists(const std::string& name) const { return headers.contains(name); }
    void RemoveHeader(const std::string& name) { headers.erase(name); }
    nlohmann::json Header(const std::string& name) const {
        return headers.contains(name) ? headers.at(name) : nlohmann::json();
    }

    std::optional<std::string> ContentType() const {
        if (!headers.contains(HeaderNames::kContentType)) return std::nullopt;
        const auto& v = headers.at(HeaderNames::kContentType);
        return v.is_string() ? std::optional<std::string>(v.get<std::string>()) : std::nullopt;
    }
    void SetContentType(const std::string& content_type) { headers[HeaderNames::kContentType] = content_type; }

    // ---- routing -----------------------------------------------

    /// Returns the current route, resolving it from the routing slip on first
    /// access (a clone - see the class comment on ownership).
    route::Route* GetRoute() {
        if (route == nullptr) {
            auto* current = dynamic_routing_slip.CurrentRoute();
            if (current != nullptr) route = current->Clone();
        }
        return route.get();
    }

    void Ratchet() {
        auto* next = dynamic_routing_slip.NextRoute();
        route = next != nullptr ? next->Clone() : nullptr;
    }

    void AddRoute(const std::string& service, const std::string& operation) {
        dynamic_routing_slip.AddRoute(std::make_unique<route::SimpleRoute>(route::SimpleRoute::Of(service, operation)));
    }

    void AddExternalRoute(const std::string& service, const std::string& operation) {
        dynamic_routing_slip.AddRoute(
            std::make_unique<route::SimpleExternalRoute>(route::SimpleExternalRoute::Of(service, operation)));
    }

    // ---- document payload accessors --------------------------

    messaging::DocumentMessage* Doc() { return dynamic_cast<messaging::DocumentMessage*>(message.get()); }
    const messaging::DocumentMessage* Doc() const { return dynamic_cast<const messaging::DocumentMessage*>(message.get()); }

    bool AddContent(nlohmann::json content) {
        auto* d = Doc();
        if (d == nullptr) return false;
        d->Put(messaging::kContent, std::move(content));
        return true;
    }
    nlohmann::json Content() const {
        auto* d = Doc();
        return d != nullptr ? d->Get(messaging::kContent) : nlohmann::json();
    }

    bool AddEntity(nlohmann::json entity) {
        auto* d = Doc();
        if (d == nullptr) return false;
        d->Put(messaging::kEntity, std::move(entity));
        return true;
    }
    nlohmann::json Entity() const {
        auto* d = Doc();
        return d != nullptr ? d->Get(messaging::kEntity) : nlohmann::json();
    }

    bool AddException(const std::string& msg) {
        auto* d = Doc();
        if (d == nullptr) return false;
        auto& bucket = d->Primary();
        if (bucket.contains(messaging::kExceptions) && bucket[messaging::kExceptions].is_array()) {
            bucket[messaging::kExceptions].push_back(msg);
        } else {
            bucket[messaging::kExceptions] = nlohmann::json::array({msg});
        }
        return true;
    }
    std::vector<std::string> Exceptions() const {
        auto* d = Doc();
        if (d == nullptr) return {};
        auto v = d->Get(messaging::kExceptions);
        return v.is_array() ? v.get<std::vector<std::string>>() : std::vector<std::string>();
    }

    void AddErrorMessage(const std::string& msg) {
        if (message) message->AddErrorMessage(msg);
    }
    std::vector<std::string> ErrorMessages() const { return message ? message->error_messages : std::vector<std::string>(); }

    bool AddNvp(const std::string& name, nlohmann::json value) {
        auto* d = Doc();
        if (d == nullptr) return false;
        d->Put(name, std::move(value));
        return true;
    }
    nlohmann::json Value(const std::string& name) const {
        auto* d = Doc();
        return d != nullptr ? d->Get(name) : nlohmann::json();
    }
    std::optional<nlohmann::json> Values() const {
        auto* d = Doc();
        return (d != nullptr && !d->data().empty()) ? std::optional<nlohmann::json>(d->data()[0]) : std::nullopt;
    }

    // ---- markers ---------------------------------------------

    bool MarkerPresent(const std::string& marker) const {
        return std::find(markers.begin(), markers.end(), marker) != markers.end();
    }
    void Mark(const std::string& marker) { markers.push_back(marker); }

    // ---- serialization ------------------------------------

    nlohmann::json ToJson() const {
        nlohmann::json obj = {
            {"id", id},
            {"dynamic_routing_slip", dynamic_routing_slip.ToJson()},
            {"markers", markers},
            {"did", did.ToJson()},
            {"reply_to_client", reply_to_client},
            {"headers", headers},
            {"sensitivity", sensitivity},
            {"delayed", delayed},
            {"min_delay", min_delay},
            {"max_delay", max_delay},
            {"copy", copy},
            {"max_copies", max_copies},
            {"min_copies", min_copies},
            {"service_level", service::ToString(service_level)},
        };
        if (route) obj["route"] = route->ToJson();
        if (client) obj["client"] = *client;
        if (client_reply_action) obj["client_reply_action"] = *client_reply_action;
        if (url) obj["url"] = *url;
        if (multipart) obj["multipart"] = multipart->ToJson();
        if (action) obj["action"] = ToString(*action);
        if (command_path) obj["command_path"] = *command_path;
        if (message) obj["message"] = message->ToJson();
        return obj;
    }

    static Envelope FromJson(const nlohmann::json& data) {
        Envelope env(data.contains("id") ? std::optional<std::string>(data.at("id").get<std::string>()) : std::nullopt);
        if (data.contains("dynamic_routing_slip")) {
            env.dynamic_routing_slip = route::DynamicRoutingSlip::FromJson(data.at("dynamic_routing_slip"));
        }
        if (data.contains("route")) env.route = route::RouteFromJson(data.at("route"));
        if (data.contains("markers")) env.markers = data.at("markers").get<std::vector<std::string>>();
        if (data.contains("did")) env.did = identity::Did::FromJson(data.at("did"));
        if (data.contains("client")) env.client = data.at("client").get<std::string>();
        env.reply_to_client = data.value("reply_to_client", false);
        if (data.contains("client_reply_action")) env.client_reply_action = data.at("client_reply_action").get<std::string>();
        if (data.contains("url")) env.url = data.at("url").get<std::string>();
        if (data.contains("multipart")) env.multipart = Multipart::FromJson(data.at("multipart"));
        if (data.contains("action")) env.action = EnvelopeActionFromString(data.at("action").get<std::string>());
        if (data.contains("command_path")) env.command_path = data.at("command_path").get<std::string>();
        if (data.contains("headers")) env.headers = data.at("headers");
        if (data.contains("message")) env.message = messaging::MessageFromJson(data.at("message"));
        env.sensitivity = data.value("sensitivity", 1);
        env.delayed = data.value("delayed", false);
        env.min_delay = data.value("min_delay", 0);
        env.max_delay = data.value("max_delay", 0);
        env.copy = data.value("copy", false);
        env.max_copies = data.value("max_copies", 0);
        env.min_copies = data.value("min_copies", 0);
        env.service_level =
            data.contains("service_level") ? service::ServiceLevelFromString(data.at("service_level").get<std::string>())
                                            : service::ServiceLevel::AtLeastOnce;
        return env;
    }

    std::string ToJsonString(int indent = 2) const { return ToJson().dump(indent); }

    static Envelope FromJsonString(const std::string& text) { return FromJson(nlohmann::json::parse(text)); }

    bool Equals(const Envelope& other) const { return id == other.id; }
};

}  // namespace ra::common
