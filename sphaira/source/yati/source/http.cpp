#include "yati/source/http.hpp"
#include "utils/devoptab_common.hpp"
#include "download.hpp"
#include "defines.hpp"
#include "log.hpp"
#include "ninmagic_tls.hpp"

#include <cstdio>
#include <charconv>
#include <limits>
#include <optional>
#include <algorithm>
#include <cctype>

namespace sphaira::yati::source {
namespace {

using devoptab::common::PushThreadData;

constexpr long CONNECT_TIMEOUT_MS = 10000;
// retry transfers that stop sending data for this long.
constexpr long LOW_SPEED_TIME_S = 15;
// a transfer that ends early is started again where it stopped, for as long as
// this since anything last arrived: a console that drops off its network needs
// that long to rejoin it, and while it is away each attempt fails at once.
constexpr u64 RESUME_WINDOW_NS = 60'000'000'000ULL;
constexpr u64 RETRY_WAIT_NS = 2'000'000'000ULL;

auto HttpOrigin(const std::string& url, const std::string& relative = {}) -> std::optional<std::string> {
    auto parsed = curl_url();
    if (!parsed) return std::nullopt;
    ON_SCOPE_EXIT(curl_url_cleanup(parsed));
    if (curl_url_set(parsed, CURLUPART_URL, url.c_str(), CURLU_DEFAULT_SCHEME) != CURLUE_OK) return std::nullopt;
    if (!relative.empty() && curl_url_set(parsed, CURLUPART_URL, relative.c_str(), 0) != CURLUE_OK) return std::nullopt;
    char* scheme{};
    char* host{};
    char* port{};
    ON_SCOPE_EXIT({ curl_free(scheme); curl_free(host); curl_free(port); });
    if (curl_url_get(parsed, CURLUPART_SCHEME, &scheme, 0) != CURLUE_OK
        || curl_url_get(parsed, CURLUPART_HOST, &host, 0) != CURLUE_OK
        || curl_url_get(parsed, CURLUPART_PORT, &port, CURLU_DEFAULT_PORT) != CURLUE_OK) return std::nullopt;
    if (strcasecmp(scheme, "http") && strcasecmp(scheme, "https")) return std::nullopt;
    std::string origin = std::string{scheme} + "://" + host + ":" + port;
    std::ranges::transform(origin, origin.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return origin;
}

} // namespace

Http::Http(const std::string& url, const std::string& user, const std::string& pass, std::stop_token token, bool legacy_ownfoil)
: m_url{url}, m_user{user}, m_pass{pass}, m_token{token}, m_legacy_ownfoil{legacy_ownfoil} {
    m_curl = curl_easy_init();
    if (!m_curl) {
        m_open_result = Result_CurlFailedEasyInit;
    }
}

Http::~Http() {
    // the transfer's thread runs on the handle, so it is joined first.
    m_transfer.reset();
    if (m_curl) {
        curl_easy_cleanup(m_curl);
    }
}

Result Http::Start(s64 off) {
    curl_easy_reset(m_curl);
    curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
    if (!m_legacy_ownfoil) curl_easy_setopt(m_curl, CURLOPT_USERAGENT, curl::USER_AGENT);
    R_UNLESS(off >= 0 && ninmagic::tls::ConfigureCurlTrust(m_curl), Result_YatiHttpReadFailed);
    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(m_curl, CURLOPT_UNRESTRICTED_AUTH, 0L);
    const bool https = !strncasecmp(m_url.c_str(), "https://", 8);
#if LIBCURL_VERSION_NUM >= 0x075500
    R_UNLESS(curl_easy_setopt(m_curl, CURLOPT_PROTOCOLS_STR, "http,https") == CURLE_OK, Result_YatiHttpReadFailed);
    R_UNLESS(curl_easy_setopt(m_curl, CURLOPT_REDIR_PROTOCOLS_STR, https ? "https" : "http,https") == CURLE_OK, Result_YatiHttpReadFailed);
#else
    R_UNLESS(curl_easy_setopt(m_curl, CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS)) == CURLE_OK, Result_YatiHttpReadFailed);
    R_UNLESS(curl_easy_setopt(m_curl, CURLOPT_REDIR_PROTOCOLS, static_cast<long>(https ? CURLPROTO_HTTPS : CURLPROTO_HTTP | CURLPROTO_HTTPS)) == CURLE_OK, Result_YatiHttpReadFailed);
#endif
    curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "identity");
    curl_easy_setopt(m_curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(m_curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(m_curl, CURLOPT_BUFFERSIZE, 1024L * 64L);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, CONNECT_TIMEOUT_MS);
    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    curl_easy_setopt(m_curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME_S);

    if (!m_user.empty()) {
        curl_easy_setopt(m_curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);
        curl_easy_setopt(m_curl, CURLOPT_USERNAME, m_user.c_str());
        curl_easy_setopt(m_curl, CURLOPT_PASSWORD, m_pass.c_str());
    }

    // open ended: the transfer only stops early when a read goes elsewhere.
    char range[32];
    std::snprintf(range, sizeof(range), "%ld-", off);
    curl_easy_setopt(m_curl, CURLOPT_RANGE, range);

    m_requested_offset = off;
    m_header_status = 0;
    m_content_range.reset();
    m_body_allowed = false;
    m_protocol_error = false;
    m_transfer = std::make_unique<PushThreadData>(m_curl, m_token);
    curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);

