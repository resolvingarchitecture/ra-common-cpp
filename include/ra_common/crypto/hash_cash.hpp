#pragma once

#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "ra_common/crypto/sha.hpp"
#include "ra_common/exception.hpp"
#include "ra_common/util/random_util.hpp"

namespace ra::common::detail {
/// Exposed for testing, mirroring `leadingZeroBitsForTest` in ra-common-ts.
inline int LeadingZeroBits(const std::vector<uint8_t>& data) {
    int total = 0;
    for (uint8_t byte : data) {
        if (byte == 0) {
            total += 8;
        } else {
            total += 8 - std::bit_width(static_cast<unsigned>(byte));
            break;
        }
    }
    return total;
}
}  // namespace ra::common::detail

namespace ra::common::crypto {

/// Hashcash proof-of-work tokens. Ports ra.common.HashCash.
class HashCash {
public:
    static constexpr int kHashBits = 160;

    const std::string& token() const { return token_; }
    int value() const { return value_; }
    const std::string& resource() const { return resource_; }
    std::chrono::year_month_day date() const { return date_; }
    int version() const { return version_; }
    const std::map<std::string, std::vector<std::string>>& extensions() const { return extensions_; }

    static HashCash Mint(const std::string& resource, int bits) {
        auto today = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());
        return MintWith(resource, {}, std::chrono::year_month_day{today}, bits, 1);
    }

    static HashCash MintWith(const std::string& resource, std::map<std::string, std::vector<std::string>> extensions,
                              std::chrono::year_month_day date, int bits, int version) {
        if (version > 1) throw RaException::Invalid("only hashcash versions 0 and 1 are supported");
        if (bits > kHashBits) throw RaException::Invalid("value must be between 0 and 160");
        if (resource.find(':') != std::string::npos) throw RaException::Invalid("resource may not contain a colon");

        std::string ext_str = SerializeExtensions(extensions);
        std::string date_str = FmtYyMmDd(date);
        std::string prefix = version == 0 ? "0:" + date_str + ":" + resource + ":" + ext_str + ":"
                                           : "1:" + std::to_string(bits) + ":" + date_str + ":" + resource + ":" +
                                                 ext_str + ":";
        std::string token = Generate(prefix, bits);
        int value = version == 0 ? Sha1Bits(token) : bits;
        return HashCash(token, value, resource, date, version, std::move(extensions));
    }

    static HashCash Parse(const std::string& token) {
        auto parts = SplitOn(token, ':');
        int version;
        try {
            size_t consumed;
            version = std::stoi(parts.at(0), &consumed);
            if (consumed != parts[0].size()) throw std::invalid_argument("trailing chars");
        } catch (...) {
            throw RaException::Invalid("bad hashcash version");
        }
        int expected = version == 0 ? 6 : version == 1 ? 7 : -1;
        if (expected == -1) throw RaException::Invalid("only hashcash versions 0 and 1 are supported");
        if (static_cast<int>(parts.size()) != expected) throw RaException::Invalid("improperly formed hashcash");

        size_t idx = 1;
        int claimed_bits = 0;
        if (version == 1) {
            try {
                claimed_bits = std::stoi(parts.at(idx));
            } catch (...) {
                throw RaException::Invalid("bad hashcash bits");
            }
            idx++;
        }
        auto date = ParseYyMmDd(parts.at(idx));
        idx++;
        std::string resource = parts.at(idx);
        idx++;
        auto extensions = DeserializeExtensions(parts.at(idx));
        int actual = Sha1Bits(token);
        int value = version == 0 ? actual : std::min(actual, claimed_bits);
        return HashCash(token, value, resource, date, version, std::move(extensions));
    }

    int ComputedBits() const { return Sha1Bits(token_); }

    bool IsValidFor(const std::string& resource, int min_bits) const {
        return resource_ == resource && ComputedBits() >= min_bits;
    }

private:
    HashCash(std::string token, int value, std::string resource, std::chrono::year_month_day date, int version,
              std::map<std::string, std::vector<std::string>> extensions)
        : token_(std::move(token)),
          value_(value),
          resource_(std::move(resource)),
          date_(date),
          version_(version),
          extensions_(std::move(extensions)) {}

