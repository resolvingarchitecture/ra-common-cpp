#pragma once

#include <optional>
#include <string>

namespace ra::common::crypto {

class Addressable {
public:
    virtual ~Addressable() = default;
    virtual std::optional<std::string> fingerprint() const = 0;
    virtual std::optional<std::string> address() const = 0;
};

}  // namespace ra::common::crypto