    if (const auto rc = m_transfer->CreateAndStart(); R_FAILED(rc)) {
        m_transfer.reset();
        return rc;
    }

    m_offset = off;
    R_SUCCEED();
}

size_t Http::HeaderCallback(char* data, size_t size, size_t count, void* opaque) {
    if (size && count > std::numeric_limits<size_t>::max() / size) return 0;
    const auto bytes = size * count;
    auto& self = *static_cast<Http*>(opaque);
    const std::string_view line{data, bytes};
    if (line.starts_with("HTTP/")) {
        self.m_header_status = 0;
        self.m_content_range.reset();
        self.m_body_allowed = false;
        const auto space = line.find(' ');
        if (space != line.npos && line.size() >= space + 4) {
            const auto parsed = std::from_chars(line.data() + space + 1, line.data() + space + 4, self.m_header_status);
            if (parsed.ec != std::errc{}) return 0;
        }
        return bytes;
    }
    if (line == "\r\n" || line == "\n") {
        if (self.m_header_status >= 200 && self.m_header_status < 300) {
            self.m_body_allowed = ninmagic::http::AcceptRange(self.m_header_status, self.m_requested_offset, self.m_content_range);
            if (!self.m_body_allowed) {
                self.m_protocol_error = true;
                log_write("[HTTP] server returned wrong range/status for offset %ld\n", self.m_requested_offset);
                return 0;
            }
        }
        return bytes;
    }
    const auto colon = line.find(':');
    if (colon == line.npos) return bytes;
    const std::string key{line.substr(0, colon)};
    auto value = line.substr(colon + 1);
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t' || value.back() == '\r' || value.back() == '\n')) value.remove_suffix(1);
    if (!strcasecmp(key.c_str(), "Content-Range")) {
        self.m_content_range = ninmagic::http::ParseContentRange(value);
        if (!self.m_content_range) { self.m_protocol_error = true; return 0; }
    } else if (!strcasecmp(key.c_str(), "Content-Encoding") && value != "identity") {
        // NSP offsets describe the original file, never a compressed HTTP body.
        self.m_protocol_error = true;
        return 0;
    } else if (!strcasecmp(key.c_str(), "Location") && (!self.m_user.empty() || !self.m_pass.empty() || self.m_url.find('@') != std::string::npos)) {
        const auto before = HttpOrigin(self.m_url);
        const auto after = HttpOrigin(self.m_url, std::string{value});
        if (!before || !after || *before != *after) { self.m_protocol_error = true; return 0; }
    }
    return bytes;
}

size_t Http::WriteCallback(char* data, size_t size, size_t count, void* opaque) {
    if (size && count > std::numeric_limits<size_t>::max() / size) return 0;
    auto& self = *static_cast<Http*>(opaque);
    if (self.m_header_status >= 300 && self.m_header_status < 400) return size * count;
    if (!self.m_body_allowed) { self.m_protocol_error = true; return 0; }
    return PushThreadData::push_thread_callback(data, size, count, self.m_transfer.get());
}

Result Http::Read(void* buf, s64 off, s64 size, u64* bytes_read) {
    R_TRY(GetOpenResult());
    *bytes_read = 0;
    R_UNLESS(off >= 0 && size >= 0 && off <= std::numeric_limits<s64>::max() - size, Result_YatiHttpReadFailed);
    if (!size) R_SUCCEED();

    UEvent cancelled;
    ueventCreate(&cancelled, false);
    std::stop_callback on_stop{m_token, [&cancelled]{ ueventSignal(&cancelled); }};

    if (m_transfer && off != m_offset) {
        log_write("[HTTP] read moved from %ld to %ld, restarting transfer\n", m_offset, off);
        m_transfer.reset();
    }

    // every read asks for bytes inside the file, so coming up short is the
    // transfer failing rather than the file ending.
    for (u64 stalled_since = 0;;) {
        R_UNLESS(!m_token.stop_requested(), Result_TransferCancelled);
        if (!m_transfer) {
            R_TRY(Start(off + *bytes_read));
        }

        // blocks until all of it has arrived, or the transfer has ended.
        const auto read = m_transfer->PullData(static_cast<char*>(buf) + *bytes_read, size - *bytes_read);
        m_offset += read;
        *bytes_read += read;
        R_UNLESS(!m_token.stop_requested(), Result_TransferCancelled);

        if (static_cast<s64>(*bytes_read) == size) {
            R_SUCCEED();
        }

        const auto code = m_transfer->code;
        log_write("[HTTP] read %lu of %ld at %ld, http code: %ld error: %d\n", *bytes_read, size, off, code, m_transfer->error);
        m_transfer.reset();

        // asking again won't change an error status (credentials, a token gone).
        R_UNLESS(code < 400 && !m_protocol_error, Result_YatiHttpReadFailed);
        const auto now = armTicksToNs(armGetSystemTick());
        if (read || !stalled_since) {
            stalled_since = now;
        }
        R_UNLESS(now - stalled_since < RESUME_WINDOW_NS, Result_YatiHttpReadFailed);

        log_write("[HTTP] resuming at %ld\n", m_offset);
        waitSingle(waiterForUEvent(&cancelled), RETRY_WAIT_NS);
    }
}

} // namespace sphaira::yati::source
