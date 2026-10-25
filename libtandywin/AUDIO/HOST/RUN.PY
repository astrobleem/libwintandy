"""Run actual foreground/provider source against the existing host mocks."""
from pathlib import Path
import argparse
import hashlib
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
parser.add_argument('--out', type=Path, default=Path(__file__).resolve().parent)
options = parser.parse_args()
root = Path(__file__).resolve().parents[1]
out = options.out.resolve()
compiler = compiler_command(parser, options.cc)
out.mkdir(parents=True, exist_ok=True)
cases = [
    ('raw-format', ['-std=c89', 'TESTS/host.c', 'SOURCE/RAWFILE.C', 'SOURCE/TESTCORE.C']),
    ('midi-transport', ['-std=c99', '-DSYNTH_HOST', '-I', 'TESTS/mdhost', '-I', 'SOURCE',
                        'TESTS/mdhost/driver.c', 'SOURCE/MIDISYN.C', 'SOURCE/INSTR.C']),
    ('synth', ['-std=c99', '-DSYNTH_HOST', '-I', 'SOURCE',
               'TESTS/synth_host.c', 'SOURCE/MIDISYN.C', 'SOURCE/INSTR.C']),
    ('live', ['-std=c99', '-DSYNTH_HOST', '-I', 'TESTS/mdhost', '-I', 'SOURCE',
              'TESTS/live_host.c', 'SOURCE/MIDISYN.C', 'SOURCE/INSTR.C']),
    ('beat', ['-std=c99', 'TESTS/beat_host.c']),
]
results = []
for name, flags in cases:
    exe = out / (name + ('.exe' if os.name == 'nt' else ''))
    command = compiler + ['-x', 'c', *flags, '-o', str(exe)]
    built = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=30)
    log = built.stdout + built.stderr
    result = {'name': name, 'command': command, 'compile': built.returncode, 'passed': False}
    if built.returncode == 0:
        ran = subprocess.run([str(exe)], cwd=root, env=runtime_environment(),
                             capture_output=True, text=True, timeout=10)
        log += '\n' + ran.stdout + ran.stderr
        result.update(exit_code=ran.returncode, passed=ran.returncode == 0,
                      output=ran.stdout.strip(),
                      exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest())
    (out/(name+'-plain.log')).write_text(log)
    results.append(result)
    print(name, result.get('output', log[-500:]))
record = {'compiler_command': compiler,
          'scope': 'Actual foreground/provider source with mock hardware',
          'results': results}
(out/'PLAIN.JSON').write_text(json.dumps(record, indent=2)+'\n')
raise SystemExit(0 if all(row['passed'] for row in results) else 1)
