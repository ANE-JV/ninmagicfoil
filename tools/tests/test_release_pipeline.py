"""Exercise corruption/branding guards without needing a Switch or its private data."""
import importlib.util
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

TOOLS = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("verify_ninmagicfoil", TOOLS / "verify-ninmagicfoil.py")
validator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validator)


def fixture():
    nro_size = 0x1000
    icon = b"\xff\xd8test icon\xff\xd9"
    nacp = bytearray(0x4000)
    nacp[:13] = b"NinMagicFoil\0"
    nacp[0x3060:0x3066] = b"0.1.0\0"
    romfs = struct.pack("<Q", 0x50) + bytes(0x48)
    data = bytearray(nro_size)
    data[0x10:0x14] = b"NRO0"
    struct.pack_into("<I", data, 0x18, nro_size)
    for i in range(3):
        struct.pack_into("<II", data, 0x20 + i * 8, i * 0x400, 0x400)
    channel = b"api.github.com/repos/ANE-JV/ninmagicfoil/releases/latest"
    data[0x400:0x400 + len(channel)] = channel
    aset = bytearray(0x38)
    aset[:4] = b"ASET"
    offset = len(aset)
    for i, asset in enumerate((icon, nacp, romfs)):
        struct.pack_into("<QQ", aset, 8 + i * 16, offset, len(asset))
        offset += len(asset)
    return data + aset + icon + nacp + romfs


class ReleaseValidation(unittest.TestCase):
    def run_validation(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "NinMagicFoil.nro"
            path.write_bytes(data)
            return validator.verify(path, "0.1.0")

    def test_valid_structure(self):
        result = self.run_validation(fixture())
        self.assertEqual(result["nacp_name"], "NinMagicFoil")
        self.assertEqual(result["version"], "0.1.0")

    def test_truncation(self):
        for size in (0, 0x7F, 0x1000, len(fixture()) - 1):
            with self.subTest(size=size), self.assertRaises(ValueError):
                self.run_validation(fixture()[:size])

    def test_overlapping_assets(self):
        data = fixture()
        struct.pack_into("<Q", data, 0x1000 + 24, 0x38)
        with self.assertRaisesRegex(ValueError, "Overlapping"):
            self.run_validation(data)

    def test_wrong_brand_and_channel(self):
        for old, new in ((b"NinMagicFoil", b"Other___Foil"), (b"ANE-JV/ninmagicfoil", b"OTHER_/ninmagicfoil")):
            with self.subTest(field=old), self.assertRaises(ValueError):
                self.run_validation(fixture().replace(old, new))

    def test_upstream_updater_rejected(self):
        data = fixture()
        channel = b"api.github.com/repos/NaGaa95/sphaira/releases/latest"
        data[0x500:0x500 + len(channel)] = channel
        with self.assertRaisesRegex(ValueError, "upstream"):
            self.run_validation(data)

    def test_wrong_version(self):
        with self.assertRaisesRegex(ValueError, "Expected version"):
            self.run_validation(fixture().replace(b"0.1.0\0", b"9.0.0\0"))

    def test_package_allowlist_and_deterministic_zip(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            nro = base / "NinMagicFoil.nro"
            nro.write_bytes(fixture())
            deps = base / "build/_deps/example-src"
            deps.mkdir(parents=True)
            (deps / "LICENSE").write_text("Public test license", encoding="utf-8")
            (deps / "private.txt").write_text("PRIVATE-SENTINEL-DO-NOT-PACK", encoding="utf-8")
            private = base / "build/.local"
            private.mkdir()
            (private / "credentials.json").write_text("PRIVATE-SENTINEL-DO-NOT-PACK", encoding="utf-8")
            libnx = base / "libnx"
            libnx.mkdir()
            (libnx / "LICENSE.md").write_text("Public test libnx license", encoding="utf-8")
            output = base / "ninmagicfoil.zip"
            command = [sys.executable, str(TOOLS / "package-ninmagicfoil.py"), str(nro),
                       "--version", "0.1.0",
                       "--build-dir", str(base / "build"), "--devkitpro", str(base / "sdk"),
                       "--libnx-source", str(libnx), "--output", str(output)]
            subprocess.run(command, check=True, capture_output=True)
            first = output.read_bytes()
            subprocess.run(command, check=True, capture_output=True)
            self.assertEqual(first, output.read_bytes())
            with zipfile.ZipFile(output) as archive:
                self.assertIsNone(archive.testzip())
                self.assertTrue(all(name.startswith("switch/NinMagicFoil/") for name in archive.namelist()))
                self.assertIn("switch/NinMagicFoil/NinMagicFoil.nro", archive.namelist())
                self.assertFalse(any(b"PRIVATE-SENTINEL" in archive.read(name) for name in archive.namelist()))


if __name__ == "__main__":
    unittest.main()
