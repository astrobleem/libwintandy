"""Exercise the Win16 implementation using the original host USER mocks."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

WINDOWS_GCC = Path(r"C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py")


def compiler_command(parser, requested):
    if os.name == "nt":
        if requested and Path(requested).resolve() != WINDOWS_GCC:
            parser.error("Windows --cc must name the shared gcc-ucrt64.py launcher; omit --cc to use it")
        if not WINDOWS_GCC.is_file():
            parser.error("Shared Windows GCC launcher missing: " + str(WINDOWS_GCC))
        return [sys.executable, str(WINDOWS_GCC), "--"]
    return [requested or shutil.which("cc") or "gcc"]


def runtime_environment():
    env = os.environ.copy()
    if os.name == "nt":
        # Matching runtime DLLs apply only to the test executable process.
        env["PATH"] = r"C:\msys64\ucrt64\bin" + os.pathsep + env.get("PATH", "")
    return env

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--cc", help="Linux C compiler, or the shared Windows GCC launcher")
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = Path(__file__).resolve().parent / "LTWUITST.C"
compiler = compiler_command(parser, args.cc)
with tempfile.TemporaryDirectory(prefix="libwintandy-ltwui-") as temp:
    exe = Path(temp) / ("ltwui-test.exe" if os.name == "nt" else "ltwui-test")
    command = compiler + [
        "-x", "c", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
        # Win16 original compares int with sizeof; host size_t is unsigned/64-bit.
        "-Wno-sign-compare", "-I", str(source.parent), "-I", str(root / "libtandywin"),
        str(source), str(root / "libtandywin" / "LTWUI.C"), "-o", str(exe),
    ]
    # The shared Windows launcher preflights dependencies before starting GCC.
    subprocess.run(command, check=True, timeout=30)
    subprocess.run([str(exe)], check=True, env=runtime_environment(), timeout=10)
