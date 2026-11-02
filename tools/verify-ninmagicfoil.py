#!/usr/bin/env python3
"""Validate a produced NinMagicFoil NRO without running it."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import struct


def project_version() -> str:
    cmake = Path(__file__).resolve().parent.parent / "sphaira/CMakeLists.txt"
    match = re.search(r"^set\(sphaira_VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\)", cmake.read_text(encoding="utf-8"), re.MULTILINE)
    if not match:
        raise ValueError("No canonical sphaira_VERSION found in sphaira/CMakeLists.txt")
    return match.group(1)


def verify(path: Path, version: str | None = None, check_channel: bool = True) -> dict:
    version = version or project_version()
    data = path.read_bytes()
    if len(data) < 0x80 or data[0x10:0x14] != b"NRO0":
        raise ValueError("Missing NRO0 executable header")
    nro_size = struct.unpack_from("<I", data, 0x18)[0]
    if nro_size < 0x80 or nro_size + 0x38 > len(data):
        raise ValueError("Invalid executable size / missing assets")
    previous_end = 0
    for i, name in enumerate(("text", "rodata", "data")):
        offset, size = struct.unpack_from("<II", data, 0x20 + i * 8)
        if offset < previous_end or offset + size > nro_size:
            raise ValueError(f"Invalid {name} segment bounds")
        previous_end = offset + size
    if data[nro_size:nro_size + 4] != b"ASET":
        raise ValueError("Missing ASET header")
    ranges = {}
    for i, name in enumerate(("icon", "nacp", "romfs")):
        offset, size = struct.unpack_from("<QQ", data, nro_size + 8 + i * 16)
        if not size or offset < 0x38 or nro_size + offset + size > len(data):
            raise ValueError(f"Invalid {name} asset bounds")
        ranges[name] = (nro_size + offset, size)
    ordered = sorted(ranges.values())
    if any(a + n > b for (a, n), (b, _) in zip(ordered, ordered[1:])):
        raise ValueError("Overlapping NRO assets")
    start, size = ranges["nacp"]
    if size != 0x4000:
        raise ValueError("NACP is not 16 KiB")
    nacp = data[start:start + size]
    decode = lambda b: b.split(b"\0", 1)[0].decode("utf-8", errors="strict")
    names = [decode(nacp[i * 0x300:i * 0x300 + 0x200]) for i in range(16)]
    if not names[0] or any(name != "NinMagicFoil" for name in names if name):
        raise ValueError(f"Unexpected NACP name: {names[0]!r}")
    actual_version = decode(nacp[0x3060:0x3070])
    if actual_version != version:
        raise ValueError(f"Expected version {version}, got {actual_version!r}")
    icon_start, icon_size = ranges["icon"]
    if data[icon_start:icon_start + 2] != b"\xff\xd8":
        raise ValueError("The hbmenu icon is not a JPEG")
    romfs_start, _ = ranges["romfs"]
    if struct.unpack_from("<Q", data, romfs_start)[0] != 0x50:
        raise ValueError("Missing standard RomFS header")
    own_channel = b"api.github.com/repos/ANE-JV/ninmagicfoil/releases/latest"
    stale_channels = (
        b"api.github.com/repos/NaGaa95/sphaira/releases/latest",
        b"api.github.com/repos/ITotalJustice/sphaira/releases/latest",
    )
    if check_channel and own_channel not in data:
        raise ValueError("NinMagicFoil update channel was not found")
    if any(channel in data for channel in stale_channels):
        raise ValueError("The executable still contains an upstream update channel")
    return {
        "file": path.name,
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "nacp_name": names[0],
        "version": actual_version,
        "icon_sha256": hashlib.sha256(data[icon_start:icon_start + icon_size]).hexdigest(),
        "asset_sizes": {k: v[1] for k, v in ranges.items()},
        "own_update_channel_checked": check_channel,
        "validation": "structural only; does not establish execution or installation on console",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("nro", type=Path)
    parser.add_argument("--version", default=project_version())
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        result = verify(args.nro, args.version)
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, f"Validation failed: {error}\n")
    rendered = json.dumps(result, indent=2, ensure_ascii=False) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
