#include "ninmagic_tls.hpp"
#include "defines.hpp"
#include "fs.hpp"
#include "log.hpp"

#include <array>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>

namespace ninmagic::tls {
namespace {
constexpr fs::FsPath CachePath{"/config/NinMagicFoil/cache/cacert.pem"};
constexpr fs::FsPath PendingPath{"/config/NinMagicFoil/cache/cacert.pending"};
constexpr std::size_t BundleSize = 200116;
std::atomic_bool ready{};

auto MatchesPin(const std::vector<u8>& bytes) -> bool {
    if (bytes.size() != BundleSize) return false;
    std::array<u8, SHA256_HASH_SIZE> digest{};
    sha256CalculateHash(digest.data(), bytes.data(), bytes.size());
    static constexpr char hex[] = "0123456789abcdef";
    std::string actual;
    actual.reserve(64);
    for (const auto byte : digest) {
        actual += hex[byte >> 4];
        actual += hex[byte & 15];
    }
    return actual == TrustStoreSha256;
}

auto ReadBundle(const char* path, std::vector<u8>& bytes) -> bool {
    auto file = std::fopen(path, "rb");
    if (!file) return false;
    ON_SCOPE_EXIT(std::fclose(file));
    bytes.resize(BundleSize);
    if (std::fread(bytes.data(), 1, bytes.size(), file) != bytes.size()
        || std::fgetc(file) != EOF || std::ferror(file)) return false;
    return MatchesPin(bytes);
}
}

auto PrepareTrustStore() -> Result {
    ready = false;
    std::vector<u8> bytes;
    {
        R_TRY(romfsInit());
        ON_SCOPE_EXIT(romfsExit());
        R_UNLESS(ReadBundle("romfs:/cacert.pem", bytes), Result_MainDownloadedUpdateInvalid);
    }
    fs::FsNativeSd filesystem;
    R_TRY(filesystem.GetFsOpenResult());
    R_TRY(filesystem.CreateDirectoryRecursivelyWithPath(CachePath));
    filesystem.DeleteFile(PendingPath);
    fs::File output;
    ON_SCOPE_EXIT({ output.Close(); filesystem.DeleteFile(PendingPath); });
    R_TRY(filesystem.CreateFile(PendingPath, bytes.size()));
    R_TRY(filesystem.OpenFile(PendingPath, FsOpenMode_Write, &output));
    R_TRY(output.Write(0, bytes.data(), bytes.size(), FsWriteOption_None));
    R_TRY(fsFileFlush(&output.m_native));
    output.Close();
    R_TRY(filesystem.Commit());
    if (filesystem.FileExists(CachePath)) R_TRY(filesystem.DeleteFile(CachePath));
    R_TRY(filesystem.RenameFile(PendingPath, CachePath));
    R_TRY(filesystem.Commit());
    R_UNLESS(ReadBundle(TrustStorePath, bytes), Result_MainDownloadedUpdateInvalid);
    ready = true;
    R_SUCCEED();
}

auto ConfigureCurlTrust(CURL* handle) -> bool {
    std::vector<u8> bytes;
    if (!handle || !ready || !ReadBundle(TrustStorePath, bytes)) {
        log_write("[TLS] pinned certificate bundle unavailable or changed\n");
        return false;
    }
    return curl_easy_setopt(handle, CURLOPT_CAINFO, TrustStorePath) == CURLE_OK
        && curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK
        && curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK;
}
}
