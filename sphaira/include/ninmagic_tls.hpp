#pragma once

#include <curl/curl.h>
#include <switch.h>

namespace ninmagic::tls {
inline constexpr char TrustStorePath[] = "sdmc:/config/NinMagicFoil/cache/cacert.pem";
inline constexpr char TrustStoreSha256[] = "796f1389bfea13c0ed78f663b765baaff54a508e5483786bd6ece73c5e3e3cbe";

// Call on the main thread before starting any worker that uses ROMFS or curl.
// ROMFS is unmounted before returning, including on failure.
auto PrepareTrustStore() -> Result;
// Verifies the cached bundle against the embedded pin before each connection.
// No insecure fallback is enabled when verification/configuration fails.
auto ConfigureCurlTrust(CURL* handle) -> bool;
}
