"""Exercise the unchanged Win16 implementation using original host USER mocks."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--cc", help="host C compiler (defaults to cc on PATH or MSYS2 UCRT64)")
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = Path(__file__).resolve().parent / "LTWUITST.C"
compiler = args.cc or shutil.which("cc") or r"C:\msys64\ucrt64\bin\cc.exe"
env = os.environ.copy()
env["PATH"] = str(Path(compiler).resolve().parent) + os.pathsep + env.get("PATH", "")
with tempfile.TemporaryDirectory(prefix="libwintandy-ltwui-") as temp:
    exe = Path(temp) / ("ltwui-test.exe" if os.name == "nt" else "ltwui-test")
    command = [
        compiler, "-x", "c", "-std=c89", "-pedantic", "-Wall", "-Wextra", "-Werror",
        # Win16 original compares int with sizeof; host size_t is unsigned/64-bit.
        "-Wno-sign-compare", "-I", str(source.parent), "-I", str(root / "libtandywin"),
        str(source), str(root / "libtandywin" / "LTWUI.C"), "-o", str(exe),
    ]
    subprocess.run(command, check=True, env=env, timeout=30)
    subprocess.run([str(exe)], check=True, env=env, timeout=10)
