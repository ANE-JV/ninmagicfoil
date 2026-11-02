# CA bundle provenance

- File: `assets/romfs/cacert.pem`.
- Downloaded over verified HTTPS from [curl CA bundle](https://curl.se/ca/cacert.pem) on 2026-11-02.
- Bundle date: 2026-10-20 13:32:40 UTC, as recorded in its header.
- SHA-256 of the complete downloaded file: `796f1389bfea13c0ed78f663b765baaff54a508e5483786bd6ece73c5e3e3cbe`.
- Mozilla certificate data under MPL 2.0, distributed by curl; [generation and licensing](https://curl.se/docs/caextract.html). The hash in the PEM's own header identifies its input data and is different from the whole-file hash above.

The devkitPro curl 7.69.1 [libnx backend patch](https://github.com/devkitPro/pacman-packages/blob/master/switch/curl/switch-curl.patch) supports CAINFO and imports PEM through `sslContextImportServerPki`. Peer and hostname verification are required; certificate errors do not enable an insecure retry. Physical-device verification, including the console clock and certificate import, remains necessary.

When updating this asset, download from the same verified HTTPS source, record its date and whole-file hash, and update the pinned bundle hash in code. Do not replace it from a store-provided URL.

At startup `PrepareTrustStore` reads and hashes the embedded PEM on the main thread, then closes ROMFS. It writes `/config/NinMagicFoil/cache/cacert.pem` and verifies the saved data. Each request verifies that cache against the compiled hash before curl loads it; a changed or missing bundle fails closed.
