#pragma once

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>  // declares `environ` on POSIX (see DESIGN.md - POSIX-only)

#include "ra_common/exception.hpp"

// Configuration loading and cross-platform (POSIX-only, see DESIGN.md)
// directory resolution. Ports ra.common.Config and ra.common.SystemSettings.

namespace ra::common {

inline std::map<std::string, std::string> LoadFromArgs(const std::vector<std::string>& args, char delimiter = '=') {
    std::map<std::string, std::string> out;
    for (const auto& arg : args) {
        auto idx = arg.find(delimiter);
        if (idx != std::string::npos && idx > 0) out[arg.substr(0, idx)] = arg.substr(idx + 1);
    }
    return out;
}

inline std::map<std::string, std::string> LoadFromEnv() {
    std::map<std::string, std::string> out;
    for (char** e = environ; *e != nullptr; e++) {
        std::string entry(*e);
        auto idx = entry.find('=');
        if (idx != std::string::npos) out[entry.substr(0, idx)] = entry.substr(idx + 1);
    }
    return out;
}

inline std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

inline std::map<std::string, std::string> ParseProperties(const std::string& text) {
    std::map<std::string, std::string> out;
    std::istringstream stream(text);
    std::string raw;
    while (std::getline(stream, raw)) {
        std::string line = Trim(raw);
        if (line.empty() || line[0] == '#' || line[0] == '!') continue;
        for (char sep : {'=', ':'}) {
            auto idx = line.find(sep);
            if (idx != std::string::npos) {
                out[Trim(line.substr(0, idx))] = Trim(line.substr(idx + 1));
                break;
            }
        }
    }
    return out;
}

inline std::map<std::string, std::string> LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw RaException(RaErrorKind::Io, "could not open " + path);
    std::ostringstream ss;
    ss << file.rdbuf();
    return ParseProperties(ss.str());
}

inline std::map<std::string, std::string> LoadAll(const std::map<std::string, std::string>& client_props,
                                                    std::optional<std::string> config_path = std::nullopt) {
    auto config = LoadFromEnv();
    if (config_path) {
        for (auto& [k, v] : LoadFromFile(*config_path)) config[k] = v;
    }
    for (auto& [k, v] : client_props) config[k] = v;
    return config;
}

/// XDG-style directory resolution. POSIX-only (reads $HOME directly, no
/// Windows %USERPROFILE% fallback) - see DESIGN.md.
namespace system_settings {

inline std::optional<std::string> UserHomeDir() {
    const char* home = std::getenv("HOME");
    return home != nullptr ? std::optional<std::string>(home) : std::nullopt;
}

inline std::optional<std::string> XdgDir(const char* env_key, const std::string& default_suffix) {
    const char* value = std::getenv(env_key);
    if (value != nullptr && value[0] != '\0') return std::string(value);
    auto home = UserHomeDir();
    return home ? std::optional<std::string>(*home + "/" + default_suffix) : std::nullopt;
}

inline std::optional<std::string> UserDataDir() { return XdgDir("XDG_DATA_HOME", ".local/share"); }
inline std::optional<std::string> UserConfigDir() { return XdgDir("XDG_CONFIG_HOME", ".config"); }
inline std::optional<std::string> UserCacheDir() { return XdgDir("XDG_CACHE_HOME", ".cache"); }

inline std::string AppDir(const std::string& base, const std::string& group, const std::string& app, bool create = false) {
    std::string dir = base + "/" + group + "/" + app;
    if (create) {
        // mkdir -p equivalent without shelling out: create each path segment.
        std::string accum = base;
        for (const auto& seg : {group, app}) {
            accum += "/" + seg;
            if (::mkdir(accum.c_str(), 0755) != 0 && errno != EEXIST) {
                throw RaException(RaErrorKind::FileCreationFailed, dir + ": could not create " + accum);
            }
        }
    }
    return dir;
}

inline std::string UserAppDataDir(const std::string& group, const std::string& app, bool create = false) {
    auto base = UserDataDir();
    if (!base) throw RaException(RaErrorKind::FileCreationFailed, "no user data dir");
    return AppDir(*base, group, app, create);
}

inline std::string UserAppConfigDir(const std::string& group, const std::string& app, bool create = false) {
    auto base = UserConfigDir();
    if (!base) throw RaException(RaErrorKind::FileCreationFailed, "no user config dir");
    return AppDir(*base, group, app, create);
}

inline std::string UserAppCacheDir(const std::string& group, const std::string& app, bool create = false) {
    auto base = UserCacheDir();
    if (!base) throw RaException(RaErrorKind::FileCreationFailed, "no user cache dir");
    return AppDir(*base, group, app, create);
}

}  // namespace system_settings

}  // namespace ra::common
