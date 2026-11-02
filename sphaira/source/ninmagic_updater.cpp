#include "ninmagic_updater.hpp"
#include "ninmagic_tls.hpp"
#include "app.hpp"
#include "defines.hpp"
#include "fs.hpp"
#include "log.hpp"
#include "minizip_helper.hpp"
#include "nacp_compat.hpp"
#include "nro.hpp"
#include "ui/progress_box.hpp"

#include <curl/curl.h>
#include <minizip/unzip.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>

namespace ninmagic::update {
namespace {
using sphaira::ui::ProgressBox;
constexpr fs::FsPath ZipPath{"/config/NinMagicFoil/update/download.zip"};
constexpr fs::FsPath StagePath{"/config/NinMagicFoil/update/NinMagicFoil.staged.nro"};
constexpr fs::FsPath BackupPath{"/config/NinMagicFoil/update/NinMagicFoil.previous.nro"};
constexpr fs::FsPath BackupPending{"/config/NinMagicFoil/update/NinMagicFoil.previous.pending"};
constexpr auto Invalid = Result_MainDownloadedUpdateInvalid;
constexpr std::size_t BufferSize = 64 * 1024;

struct Transfer {
    std::stop_token token;
    ProgressBox* progress{};
    std::uint64_t limit{};
    std::uint64_t written{};
    long status{};
    std::function<bool(const void*, std::size_t)> write;
};

auto HeaderCallback(char* data, std::size_t size, std::size_t count, void* opaque) -> std::size_t {
    if (size && count > std::numeric_limits<std::size_t>::max() / size) return 0;
    const auto bytes = size * count;
    auto& transfer = *static_cast<Transfer*>(opaque);
    const std::string_view line{data, bytes};
    if (line.starts_with("HTTP/")) {
        const auto space = line.find(' ');
        transfer.status = 0;
        if (space != std::string_view::npos && line.size() >= space + 4) {
            for (std::size_t i = 1; i <= 3; ++i) {
                if (line[space + i] < '0' || line[space + i] > '9') return 0;
                transfer.status = transfer.status * 10 + (line[space + i] - '0');
            }
        }
    }
    return bytes;
}

auto WriteCallback(char* data, std::size_t size, std::size_t count, void* opaque) -> std::size_t {
    if (size && count > std::numeric_limits<std::size_t>::max() / size) return 0;
    const auto bytes = size * count;
    auto& transfer = *static_cast<Transfer*>(opaque);
    // A redirect or error page is never part of a release asset.
    if (transfer.status != 200) return bytes;
    if (transfer.token.stop_requested() || transfer.written > transfer.limit || bytes > transfer.limit - transfer.written) return 0;
    if (!transfer.write(data, bytes)) return 0;
    transfer.written += bytes;
    return bytes;
}

auto ProgressCallback(void* opaque, curl_off_t total, curl_off_t now, curl_off_t, curl_off_t) -> int {
    auto& transfer = *static_cast<Transfer*>(opaque);
    if (transfer.token.stop_requested()) return 1;
    if (transfer.progress) {
        if (transfer.progress->ShouldExit()) return 1;
        transfer.progress->UpdateTransfer(now, total > 0 ? total : transfer.limit);
    }
    return 0;
}

auto HttpsGet(const std::string& url, Transfer& transfer, bool asset, long& status) -> bool {
    status = 0;
    if (asset ? !IsDownloadHost(url) : url != ReleaseApi) return false;
    auto handle = curl_easy_init();
    if (!handle) return false;
    ON_SCOPE_EXIT(curl_easy_cleanup(handle));
    auto headers = curl_slist_append(nullptr, asset ? "Accept: application/octet-stream" : "Accept: application/vnd.github+json");
    if (!headers) return false;
    ON_SCOPE_EXIT(curl_slist_free_all(headers));
    char detail[CURL_ERROR_SIZE]{};
    const auto set = [handle](CURLoption option, auto value) { return curl_easy_setopt(handle, option, value) == CURLE_OK; };
    const bool configured = set(CURLOPT_URL, url.c_str())
        && set(CURLOPT_USERAGENT, "NinMagicFoil/" APP_VERSION)
        && set(CURLOPT_HTTPHEADER, headers)
        && set(CURLOPT_FOLLOWLOCATION, 0L)
#if LIBCURL_VERSION_NUM >= 0x075500
        && set(CURLOPT_PROTOCOLS_STR, "https")
        && set(CURLOPT_REDIR_PROTOCOLS_STR, "https")
#else
        && set(CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTPS))
        && set(CURLOPT_REDIR_PROTOCOLS, static_cast<long>(CURLPROTO_HTTPS))
#endif
        && ninmagic::tls::ConfigureCurlTrust(handle)
        && set(CURLOPT_CONNECTTIMEOUT, 15L)
        && set(CURLOPT_TIMEOUT, asset ? 300L : 40L)
        && set(CURLOPT_LOW_SPEED_LIMIT, 512L)
        && set(CURLOPT_LOW_SPEED_TIME, 20L)
        && set(CURLOPT_NOSIGNAL, 1L)
        && set(CURLOPT_FAILONERROR, 1L)
        && set(CURLOPT_MAXFILESIZE_LARGE, static_cast<curl_off_t>(transfer.limit))
        && set(CURLOPT_HEADERFUNCTION, HeaderCallback)
        && set(CURLOPT_HEADERDATA, &transfer)
        && set(CURLOPT_WRITEFUNCTION, WriteCallback)
        && set(CURLOPT_WRITEDATA, &transfer)
        && set(CURLOPT_NOPROGRESS, 0L)
        && set(CURLOPT_XFERINFOFUNCTION, ProgressCallback)
        && set(CURLOPT_XFERINFODATA, &transfer)
        && set(CURLOPT_ERRORBUFFER, detail);
    if (!configured) return false;
    // Validate every redirect before connecting. GitHub release assets normally
    // redirect once to its asset CDN; arbitrary HTTPS hosts are not accepted.
    std::string current_url = url;
    for (unsigned redirects = 0; redirects <= 5; ++redirects) {
        if (transfer.token.stop_requested() || !set(CURLOPT_URL, current_url.c_str())) return false;
        transfer.status = 0;
        const auto rc = curl_easy_perform(handle);
        if (curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &status) != CURLE_OK) return false;
        if (rc == CURLE_OK && status >= 300 && status < 400 && asset && redirects < 5) {
            char* location{};
            if (curl_easy_getinfo(handle, CURLINFO_REDIRECT_URL, &location) != CURLE_OK
                || !location || !IsDownloadHost(location) || transfer.written != 0) return false;
            current_url = location;
            continue;
        }
        const bool ok = rc == CURLE_OK && status == 200 && transfer.written > 0
            && transfer.written <= transfer.limit && !transfer.token.stop_requested();
        if (!ok) log_write("[NinMagic updater] HTTPS failed: curl=%d http=%ld detail=%s\n", int(rc), status, detail);
        return ok;
    }
    return false;
}