    static int Sha1Bits(const std::string& token) {
        auto digest = detail::Sha1(reinterpret_cast<const uint8_t*>(token.data()), token.size());
        return detail::LeadingZeroBits(digest);
    }

    static std::string Generate(const std::string& prefix, int bits) {
        auto rnd_bytes = RandomBytesOf(8);
        std::ostringstream rnd_os;
        for (auto b : rnd_bytes) {
            char buf[3];
            std::snprintf(buf, sizeof(buf), "%02x", b);
            rnd_os << buf;
        }
        uint64_t counter;
        auto counter_bytes = RandomBytesOf(8);
        std::memcpy(&counter, counter_bytes.data(), 8);
        std::string stem = prefix + rnd_os.str() + ":";
        for (;;) {
            counter++;
            std::ostringstream os;
            os << stem << std::hex << counter;
            std::string candidate = os.str();
            if (Sha1Bits(candidate) >= bits) return candidate;
        }
    }

    static std::vector<std::string> SplitOn(const std::string& s, char delim) {
        std::vector<std::string> parts;
        std::string cur;
        for (char c : s) {
            if (c == delim) {
                parts.push_back(cur);
                cur.clear();
            } else {
                cur += c;
            }
        }
        parts.push_back(cur);
        return parts;
    }

    static std::string SerializeExtensions(const std::map<std::string, std::vector<std::string>>& ext) {
        if (ext.empty()) return "";
        std::vector<std::string> parts;
        for (const auto& [key, values] : ext) {
            if (key.find_first_of(":;=") != std::string::npos) {
                throw RaException::Invalid("illegal char in extension key: " + key);
            }
            if (values.empty()) {
                parts.push_back(key);
                continue;
            }
            for (const auto& v : values) {
                if (v.find_first_of(":;,") != std::string::npos) {
                    throw RaException::Invalid("illegal char in extension value: " + v);
                }
            }
            std::string joined;
            for (size_t i = 0; i < values.size(); i++) joined += (i == 0 ? "" : ",") + values[i];
            parts.push_back(key + "=" + joined);
        }
        std::string out;
        for (size_t i = 0; i < parts.size(); i++) out += (i == 0 ? "" : ";") + parts[i];
        return out;
    }

    static std::map<std::string, std::vector<std::string>> DeserializeExtensions(const std::string& text) {
        std::map<std::string, std::vector<std::string>> out;
        if (text.empty()) return out;
        for (const auto& item : SplitOn(text, ';')) {
            auto eq = item.find('=');
            if (eq == std::string::npos) {
                out[item] = {};
            } else {
                out[item.substr(0, eq)] = SplitOn(item.substr(eq + 1), ',');
            }
        }
        return out;
    }

    static std::string FmtYyMmDd(std::chrono::year_month_day d) {
        int yy = static_cast<int>(d.year()) % 100;
        unsigned mm = static_cast<unsigned>(d.month());
        unsigned dd = static_cast<unsigned>(d.day());
        char buf[7];
        std::snprintf(buf, sizeof(buf), "%02d%02u%02u", yy, mm, dd);
        return buf;
    }

    static std::chrono::year_month_day ParseYyMmDd(const std::string& s) {
        if (s.size() != 6 || !std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; })) {
            throw RaException::Invalid("bad hashcash date: " + s);
        }
        int yy = std::stoi(s.substr(0, 2));
        int mm = std::stoi(s.substr(2, 2));
        int dd = std::stoi(s.substr(4, 2));
        std::chrono::year_month_day date{std::chrono::year{2000 + yy}, std::chrono::month{static_cast<unsigned>(mm)},
                                          std::chrono::day{static_cast<unsigned>(dd)}};
        if (!date.ok()) throw RaException::Invalid("bad hashcash date: " + s);
        return date;
    }

    std::string token_;
    int value_;
    std::string resource_;
    std::chrono::year_month_day date_;
    int version_;
    std::map<std::string, std::vector<std::string>> extensions_;
};

}  // namespace ra::common::crypto
