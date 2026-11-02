#pragma once
#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string_view>

namespace ninmagic::http {
struct ContentRange { std::uint64_t start, end, total; };
inline auto ParseContentRange(std::string_view value) -> std::optional<ContentRange> {
    if (!value.starts_with("bytes ")) return {};
    value.remove_prefix(6);
    const auto dash = value.find('-');
    const auto slash = value.find('/');
    if (dash == value.npos || slash == value.npos || dash >= slash) return {};
    const auto number = [](std::string_view part, std::uint64_t& out) {
        if (part.empty()) return false;
        const auto result = std::from_chars(part.data(), part.data() + part.size(), out);
        return result.ec == std::errc{} && result.ptr == part.data() + part.size()
            && out <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    };
    ContentRange out{};
    if (!number(value.substr(0, dash), out.start) || !number(value.substr(dash + 1, slash - dash - 1), out.end)
        || !number(value.substr(slash + 1), out.total) || out.start > out.end || out.end >= out.total) return {};
    return out;
}
inline auto AcceptRange(long status, std::int64_t offset, const std::optional<ContentRange>& range) -> bool {
    if (offset < 0) return false;
    if (status == 200) return offset == 0 && !range;
    return status == 206 && range && range->start == static_cast<std::uint64_t>(offset);
}
}
