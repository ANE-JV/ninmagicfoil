#!/usr/bin/env bash
# Run inside devkitPro MSYS2 or the official devkitpro/devkita64 build image.
set -euo pipefail
source_root=$(cd "${1:-$(dirname "$0")/..}" && pwd)
build_root=${2:-"$source_root/build/ninmagicfoil"}
jobs=${NINMAGIC_BUILD_JOBS:-4}
: "${DEVKITPRO:?Set DEVKITPRO to the installed devkitPro SDK}"
export DEVKITA64=${DEVKITA64:-"$DEVKITPRO/devkitA64"}
export PATH="$DEVKITPRO/tools/bin:$PATH"
libnx_commit=$(sed -nE 's/^[[:space:]]*"libnx_commit": "([a-f0-9]{40})",?$/\1/p' "$source_root/tools/toolchain-lock.json")
if [[ ! "$libnx_commit" =~ ^[a-f0-9]{40}$ ]]; then
    echo 'Missing or invalid pinned libnx commit in tools/toolchain-lock.json.' >&2; exit 1
fi
toolchain_root=${NINMAGIC_TOOLCHAIN_ROOT:-"$build_root/toolchain"}
libnx_source="$toolchain_root/libnx"
export NINMAGIC_LIBNX_ROOT="$toolchain_root/libnx-sdk"
mkdir -p "$toolchain_root" "$build_root"
if [[ ! -d "$libnx_source/.git" ]]; then
    git init "$libnx_source"
    git -C "$libnx_source" remote add origin https://github.com/switchbrew/libnx.git
fi
if [[ "$(git -C "$libnx_source" remote get-url origin)" != https://github.com/switchbrew/libnx.git ]]; then
    echo 'Unexpected libnx source remote; refusing to use it.' >&2; exit 1
fi
if ! git -C "$libnx_source" cat-file -e "$libnx_commit^{commit}" 2>/dev/null; then
    git -C "$libnx_source" fetch --depth=1 origin "$libnx_commit"
fi
if [[ -n "$(git -C "$libnx_source" status --porcelain --untracked-files=no)" ]]; then
    echo 'libnx source has local modifications; refusing to overwrite them.' >&2; exit 1
fi
git -C "$libnx_source" checkout --detach "$libnx_commit"
make -C "$libnx_source" -j"$jobs"
mkdir -p "$NINMAGIC_LIBNX_ROOT/include" "$NINMAGIC_LIBNX_ROOT/lib"
cp -Rp "$libnx_source/nx/include/." "$NINMAGIC_LIBNX_ROOT/include/"
cp -Rp "$libnx_source/nx/external/bsd/include/." "$NINMAGIC_LIBNX_ROOT/include/"
# devkitPro installs deko3d's public headers next to the system libnx headers.
cp -p "$DEVKITPRO/libnx/include/deko3d.h" "$DEVKITPRO/libnx/include/deko3d.hpp" "$NINMAGIC_LIBNX_ROOT/include/"
cp -p "$libnx_source/nx/lib/libnx.a" "$NINMAGIC_LIBNX_ROOT/lib/"
cp -p "$libnx_source/nx/switch.specs" "$libnx_source/nx/switch.ld" "$libnx_source/nx/switch_rules" "$NINMAGIC_LIBNX_ROOT/"
cp -p "$libnx_source/nx/default_icon.jpg" "$NINMAGIC_LIBNX_ROOT/"
cmake -S "$source_root" -B "$build_root" \
    -DCMAKE_TOOLCHAIN_FILE="$source_root/cmake/NinMagicSwitch.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DLTO=OFF -DCMAKE_DEPENDS_USE_COMPILER=FALSE \
    -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build "$build_root" --parallel "$jobs"
test -s "$build_root/NinMagicFoil.nro"
{
    printf 'libnx %s\n' "$libnx_commit"
    "$DEVKITA64/bin/aarch64-none-elf-gcc" --version | head -n 1
    cmake --version | head -n 1
    if command -v pacman >/dev/null; then
        pacman -Q devkitA64 devkitA64-gdb libnx switch-tools switch-cmake switch-curl switch-glm switch-mbedtls switch-ntfs-3g switch-zlib 2>/dev/null || true
    elif command -v dkp-pacman >/dev/null; then
        dkp-pacman -Q devkitA64 devkitA64-gdb libnx switch-tools switch-cmake switch-curl switch-glm switch-mbedtls switch-ntfs-3g switch-zlib 2>/dev/null || true
    fi
} > "$build_root/toolchain-versions.txt"
printf '\nBuilt %s\n' "$build_root/NinMagicFoil.nro"