auto ReadExact(fs::File& file, std::uint64_t offset, void* data, std::uint64_t size) -> Result {
    u64 read{};
    R_TRY(file.Read(offset, data, size, FsReadOption_None, &read));
    R_UNLESS(read == size, Invalid);
    R_SUCCEED();
}

auto OpenWritable(fs::FsNativeSd& filesystem, const fs::FsPath& path, fs::File& file, std::uint64_t size) -> Result {
    const auto create = filesystem.CreateFile(path, size);
    R_UNLESS(R_SUCCEEDED(create) || create == FsError_PathAlreadyExists, create);
    R_TRY(filesystem.OpenFile(path, FsOpenMode_Write | FsOpenMode_Append, &file));
    R_TRY(file.SetSize(size));
    R_SUCCEED();
}

auto Flush(fs::FsNativeSd& filesystem, fs::File& file) -> Result {
    R_TRY(fsFileFlush(&file.m_native));
    file.Close();
    return filesystem.Commit();
}

auto HashFile(fs::FsNativeSd& filesystem, const fs::FsPath& path, std::string& digest, std::uint64_t& size, ProgressBox* cancel = nullptr) -> Result {
    fs::File file;
    R_TRY(filesystem.OpenFile(path, FsOpenMode_Read, &file));
    s64 signed_size{};
    R_TRY(file.GetSize(&signed_size));
    R_UNLESS(signed_size > 0 && static_cast<std::uint64_t>(signed_size) <= MaxAssetSize, Invalid);
    size = signed_size;
    auto buffer = std::make_unique<std::array<u8, BufferSize>>();
    Sha256Context context;
    sha256ContextCreate(&context);
    for (std::uint64_t offset = 0; offset < size;) {
        if (cancel) R_TRY(cancel->ShouldExitResult());
        const auto length = std::min<std::uint64_t>(buffer->size(), size - offset);
        R_TRY(ReadExact(file, offset, buffer->data(), length));
        sha256ContextUpdate(&context, buffer->data(), length);
        offset += length;
    }
    std::array<u8, SHA256_HASH_SIZE> bytes{};
    sha256ContextGetHash(&context, bytes.data());
    static constexpr char hex[] = "0123456789abcdef";
    digest.clear();
    for (const auto byte : bytes) {
        digest.push_back(hex[byte >> 4]);
        digest.push_back(hex[byte & 15]);
    }
    R_SUCCEED();
}

