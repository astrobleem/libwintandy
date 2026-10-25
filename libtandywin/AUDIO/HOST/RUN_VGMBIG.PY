"""Developer test runner: large-file VGM/VGZ through the real FMTAPP.C path.

Generates the synthetic >64 KiB inputs, builds HOST/TEST_FMTAPP.C against the
packaged SOURCE with a host Win16 stub, and runs it. Host results say nothing
about 8088 speed or audible playback.

  python RUN_VGMBIG.PY --out NEWDIR [--cc "gcc"]
  (AI host: --cc "python C:/.../gcc-ucrt64.py --"; the C files are compiled as C)
"""
import argparse
import shlex
import subprocess
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
src = here.parent / "SOURCE"
CORE = ["FMTCORE.C", "VGM.C", "VGMGRP.C", "MODPLAY.C", "GUNZIP.C"]


def run(cmd, **kw):
    r = subprocess.run(cmd, capture_output=True, text=True, **kw)
    sys.stdout.write(r.stdout)
    sys.stderr.write(r.stderr)
    return r.returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--cc", default="gcc")
    a = ap.parse_args()
    if a.out.exists():
        ap.error("--out must be new")
    inputs = a.out / "inputs"
    tmp = a.out / "tmp"
    tmp.mkdir(parents=True)
    exe = a.out / "test_fmtapp.exe"
    if run([sys.executable, str(here / "MAKE_BIGVGM.PY"), str(inputs)]):
        return 1
    cc = shlex.split(a.cc) + ["-x", "c", "-std=gnu99", "-w", "-I",
                             str(here / "FMTSTUB"), "-I", str(src),
                             str(here / "TEST_FMTAPP.C")]
    cc += [str(src / n) for n in CORE] + ["-o", str(exe)]
    if run(cc):
        return 1
    rc = 0
    plain = str(inputs / "PLAIN.VGM")
    big = str(inputs / "BIG.VGM")
    rc |= run([str(exe), str(tmp), plain, "--good", plain])
    rc |= run([str(exe), str(tmp), big, "--good", big])
    return rc


if __name__ == "__main__":
    sys.exit(main())
