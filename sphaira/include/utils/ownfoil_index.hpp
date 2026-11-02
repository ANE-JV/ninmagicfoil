#pragma once

// Plain Tinfoil shop indices. This parser has no console or network dependency.
#include <yyjson.h>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <utility>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace sphaira::ownfoil::index {
enum class Kind { Base, Update, Dlc, Unknown };
struct File {
    std::string url, name, id, title_id, publisher, icon, banner, extension, sha256;
    int64_t version{};
    int64_t size{};
    Kind kind{Kind::Unknown};
    bool directory{};
};
inline auto String(yyjson_val* obj, const char* key) -> std::string {
    const auto value = yyjson_obj_get(obj, key);
    return yyjson_is_str(value) ? yyjson_get_str(value) : "";
}
inline auto Integer(yyjson_val* obj, const char* key, int64_t fallback = 0) -> int64_t {
    const auto value = yyjson_obj_get(obj, key);
    if (yyjson_is_int(value)) return yyjson_get_sint(value);
    if (yyjson_is_str(value)) {
        int64_t number{};
        const auto text = std::string_view{yyjson_get_str(value)};
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), number);
        if (parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()) return number;
    }
    return fallback;
}
inline auto Lower(std::string text) -> std::string {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}
inline auto HexDigit(char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
inline auto Decode(std::string_view text) -> std::string {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size() && HexDigit(text[i+1]) >= 0 && HexDigit(text[i+2]) >= 0) {
            const auto c = static_cast<char>(HexDigit(text[i+1]) * 16 + HexDigit(text[i+2]));
            if (static_cast<unsigned char>(c) >= 32) out += c;
            i += 2;
        } else if (static_cast<unsigned char>(text[i]) >= 32) out += text[i];
    }
    return out;
}
inline auto Id(std::string_view text) -> std::string {
    for (size_t i = 0; i + 16 <= text.size(); ++i) {
        if (i && HexDigit(text[i-1]) >= 0) continue;
        if (i + 16 < text.size() && HexDigit(text[i+16]) >= 0) continue;
        const auto part = text.substr(i, 16);
        if (!std::all_of(part.begin(), part.end(), [](char c) { return HexDigit(c) >= 0; })) continue;
        uint64_t numeric{};
        std::from_chars(part.data(), part.data() + part.size(), numeric, 16);
        // Switch application IDs, not arbitrary hexadecimal hashes.
        if ((numeric >> 56) != 1) continue;
        auto out = std::string(part);
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::toupper(c); });
        return out;
    }
    return {};
}
inline auto Extension(std::string text) -> std::string {
    text = Lower(text.substr(0, text.find_first_of("?#")));
    const auto dot = text.rfind('.');
    if (dot == std::string::npos) return {};
    auto ext = text.substr(dot + 1);
    return ext == "nsp" || ext == "nsz" || ext == "xci" || ext == "xcz" ? ext : "";
}
inline auto CleanName(std::string text) -> std::string {
    if (const auto dot = text.rfind('.'); dot != std::string::npos && !Extension(text).empty()) text.erase(dot);
    for (size_t pos = 0; (pos = text.find('[', pos)) != std::string::npos;) {
        const auto end = text.find(']', pos);
        if (end == std::string::npos) break;
        const auto token = text.substr(pos + 1, end - pos - 1);
        const auto version = token.size() > 1 && (token[0] == 'v' || token[0] == 'V')
            && std::all_of(token.begin() + 1, token.end(), [](unsigned char c) { return std::isdigit(c); });
        if ((token.size() == 16 && !Id(token).empty()) || version) text.erase(pos, end - pos + 1);
        else pos = end + 1;
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) text.pop_back();
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) text.erase(0, 1);
    return text;
}
inline auto Parse(const char* bytes, size_t size, std::vector<File>& out, std::string& error) -> bool {
    out.clear();
    yyjson_doc* doc = yyjson_read(bytes, size, YYJSON_READ_NOFLAG);
    if (!doc) { error = "The server did not return a plain JSON shop index"; return false; }
    const auto root = yyjson_doc_get_root(doc);
    const auto files = yyjson_obj_get(root, "files");
    if (!yyjson_is_arr(files)) {
        error = "The JSON index has no files array (encrypted indices are not supported)";
        yyjson_doc_free(doc);
        return false;
    }
    const auto titledb = yyjson_obj_get(root, "titledb");
    size_t i, max;
    yyjson_val* item;
    yyjson_arr_foreach(files, i, max, item) {
        File file;
        file.url = yyjson_is_str(item) ? yyjson_get_str(item) : String(item, "url");
        if (file.url.empty()) continue;
        const auto fragment = file.url.find('#');
        auto filename = fragment == std::string::npos ? file.url.substr(0, file.url.find('?')) : file.url.substr(fragment + 1);
        if (fragment == std::string::npos) filename = filename.substr(filename.find_last_of('/') + 1);
        filename = Decode(filename);
        file.url = file.url.substr(0, fragment);
        file.extension = Extension(filename);
        if (file.extension.empty()) file.extension = Extension(file.url);
        if (file.extension.empty()) continue;
        file.id = Id(String(item, "titleId"));
        if (file.id.empty()) file.id = Id(String(item, "title_id"));
        if (file.id.empty()) file.id = Id(String(item, "id"));
        if (file.id.empty()) file.id = Id(filename);
        file.version = Integer(item, "version", -1);
        if (file.version < 0) {
            const auto lower = Lower(filename);
            const auto v = lower.find("[v");
            file.version = 0;
            if (v != std::string::npos) {
                const auto end = lower.find(']', v + 2);
                if (end != std::string::npos) {
                    int64_t number{};
                    const auto parsed = std::from_chars(lower.data() + v + 2, lower.data() + end, number);
                    if (parsed.ec == std::errc{} && parsed.ptr == lower.data() + end && number >= 0) file.version = number;
                }
            }
        }
        file.size = std::max<int64_t>(0, Integer(item, "size"));
        file.sha256 = Lower(String(item, "sha256"));
        if (file.sha256.size() != 64 || !std::all_of(file.sha256.begin(), file.sha256.end(), [](char c) { return HexDigit(c) >= 0; })) file.sha256.clear();
        if (!file.id.empty()) {
            uint64_t id{};
            std::from_chars(file.id.data(), file.id.data() + file.id.size(), id, 16);
            const auto suffix = id & 0xfff;
            file.kind = suffix == 0 ? Kind::Base : suffix == 0x800 ? Kind::Update : Kind::Dlc;
            const auto title = file.kind == Kind::Dlc ? (id - 0x1000) & ~uint64_t(0xfff) : id & ~uint64_t(0xfff);
            char title_text[17]{};
            std::snprintf(title_text, sizeof(title_text), "%016llX", static_cast<unsigned long long>(title));
            file.title_id = title_text;
        }
        const auto metadata = yyjson_obj_get(titledb, file.id.c_str());
        file.name = String(metadata, "name");
        if (file.name.empty()) file.name = String(item, "name");
        if (file.name.empty()) file.name = CleanName(filename);
        if (file.name.empty()) file.name = file.id.empty() ? "Unnamed package" : file.id;
        file.publisher = String(metadata, "publisher");
        file.icon = String(metadata, "iconUrl");
        if (file.icon.empty()) file.icon = String(metadata, "icon");
        file.banner = String(metadata, "bannerUrl");
        out.emplace_back(std::move(file));
    }
    yyjson_doc_free(doc);
    return true;
}

