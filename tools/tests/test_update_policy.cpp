#include "ninmagic_update_policy.hpp"
#include <cassert>
#include <iostream>
#include <limits>

int main() {
    using namespace ninmagic::update;
    assert(ParseVersion("0.1.0") == ParseVersion("v0.1.0"));
    assert(ParseVersion("1.0.0") > ParseVersion("0.99.99"));
    for (const auto bad : {"", "v", "1.2", "1.2.3.4", "1.2.3-beta", "01.2.3", "1.-1.0", "65536.0.0", "1.2.3/../bad"}) assert(!ParseVersion(bad));
    assert(!ParseVersion("v1.2.3", false));
    const std::string hash(64, 'a');
    assert(ParseDigest("sha256:" + hash) == hash);
    assert(ParseDigest("sha256:" + std::string(64, 'A')) == hash);
    assert(!ParseDigest("sha256:" + std::string(63, 'a')));
    assert(!ParseDigest("sha256:" + std::string(64, 'g')));
    assert(!ParseDigest("sha512:" + hash));
    const auto good = "https://github.com/ANE-JV/ninmagicfoil/releases/download/v0.1.1/ninmagicfoil.zip";
    assert(IsAssetUrl(good, "v0.1.1"));
    for (const auto bad : {
        "http://github.com/ANE-JV/ninmagicfoil/releases/download/v0.1.1/ninmagicfoil.zip",
        "https://github.com/NaGaa95/sphaira/releases/download/v0.1.1/sphaira.zip",
        "https://github.com/ANE-JV/ninmagicfoil/releases/download/v0.1.1/other.zip",
        "https://github.com/ANE-JV/ninmagicfoil/releases/download/v0.1.1/ninmagicfoil.zip?other",
        "https://github.com.evil.example/ANE-JV/ninmagicfoil/releases/download/v0.1.1/ninmagicfoil.zip"}) assert(!IsAssetUrl(bad, "v0.1.1"));
    assert(IsDownloadHost(good));
    assert(IsDownloadHost("https://release-assets.githubusercontent.com/github-production-release-asset/file?token=public-signed"));
    for (const auto bad : {"http://github.com/file", "https://github.com@evil.example/file", "https://evil.example/file", "https://github.com:444/file", "https://github.com/file#fragment", "https://github.com/\\file", "https://github.com/file\r\n"}) assert(!IsDownloadHost(bad));
    assert(IsInstallPath(ninmagic::InstallPath));
    assert(IsInstallPath("sdmc:/switch/NinMagicFoil/NinMagicFoil.nro"));
    for (const auto bad : {"/hbmenu.nro", "/switch/sphaira/sphaira.nro", "/switch/NinMagicFoil/../hbmenu.nro", "/switch/NinMagicFoil/NinMagicFoil.nro.bak"}) assert(!IsInstallPath(bad));
    Release r{"v0.1.1", good, hash, 1234};
    assert(IsValidRelease(r));
    r.size = 0; assert(!IsValidRelease(r));
    r.size = MaxAssetSize + 1; assert(!IsValidRelease(r));
    assert(SectionFits(100, 90, 10));
    assert(!SectionFits(100, 90, 11));
    assert(!SectionFits(100, std::numeric_limits<std::uint64_t>::max(), 2));
    std::cout << "NinMagicFoil update policy: passed\n";
}
