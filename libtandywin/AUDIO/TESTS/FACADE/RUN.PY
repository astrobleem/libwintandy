"""Check the Win16 header layout and typed facade with host fixtures."""
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
parser.add_argument('--cc', help='Linux C compiler, or the shared Windows GCC launcher')
args = parser.parse_args()
compiler = compiler_command(parser, args.cc)
root = Path(__file__).resolve().parents[2]
source = Path(__file__).resolve().parent / 'layout.c'
facade_source = source.parent / 'facade.c'
if (root/'FACADE/LWAUDIO.H').read_bytes() != (root/'SOURCE/LWAUDIO.H').read_bytes():
    raise SystemExit('FACADE/LWAUDIO.H differs from SOURCE/LWAUDIO.H')
with tempfile.TemporaryDirectory(prefix='audio-facade-layout-') as temp:
    exe = Path(temp) / ('layout.exe' if os.name == 'nt' else 'layout')
    command = compiler + [
        '-x', 'c', '-std=c89', '-pedantic', '-Wall', '-Wextra', '-Werror',
        '-fpack-struct=2', '-I', str(source.parent), '-I', str(root/'SOURCE'),
        str(source), '-o', str(exe),
    ]
    subprocess.run(command, check=True, timeout=30)
    subprocess.run([str(exe)], check=True, env=runtime_environment(), timeout=30)
    facade_obj = Path(temp) / 'facade.o'
    facade_command = compiler + [
        '-x', 'c', '-std=c89', '-pedantic', '-Wall', '-Wextra', '-Werror',
        '-fpack-struct=2', '-I', str(facade_source.parent),
        '-I', str(root/'FACADE'), '-I', str(root/'SOURCE'),
        '-c', str(facade_source), '-o', str(facade_obj),
    ]
    subprocess.run(facade_command, check=True, timeout=30)
print('Win16 header layout and typed facade checks passed (host)')
