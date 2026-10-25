"""Run C89 tests on the current packaged production sources."""
from pathlib import Path
import argparse
import json
import os
import shutil
import subprocess
import sys

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
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
source, host = root/'SOURCE', root/'HOST'
compiler = compiler_command(parser, args.cc)
out = args.out.resolve()
if out.exists():
    parser.error('--out must be new')
out.mkdir(parents=True)
cases = [
    ('formats', [host/'TEST_FMTS.C'] + [source/(name+'.C') for name in
                 ('FMTCORE', 'VGM', 'VGMGRP', 'MODPLAY', 'GUNZIP')]),
    ('burst', [host/'TEST_BURST.C']),
    ('paired', [host/'TEST_PAIR.C']),
    ('mod', [host/'TEST_MOD.C', source/'MODPLAY.C']),
]
rows = []
for name, files in cases:
    exe = out/(name+('.exe' if os.name == 'nt' else ''))
    command = compiler + ['-x', 'c', '-std=c89', '-Wall', '-Wextra', '-pedantic',
                          '-I', str(source), '-I', str(host/'STUB')]
    command += [str(path) for path in files] + ['-o', str(exe)]
    built = subprocess.run(command, capture_output=True, text=True, timeout=25)
    log = built.stdout + built.stderr
    row = {'case': name, 'command': command, 'compile': built.returncode, 'passed': False}
    if built.returncode == 0:
        ran = subprocess.run([str(exe)], cwd=root/'INPUTS', env=runtime_environment(),
                             capture_output=True, text=True, timeout=15)
        log += '\n' + ran.stdout + ran.stderr
        row.update(exit=ran.returncode, passed=ran.returncode == 0, output=ran.stdout)
    (out/(name+'.LOG')).write_text(log)
    rows.append(row)
    print(json.dumps(row))
(out/'RESULT.JSON').write_text(json.dumps({
    'results': rows, 'scope': 'Production source with host hardware mocks'}, indent=2)+'\n')
raise SystemExit(0 if all(row['passed'] for row in rows) else 1)
