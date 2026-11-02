#!/usr/bin/env python3
"""Compile and run the portable C++ policy/parser tests with a host compiler."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def compiler_path(compiler, value):
    value = Path(value).resolve().as_posix()
    if sys.platform == "win32" and "/usr/bin/" in compiler.replace("\\", "/").lower():
        # MSYS-hosted GCC accepts /c/...; native Clang accepts C:/....
        if len(value) > 2 and value[1] == ":":
            return "/" + value[0].lower() + value[2:]
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--yyjson", type=Path, required=True, help="Pinned yyjson source directory containing yyjson.c and yyjson.h")
    parser.add_argument("--output", type=Path, default=ROOT / ".local/host-tests")
    parser.add_argument("--cxx", default=shutil.which("clang++") or shutil.which("g++"))
    parser.add_argument("--cc", default=shutil.which("clang") or shutil.which("gcc"))
    args = parser.parse_args()
    if not args.cxx or not args.cc:
        parser.error("Install a host C/C++ compiler (Clang or GCC); do not use the Switch cross-compiler")
    yyjson = args.yyjson.resolve()
    if not (yyjson / "yyjson.c").is_file() or not (yyjson / "yyjson.h").is_file():
        parser.error("--yyjson must point to the fetched, pinned yyjson src directory")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    yyjson_object = output / "yyjson.o"
    subprocess.run([args.cc, "-std=c99", "-O1", "-c", compiler_path(args.cc, yyjson / "yyjson.c"),
                    "-o", compiler_path(args.cc, yyjson_object)], check=True)
    for name in ("test_update_policy", "test_http_policy", "test_ownfoil_index"):
        executable = output / (name + (".exe" if sys.platform == "win32" else ""))
        command = [args.cxx, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-O1",
                   "-I", compiler_path(args.cxx, ROOT / "sphaira/include"), "-I", compiler_path(args.cxx, yyjson),
                   compiler_path(args.cxx, ROOT / "tools/tests" / (name + ".cpp")), "-o", compiler_path(args.cxx, executable)]
        if name == "test_ownfoil_index":
            command.append(compiler_path(args.cxx, yyjson_object))
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)
    print("All 3 portable C++ test executables passed.")


if __name__ == "__main__":
    main()
