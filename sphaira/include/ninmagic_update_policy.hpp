#pragma once

#include "ninmagic_branding.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace ninmagic::update {
inline constexpr std::uint64_t MaxAssetSize = 256ULL * 1024 * 1024;
inline constexpr std::uint64_t MaxMetadataSize = 1024 * 1024;
using VersionParts = std::array<std::uint32_t, 3>;

// Release tags may have a leading v; NACP versions must use the plain form.
inline auto ParseVersion(std::string_view text, bool allow_prefix = true) -> std::optional<VersionParts> {
    if (allow_prefix && text.starts_with('v')) text.remove_prefix(1);
    VersionParts out{};
    for (std::size_t i = 0; i < out.size(); ++i) {
        const auto dot = text.find('.');
        const auto part = text.substr(0, dot);
        if (part.empty() || part.size() > 5 || (part.size() > 1 && part.front() == '0')) return {};
        for (const char c : part) {
            if (c < '0' || c > '9') return {};
            out[i] = out[i] * 10 + static_cast<unsigned>(c - '0');
        }
        if (out[i] > 65535) return {};
        if (i + 1 == out.size()) {
            if (dot != std::string_view::npos) return {};
        } else {
            if (dot == std::string_view::npos) return {};
            text.remove_prefix(dot + 1);
        }
    }
    return out;
}

inline auto ParseDigest(std::string_view value) -> std::optional<std::string> {
    if (!value.starts_with("sha256:") || value.size() != 71) return {};
    value.remove_prefix(7);
    std::string out;
    out.reserve(64);
    for (const char c : value) {
        if (c >= '0' && c <= '9') out.push_back(c);
        else if (c >= 'a' && c <= 'f') out.push_back(c);
        else if (c >= 'A' && c <= 'F') out.push_back(static_cast<char>(c + ('a' - 'A')));
        else return {};
    }
    return out;
}

inline auto IsAssetUrl(std::string_view url, std::string_view tag) -> bool {
    if (!ParseVersion(tag)) return false;
    return url == "https://github.com/ANE-JV/ninmagicfoil/releases/download/" + std::string{tag} + "/" + ReleaseAsset;
}

inline auto IsDownloadHost(std::string_view url) -> bool {
    if (!url.starts_with("https://") || url.size() > 8192) return false;
    for (const unsigned char c : url) {
        if (c <= 32 || c == 127 || c == '\\' || c == '#') return false;
    }
    const auto end = url.find('/', 8);
    if (end == std::string_view::npos) return false;
    const auto host = url.substr(8, end - 8);
    return host == "github.com" || host == "release-assets.githubusercontent.com" || host == "objects.githubusercontent.com";
}

inline auto IsInstallPath(std::string_view path) -> bool {
    if (path.starts_with("sdmc:")) path.remove_prefix(5);
    return path == InstallPath;
}

inline auto SectionFits(std::uint64_t total, std::uint64_t offset, std::uint64_t size) -> bool {
    return offset <= total && size <= total - offset;
}

struct Release {
    std::string tag;
    std::string url;
    std::string sha256;
    std::uint64_t size{};
};

inline auto IsValidRelease(const Release& release) -> bool {
    return release.size > 0 && release.size <= MaxAssetSize
        && IsAssetUrl(release.url, release.tag)
        && ParseDigest("sha256:" + release.sha256).has_value();
}
}