auto ValidateNro(fs::FsNativeSd& filesystem, const fs::FsPath& path, std::string_view version) -> Result {
    fs::File file;
    R_TRY(filesystem.OpenFile(path, FsOpenMode_Read, &file));
    s64 size{};
    R_TRY(file.GetSize(&size));
    R_UNLESS(size >= static_cast<s64>(sizeof(sphaira::NroData)) && static_cast<std::uint64_t>(size) <= MaxAssetSize, Invalid);
    sphaira::NroData data{};
    R_TRY(ReadExact(file, 0, &data, sizeof(data)));
    R_UNLESS(data.header.magic == NROHEADER_MAGIC && data.header.size >= sizeof(data), Invalid);
    R_UNLESS(SectionFits(size, data.header.size, sizeof(NroAssetHeader)), Invalid);
    for (const auto& segment : data.header.segments) R_UNLESS(SectionFits(data.header.size, segment.file_off, segment.size), Invalid);
    NroAssetHeader assets{};
    R_TRY(ReadExact(file, data.header.size, &assets, sizeof(assets)));
    R_UNLESS(assets.magic == NROASSETHEADER_MAGIC && assets.version == NROASSETHEADER_VERSION, Invalid);
    const auto tail = size - data.header.size;
    for (const auto& section : {assets.icon, assets.nacp, assets.romfs}) {
        R_UNLESS(SectionFits(tail, section.offset, section.size), Invalid);
        R_UNLESS(!section.size || section.offset >= sizeof(assets), Invalid);
    }
    R_UNLESS(assets.nacp.size == sizeof(NacpStruct), Invalid);
    auto nacp = std::make_unique<NacpStruct>();
    R_TRY(ReadExact(file, data.header.size + assets.nacp.offset, nacp.get(), sizeof(NacpStruct)));
    bool has_name{};
    for (const auto& language : sphaira::NacpLanguageEntries(*nacp)) {
        if (!language.name[0]) continue;
        const auto length = strnlen(language.name, sizeof(language.name));
        R_UNLESS((length < sizeof(language.name) && std::string_view{language.name, length} == AppName), Invalid);
        has_name = true;
    }
    R_UNLESS(has_name, Invalid);
    const auto length = strnlen(nacp->display_version, sizeof(nacp->display_version));
    R_UNLESS(length < sizeof(nacp->display_version), Invalid);
    const auto actual = ParseVersion({nacp->display_version, length}, false);
    const auto expected = ParseVersion(version);
    R_UNLESS(actual && expected && *actual == *expected, Invalid);
    R_SUCCEED();
}

