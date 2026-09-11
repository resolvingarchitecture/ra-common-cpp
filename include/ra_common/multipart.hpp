#pragma once

#include <optional>
#include <string>

#include "nlohmann/json.hpp"
#include "ra_common/util/random_util.hpp"

// Ports ra.common.file.Multipart - a multipart/form-data body builder. Like
// the Java version (whose HTTP transport was commented out) this only
// accumulates the body string; sending it is the caller's concern.

namespace ra::common {

class Multipart {
public:
    explicit Multipart(std::optional<std::string> charset = std::nullopt, std::optional<std::string> boundary = std::nullopt)
        : boundary_(boundary.value_or(RandomAlphanumeric(32))), charset_(std::move(charset)) {}

    const std::string& boundary() const { return boundary_; }
    const std::optional<std::string>& charset() const { return charset_; }

    void AddFormField(const std::string& name, const std::string& value) {
        std::string charset = charset_.value_or("UTF-8");
        body_ += "--" + boundary_ + kLineFeed;
        body_ += "Content-Disposition: form-data; name=\"" + name + "\"" + kLineFeed;
        body_ += "Content-Type: text/plain; charset=" + charset + kLineFeed + kLineFeed;
        body_ += value + kLineFeed;
    }

    void AddFilePart(const std::string& field_name, std::optional<std::string> file_name = std::nullopt) {
        body_ += "--" + boundary_ + kLineFeed;
        body_ += file_name ? "Content-Disposition: file; filename=\"" + *file_name + "\"" + kLineFeed
                            : "Content-Disposition: file; name=\"" + field_name + "\";" + kLineFeed;
        body_ += std::string("Content-Type: application/octet-stream") + kLineFeed;
        body_ += std::string("Content-Transfer-Encoding: binary") + kLineFeed + kLineFeed;
    }

    void AddHeaderField(const std::string& name, const std::string& value) { body_ += name + ": " + value + kLineFeed; }

    void AppendRaw(const std::string& text) { body_ += text; }

    const std::string& body() const { return body_; }

    std::string Finish() const { return body_ + "--" + boundary_ + "--" + kLineFeed; }

    nlohmann::json ToJson() const {
        nlohmann::json obj = {{"boundary", boundary_}};
        if (charset_) obj["charset"] = *charset_;
        return obj;
    }

    static Multipart FromJson(const nlohmann::json& data) {
        return Multipart(data.contains("charset") ? std::optional<std::string>(data.at("charset").get<std::string>())
                                                    : std::nullopt,
                          data.at("boundary").get<std::string>());
    }

private:
    static constexpr const char* kLineFeed = "\r\n";
    std::string boundary_;
    std::optional<std::string> charset_;
    std::string body_;
};

}  // namespace ra::common
