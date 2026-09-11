#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace ra::common::messaging {

class Email {
public:
    static constexpr const char* kMimetypeTextPlain = "text/plain";

    std::optional<std::string> to;
    std::optional<std::string> from;
    std::optional<std::string> subject;
    std::optional<std::string> message;
    std::optional<int64_t> id;
    std::string message_type = kMimetypeTextPlain;
    int flag = 0;

    static Email Anonymous(const std::string& to, const std::string& subject, const std::string& message) {
        Email e;
        e.to = to;
        e.subject = subject;
        e.message = message;
        return e;
    }
};

}  // namespace ra::common::messaging
