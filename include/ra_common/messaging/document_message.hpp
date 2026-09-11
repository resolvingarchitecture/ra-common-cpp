#pragma once

#include <vector>

#include "nlohmann/json.hpp"
#include "ra_common/messaging/message.hpp"

namespace ra::common::messaging {

class DocumentMessage : public Message {
public:
    explicit DocumentMessage(std::vector<nlohmann::json> data = {nlohmann::json::object()}) : data_(std::move(data)) {}

    std::string Kind() const override { return "document"; }

    std::vector<nlohmann::json>& data() { return data_; }
    const std::vector<nlohmann::json>& data() const { return data_; }

    nlohmann::json& Primary() {
        if (data_.empty()) data_.push_back(nlohmann::json::object());
        return data_[0];
    }

    nlohmann::json Get(const std::string& key) const {
        return (!data_.empty() && data_[0].contains(key)) ? data_[0].at(key) : nlohmann::json();
    }

    void Put(const std::string& key, nlohmann::json value) { Primary()[key] = std::move(value); }

    nlohmann::json ToJson() const override { return WithErrors({{"kind", Kind()}, {"data", data_}}); }

    static DocumentMessage FromJson(const nlohmann::json& data) {
        auto msg = DocumentMessage(data.contains("data") ? data.at("data").get<std::vector<nlohmann::json>>()
                                                           : std::vector<nlohmann::json>{nlohmann::json::object()});
        msg.error_messages = ErrorsFromJson(data);
        return msg;
    }

private:
    std::vector<nlohmann::json> data_;
};

}  // namespace ra::common::messaging