auto ExtractNro(fs::FsNativeSd& filesystem, ProgressBox* progress) -> Result {
    zlib_filefunc64_def callbacks;
    sphaira::mz::FileFuncNative(&callbacks);
    auto zip = unzOpen2_64(ZipPath.s, &callbacks);
    R_UNLESS(zip, Invalid);
    ON_SCOPE_EXIT(unzClose(zip));
    unz_global_info64 global{};
    R_UNLESS(unzGetGlobalInfo64(zip, &global) == UNZ_OK && global.number_entry > 0 && global.number_entry <= 4096, Invalid);
    R_UNLESS(unzGoToFirstFile(zip) == UNZ_OK, Invalid);
    bool found{};
    for (std::uint64_t index = 0; index < global.number_entry; ++index) {
        R_TRY(progress->ShouldExitResult());
        if (index) R_UNLESS(unzGoToNextFile(zip) == UNZ_OK, Invalid);
        unz_file_info64 info{};
        std::array<char, 1024> name{};
        R_UNLESS(unzGetCurrentFileInfo64(zip, &info, name.data(), name.size(), nullptr, 0, nullptr, 0) == UNZ_OK, Invalid);
        R_UNLESS(info.size_filename < name.size(), Invalid);
        const std::string_view entry{name.data(), info.size_filename};
        if (entry != ArchiveNroPath) continue;
        R_UNLESS(!found && !(info.flag & 1) && (info.compression_method == 0 || info.compression_method == 8), Invalid);
        R_UNLESS(info.uncompressed_size >= sizeof(sphaira::NroData) && info.uncompressed_size <= MaxAssetSize, Invalid);
        const auto type = (info.external_fa >> 16) & 0170000;
        R_UNLESS(type == 0 || type == 0100000, Invalid);
        found = true;
        R_UNLESS(unzOpenCurrentFile(zip) == UNZ_OK, Invalid);
        bool opened = true;
        ON_SCOPE_EXIT(if (opened) unzCloseCurrentFile(zip));
        fs::File output;
        R_TRY(OpenWritable(filesystem, StagePath, output, info.uncompressed_size));
        auto buffer = std::make_unique<std::array<u8, BufferSize>>();
        std::uint64_t written{};
        while (written < info.uncompressed_size) {
            R_TRY(progress->ShouldExitResult());
            const auto amount = std::min<std::uint64_t>(buffer->size(), info.uncompressed_size - written);
            const auto read = unzReadCurrentFile(zip, buffer->data(), amount);
            R_UNLESS(read > 0 && static_cast<std::uint64_t>(read) <= amount, Invalid);
            R_TRY(output.Write(written, buffer->data(), read, FsWriteOption_None));
            written += read;
            progress->UpdateTransfer(written, info.uncompressed_size);
        }
        unsigned char extra{};
        R_UNLESS(unzReadCurrentFile(zip, &extra, 1) == 0, Invalid);
        const auto close = unzCloseCurrentFile(zip);
        opened = false;
        R_UNLESS(close == UNZ_OK, Invalid); // includes CRC validation, even when CRC is zero.
        R_TRY(Flush(filesystem, output));
    }
    R_UNLESS(found, Invalid);
    R_SUCCEED();
}

// Once replacement starts cancellation must not interrupt either replacement
// or rollback. The progress widget remains alive until this worker returns.
auto CopyCritical(fs::FsNativeSd& filesystem, const fs::FsPath& source, const fs::FsPath& destination, ProgressBox* progress) -> Result {
    fs::File input;
    R_TRY(filesystem.OpenFile(source, FsOpenMode_Read, &input));
    s64 size{};
    R_TRY(input.GetSize(&size));
    R_UNLESS(size > 0 && static_cast<std::uint64_t>(size) <= MaxAssetSize, Invalid);
    fs::File output;
    R_TRY(OpenWritable(filesystem, destination, output, size));
    auto buffer = std::make_unique<std::array<u8, BufferSize>>();
    for (s64 offset = 0; offset < size;) {
        const auto length = std::min<s64>(buffer->size(), size - offset);
        R_TRY(ReadExact(input, offset, buffer->data(), length));
        R_TRY(output.Write(offset, buffer->data(), length, FsWriteOption_None));
        offset += length;
        progress->UpdateTransfer(offset, size);
    }
    return Flush(filesystem, output);
}
}

auto FetchReleaseJson(std::stop_token token, std::string& out, long& http_status) -> bool {
    out.clear();
    Transfer transfer{.token = token, .limit = MaxMetadataSize};
    transfer.write = [&out](const void* data, std::size_t size) {
        out.append(static_cast<const char*>(data), size);
        return true;
    };
    const bool ok = HttpsGet(ReleaseApi, transfer, false, http_status);
    if (!ok) out.clear();
    return ok;
}

