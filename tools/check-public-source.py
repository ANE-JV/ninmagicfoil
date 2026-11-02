#!/usr/bin/env python3
"""Fail on private inputs accidentally selected for a public source checkout.

Scans Git's tracked and non-ignored new files, never printing matched secret text.
This is a focused release guard, not proof that all sensitive data is absent.
"""
from pathlib import Path
import re
import subprocess
import sys
from urllib.parse import urlsplit

ROOT = Path(__file__).resolve().parent.parent
FORBIDDEN_PARTS = {".local", "dist", "node_modules", "__pycache__"}
FORBIDDEN_NAMES = {"prod.keys", "title.keys", "dev.keys", "console.keys", ".env", "credentials.json"}
FORBIDDEN_SUFFIXES = {".tik", ".nsp", ".nsz", ".xci", ".xcz", ".mp4", ".nro"}
PATTERNS = {
    "private-key": re.compile(rb"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
    "github-token": re.compile(rb"\b(?:gh[pousr]_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{60,})\b"),
}
URLS = re.compile(rb"https?://[^\s\"'<>]+")


def main():
    result = subprocess.run(["git", "-C", str(ROOT), "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
                            check=True, capture_output=True)
    names = sorted(set(result.stdout.decode("utf-8").split("\0")) - {""})
    failures = []
    for name in names:
        path = Path(name)
        if (set(path.parts) & FORBIDDEN_PARTS or path.name.lower() in FORBIDDEN_NAMES
                or path.suffix.lower() in FORBIDDEN_SUFFIXES):
            failures.append((name, "private-or-build-file"))
            continue
        full = ROOT / path
        if full.is_symlink():
            failures.append((name, "symlink-needs-review"))
            continue
        if not full.is_file():
            continue
        data = full.read_bytes()
        for rule, pattern in PATTERNS.items():
            if pattern.search(data):
                failures.append((name, rule))
        for match in URLS.finditer(data):
            try:
                url = urlsplit(match.group().decode("utf-8"))
                if url.password and url.hostname and not url.hostname.endswith((".invalid", ".example")):
                    failures.append((name, "credential-in-url"))
                    break
            except (ValueError, UnicodeDecodeError):
                continue
    if failures:
        for name, reason in failures:
            print(f"{name}: {reason}", file=sys.stderr)
        return 1
    print(f"Public-source guard passed for {len(names)} selected files (ignored local files excluded).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
