#!/usr/bin/env python3
"""Create an allowlisted release ZIP from a validated build and license notices."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import zipfile


ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("verify_ninmagicfoil", Path(__file__).with_name("verify-ninmagicfoil.py"))
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)
PACKAGE_ROOT = "switch/NinMagicFoil/"
LICENSE_NAMES = re.compile(r"^(license|licence|copying|copyright|notice|authors)([._-].*)?$", re.I)


def git_revision(path: Path) -> str | None:
    result = subprocess.run(["git", "-C", str(path), "rev-parse", "HEAD"], capture_output=True, text=True)
    return result.stdout.strip() if result.returncode == 0 else None


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("nro", type=Path)
    parser.add_argument("--version", default=validator.project_version())
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--deps-dir", type=Path, help="Override FetchContent cache for an existing local build")
    parser.add_argument("--libnx-source", type=Path, help="Override pinned libnx source location")
    parser.add_argument("--devkitpro", type=Path, default=Path(os.environ.get("DEVKITPRO", "C:/devkitPro")))
    parser.add_argument("--output", type=Path, default=ROOT / "dist/ninmagicfoil.zip")
    args = parser.parse_args()
    validated = validator.verify(args.nro, args.version)
    entries: dict[str, bytes] = {PACKAGE_ROOT + "NinMagicFoil.nro": args.nro.read_bytes()}
    entries[PACKAGE_ROOT + "licenses/NinMagicFoil-GPL-3.0.txt"] = (ROOT / "LICENSE").read_bytes()
    ca_provenance = ROOT / "assets/cacert-provenance.md"
    if ca_provenance.is_file():
        entries[PACKAGE_ROOT + "licenses/cacert-provenance.md"] = ca_provenance.read_bytes()
    dependencies = []
    deps_dir = args.deps_dir or args.build_dir / "_deps"
    if not deps_dir.is_dir():
        raise SystemExit("Missing build dependency tree; cannot collect corresponding notices")
    for dep in sorted(deps_dir.glob("*-src")):
        if not dep.is_dir():
            continue
        notices = [p for p in dep.iterdir() if p.is_file() and LICENSE_NAMES.match(p.name)]
        # Header-only projects occasionally keep their notice in a documented subfolder.
        for subfolder in ("LICENSES", "licenses"):
            folder = dep / subfolder
            if folder.is_dir():
                notices.extend(p for p in folder.rglob("*") if p.is_file())
        dependency = {"name": dep.name.removesuffix("-src"), "commit": git_revision(dep), "notices": []}
        for notice in sorted(set(notices)):
            if notice.is_symlink() or notice.stat().st_size > 2_000_000:
                raise SystemExit(f"Invalid dependency notice: {notice.name}")
            name = PACKAGE_ROOT + "licenses/dependencies/" + dep.name.removesuffix("-src") + "/" + notice.relative_to(dep).as_posix()
            entries[name] = notice.read_bytes()
            dependency["notices"].append(notice.relative_to(dep).as_posix())
        dependencies.append(dependency)
    libnx_source = args.libnx_source or args.build_dir / "toolchain/libnx"
    if not (libnx_source / "LICENSE.md").is_file():
        raise SystemExit("Missing pinned libnx source license")
    entries[PACKAGE_ROOT + "licenses/libnx-LICENSE.md"] = (libnx_source / "LICENSE.md").read_bytes()
    dependencies.append({"name": "libnx", "commit": git_revision(libnx_source), "notices": ["LICENSE.md"]})
    # Packaged portlib and project-preserved notices, never arbitrary files from the SDK.
    portlibs = args.devkitpro / "portlibs/switch/licenses"
    linked_portlibs = ("switch-zlib", "switch-mbedtls", "switch-ntfs-3g", "switch-curl", "switch-glm")
    for library in linked_portlibs:
        folder = portlibs / library
        if folder.is_dir():
            for notice in sorted(folder.rglob("*")):
                if notice.is_file() and not notice.is_symlink():
                    entries[PACKAGE_ROOT + "licenses/portlibs/" + library + "/" + notice.relative_to(folder).as_posix()] = notice.read_bytes()
    notices_dir = ROOT / "third_party/licenses"
    if notices_dir.is_dir():
        for notice in sorted(notices_dir.rglob("*")):
            if notice.is_file() and not notice.is_symlink():
                entries[PACKAGE_ROOT + "licenses/notices/" + notice.relative_to(notices_dir).as_posix()] = notice.read_bytes()
    if not dependencies:
        raise SystemExit("No dependency source information found")
    versions = args.build_dir / "toolchain-versions.txt"
    toolchain_versions = versions.read_text(encoding="utf-8") if versions.is_file() else "not recorded"
    receipt = {"project": "NinMagicFoil", "version": args.version, "source_repository": "https://github.com/ANE-JV/ninmagicfoil", "source_commit": git_revision(ROOT), "binary": validated, "dependencies": dependencies, "toolchain_versions": toolchain_versions}
    entries[PACKAGE_ROOT + "build-info.json"] = (json.dumps(receipt, ensure_ascii=False, indent=2) + "\n").encode()
    entries[PACKAGE_ROOT + "LEEME.txt"] = (
        f"NinMagicFoil {args.version}\n\n"
        "Extrae switch/NinMagicFoil en la microSD y abre NinMagicFoil desde hbmenu en modo aplicacion.\n"
        "Esta distribucion incluye solo la aplicacion y sus avisos; no incluye juegos ni configuracion privada.\n"
        "Las pruebas estructurales del NRO no sustituyen la comprobacion de funcionamiento en consola.\n"
        "Fuentes y estado de compatibilidad: https://github.com/ANE-JV/ninmagicfoil\n"
    ).encode("utf-8")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temp = args.output.with_suffix(".zip.tmp")
    with zipfile.ZipFile(temp, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, contents in sorted(entries.items()):
            if not name.startswith(PACKAGE_ROOT) or any(part in ("..", ".local", ".git") for part in Path(name).parts):
                raise SystemExit("Unsafe package entry")
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            archive.writestr(info, contents, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    with zipfile.ZipFile(temp) as archive:
        if archive.testzip() is not None or set(archive.namelist()) != set(entries):
            raise SystemExit("Package read-back failed")
    temp.replace(args.output)
    sha256 = hashlib.sha256(args.output.read_bytes()).hexdigest()
    checksum = args.output.with_suffix(".zip.sha256")
    checksum.write_text(f"{sha256}  {args.output.name}\n", encoding="ascii")
    (args.output.parent / "SHA256SUMS").write_text(
        f"{validated['sha256']}  NinMagicFoil.nro\n{sha256}  {args.output.name}\n", encoding="ascii")
    print(json.dumps({"package": str(args.output), "bytes": args.output.stat().st_size, "sha256": sha256, "entries": len(entries)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