inline auto HtmlText(std::string text) -> std::string {
    for (const auto& [entity, replacement] : {std::pair{"&amp;", "&"}, {"&quot;", "\""}, {"&#39;", "'"}, {"&lt;", "<"}, {"&gt;", ">"}}) {
        size_t pos{};
        while ((pos = text.find(entity, pos)) != std::string::npos) {
            text.replace(pos, std::strlen(entity), replacement);
            pos += std::strlen(replacement);
        }
    }
    return text;
}
inline auto JsonQuote(const std::string& text) -> std::string {
    std::string out = "\"";
    for (const auto c : text) {
        if (c == '\\' || c == '"') out += '\\';
        if (static_cast<unsigned char>(c) >= 32) out += c;
    }
    return out + '"';
}
// Ownfoil's older Sphaira route is a list of hrefs. It can contain thousands of
// subdirectories; expose those for lazy navigation rather than crawling a NAS.
inline auto ParseHtml(const char* bytes, size_t size, std::vector<File>& out, std::string& error) -> bool {
    const std::string text(bytes, size);
    const auto lower = Lower(text);
    std::string json = "{\"files\":[";
    std::vector<File> folders;
    bool seen{};
    for (size_t pos = 0; (pos = lower.find("<a ", pos)) != std::string::npos;) {
        const auto end = lower.find('>', pos);
        const auto attr = lower.find("href", pos + 3);
        if (end == std::string::npos) break;
        pos = end + 1;
        if (attr == std::string::npos || attr >= end) continue;
        auto begin = lower.find('=', attr + 4);
        if (begin == std::string::npos || begin >= end) continue;
        do { ++begin; } while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])));
        if (begin >= end || (text[begin] != '\'' && text[begin] != '"')) continue;
        const auto finish = text.find(text[begin], begin + 1);
        if (finish == std::string::npos || finish > end) continue;
        auto href = HtmlText(text.substr(begin + 1, finish - begin - 1));
        if (href.starts_with("00 - ERROR")) {
            error = "The server rejected the login or this account has no shop access";
            out.clear(); return false;
        }
        if (href.empty() || href == "../" || href == "./" || href == "/") continue;
        seen = true;
        if (href.ends_with('/')) {
            File folder;
            folder.url = href;
            folder.name = Decode(href.substr(0, href.size() - 1));
            folder.name = folder.name.substr(folder.name.find_last_of('/') + 1);
            folder.directory = true;
            folders.push_back(std::move(folder));
        } else {
            if (json.back() != '[') json += ',';
            json += "{\"url\":" + JsonQuote(href) + '}';
        }
    }
    if (!seen && lower.find("<table>") == std::string::npos) {
        error = "The server returned a web page instead of a shop directory";
        return false;
    }
    json += "]}";
    if (!Parse(json.data(), json.size(), out, error)) return false;
    out.insert(out.end(), std::make_move_iterator(folders.begin()), std::make_move_iterator(folders.end()));
    return true;
}
} // namespace sphaira::ownfoil::index
