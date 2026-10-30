"""Strict C89 actual-source canvas/example checks with host mocks, not native QA."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shlex
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
repository = root.parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=False)
out = args.out.resolve()
launcher = Path(r'C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py')
if os.name == 'nt':
    if not launcher.is_file():
        parser.error('Shared Windows GCC launcher missing: ' + str(launcher))
    compiler = [sys.executable, str(launcher), '--']
else:
    compiler = shlex.split(os.environ.get('CC', 'cc'))
flags = ['-x', 'c', '-std=c89', '-pedantic', '-Wall', '-Wextra', '-Wshadow',
         '-Wstrict-prototypes', '-Werror']
env = os.environ.copy()
if os.name == 'nt':
    env['PATH'] = r'C:\msys64\ucrt64\bin' + os.pathsep + env.get('PATH', '')
record = {'evidence': 'actual source with host USER/GDI mocks only; not native qualification',
          'checks': {}}
inputs = [root / 'WTCANVAS.C', root / 'WTCANVAS.H', root / 'TESTS/windows.h',
          root / 'TESTS/CANVTEST.C', root / 'TESTS/EXAMPLE.C',
          root / 'TESTS/EXSTUB/windows.h', repository / 'examples/CANVAS30/CANVAS.C']
record['source'] = {p.relative_to(repository).as_posix(): {
    'bytes': len(p.read_bytes()), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
    for p in inputs}
for name, source, stub in [('CANVTEST', 'CANVTEST.C', root / 'TESTS'),
                           ('EXAMPLE', 'EXAMPLE.C', root / 'TESTS/EXSTUB')]:
    exe = out / (name + '.EXE')
    # The actual example uses native int-to-RECT assignments; host RECT is
    # deliberately short while host int is wider. Library/contracts also pass
    # Wconversion; do not suppress all warnings merely to model that ABI.
    check_flags = flags + (['-Wconversion'] if name == 'CANVTEST' else [])
    command = compiler + check_flags + ['-I' + str(stub), '-I' + str(root),
        str(root / 'WTCANVAS.C'), str(root / 'TESTS' / source), '-o', str(exe)]
    compiled = subprocess.run(command, capture_output=True, timeout=60)
    (out / (name + '-COMPILE.LOG')).write_bytes(compiled.stdout + compiled.stderr)
    item = {'command': command, 'compile_returncode': compiled.returncode}
    record['checks'][name] = item
    if not compiled.returncode:
        run = subprocess.run([str(exe)], capture_output=True, timeout=30, env=env)
        (out / (name + '-RUN.LOG')).write_bytes(run.stdout + run.stderr)
        item['returncode'] = run.returncode
        item['output'] = (run.stdout + run.stderr).decode('ascii', errors='replace')
        sys.stdout.buffer.write(run.stdout)
        sys.stderr.buffer.write(run.stderr)
        code = run.returncode
    else:
        sys.stderr.buffer.write(compiled.stdout + compiled.stderr)
        code = compiled.returncode
    (out / 'RESULT.JSON').write_text(json.dumps(record, indent=2) + '\n', encoding='ascii')
    if code:
        raise SystemExit(code)
