"""Exercise actual RAWCLK ownership and output code with fake host hardware."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys

HERE = Path(__file__).resolve().parent
DEFAULT_GCC = Path(r"C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py")
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--gcc-launcher', type=Path, default=DEFAULT_GCC)
args = parser.parse_args()
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=False)
if os.name == 'nt':
    if args.gcc_launcher.resolve() != DEFAULT_GCC:
        parser.error('Windows host GCC must use the shared verified launcher')
    cc = [sys.executable, str(args.gcc_launcher), '--']
else:
    cc = ['gcc']
env = os.environ.copy()
if os.name == 'nt':
    env['PATH'] = r'C:\msys64\ucrt64\bin' + os.pathsep + env.get('PATH', '')
rows = []
for name, source, extra in [('winamp', 'TEST_BURST.C', []), ('speaker', 'TEST_BURST.C', [
        '-DCLOCK_SOURCE="../../../sound/TIMECLK.C"', '-DSPEAKER_CLOCK']),
        ('paired', 'TEST_PAIR.C', []), ('fxmerge', 'TEST_FX.C', [])]:
    exe = out / (name + ('.exe' if os.name == 'nt' else ''))
    command = cc + ['-x', 'c', '-std=c89', '-Wall', '-Wextra',
                    '-I', str(HERE/'STUB'), *extra,
                    str(HERE/source), '-o', str(exe)]
    # GCC's own launcher preflights before any compiler child is created.
    built = subprocess.run(command, capture_output=True, text=True, timeout=30)
    log = built.stdout + built.stderr
    row = {'name': name, 'command': command, 'compile': built.returncode,
           'passed': False}
    if built.returncode == 0:
        ran = subprocess.run([str(exe)], env=env, capture_output=True,
                             text=True, timeout=20)
        log += ran.stdout + ran.stderr
        row.update(exit=ran.returncode, passed=ran.returncode == 0,
                   output=ran.stdout.strip(),
                   executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest())
    (out/(name+'.LOG')).write_text(log)
    rows.append(row)
    print(json.dumps(row))
(out/'RESULT.JSON').write_text(json.dumps({
    'scope': 'Actual RAWCLK source, stub Win16/hardware; native IRQ timing separate',
    'source_sha256': {
        path.resolve().relative_to(HERE.parents[2].resolve()).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in (HERE/'TEST_BURST.C', HERE/'TEST_PAIR.C', HERE/'TEST_FX.C', HERE/'STUB/windows.h',
                     *(HERE/'../SOURCE'/name for name in
                       ('TIMECLK.C', 'TIMECLK.H', 'PSGOWN.H', 'TCBURST.H',
                        'TCXBASS.H', 'PITLINK.H', 'WINHOOK.ASM', 'TIMECLK.DEF', 'FXMERGE.C', 'FXMERGE.H')),
                     *(HERE/'../../../sound'/name for name in
                       ('TIMECLK.C', 'TIMECLK.H', 'PSGOWN.H', 'TCBURST.H',
                        'TCXBASS.H', 'TCNOISE.H', 'PITLINK.H')))},
    'results': rows}, indent=2)+'\n')
raise SystemExit(0 if all(row['passed'] for row in rows) else 1)