auto Install(ProgressBox* progress, const Release& release) -> Result {
    R_UNLESS(progress && IsValidRelease(release), Invalid);
    const auto expected = ParseVersion(release.tag);
    const auto current = ParseVersion(APP_VERSION, false);
    R_UNLESS(expected && current && *expected > *current, Invalid);
    R_UNLESS(IsInstallPath(sphaira::App::GetExePath().toString()), Invalid);
    fs::FsNativeSd filesystem;
    R_TRY(filesystem.GetFsOpenResult());
    R_TRY(filesystem.CreateDirectoryRecursively(UpdatePath));
    // Verify the current target before replacing even a recovery copy.
    R_TRY(ValidateNro(filesystem, InstallPath, APP_VERSION));
    filesystem.DeleteFile(StagePath);
    filesystem.DeleteFile(ZipPath);
    ON_SCOPE_EXIT({ filesystem.DeleteFile(StagePath); filesystem.DeleteFile(ZipPath); });

    progress->NewTransfer("Downloading NinMagicFoil");
    fs::File download;
    R_TRY(OpenWritable(filesystem, ZipPath, download, 0));
    Transfer transfer{.token = progress->GetToken(), .progress = progress, .limit = release.size};
    transfer.write = [&download, &transfer](const void* data, std::size_t size) {
        return R_SUCCEEDED(download.Write(transfer.written, data, size, FsWriteOption_None));
    };
    long http_status{};
    const auto downloaded = HttpsGet(release.url, transfer, true, http_status);
    R_UNLESS(downloaded && transfer.written == release.size, Result_MainFailedToDownloadUpdate);
    R_TRY(Flush(filesystem, download));
    R_TRY(progress->ShouldExitResult());
    progress->NewTransfer("Verifying SHA-256");
    std::string archive_hash;
    std::uint64_t archive_size{};
    R_TRY(HashFile(filesystem, ZipPath, archive_hash, archive_size, progress));
    R_UNLESS(archive_size == release.size && archive_hash == release.sha256, Invalid);
    progress->NewTransfer("Extracting NinMagicFoil.nro");
    R_TRY(ExtractNro(filesystem, progress));
    R_TRY(ValidateNro(filesystem, StagePath, release.tag));
    std::string staged_hash;
    std::uint64_t staged_size{};
    R_TRY(HashFile(filesystem, StagePath, staged_hash, staged_size, progress));
    std::string original_hash;
    std::uint64_t original_size{};
    R_TRY(HashFile(filesystem, InstallPath, original_hash, original_size, progress));
    R_TRY(progress->ShouldExitResult());

    progress->NewTransfer("Backing up NinMagicFoil");
    R_TRY(CopyCritical(filesystem, InstallPath, BackupPending, progress));
    std::string backup_hash;
    std::uint64_t backup_size{};
    R_TRY(HashFile(filesystem, BackupPending, backup_hash, backup_size));
    R_UNLESS(backup_hash == original_hash && backup_size == original_size, Invalid);
    if (filesystem.FileExists(BackupPath)) R_TRY(filesystem.DeleteFile(BackupPath));
    R_TRY(filesystem.RenameFile(BackupPending, BackupPath));
    R_TRY(progress->ShouldExitResult());

    progress->NewTransfer("Installing verified NinMagicFoil update");
    auto result = CopyCritical(filesystem, StagePath, InstallPath, progress);
    std::string installed_hash;
    std::uint64_t installed_size{};
    if (R_SUCCEEDED(result)) result = HashFile(filesystem, InstallPath, installed_hash, installed_size);
    if (R_SUCCEEDED(result) && (installed_hash != staged_hash || installed_size != staged_size)) result = Invalid;
    if (R_SUCCEEDED(result)) result = ValidateNro(filesystem, InstallPath, release.tag);
    if (R_FAILED(result)) {
        progress->NewTransfer("Restoring previous NinMagicFoil");
        auto rollback = CopyCritical(filesystem, BackupPath, InstallPath, progress);
        if (R_SUCCEEDED(rollback)) rollback = HashFile(filesystem, InstallPath, installed_hash, installed_size);
        const bool recovered = R_SUCCEEDED(rollback) && installed_hash == original_hash && installed_size == original_size;
        log_write("[NinMagic updater] replacement failed: 0x%X rollback verified: %d; backup retained at %s\n", result, recovered, BackupPath.s);
        R_THROW(Result_MainFailedToInstallUpdate);
    }
    // Retain the verified backup for manual recovery after a later boot failure.
    log_write("[NinMagic updater] verified update installed; previous NRO retained at %s\n", BackupPath.s);
    R_SUCCEED();
}
}
