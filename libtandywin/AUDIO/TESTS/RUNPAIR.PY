"""Exercise actual foreground MIDI producer/controller source on the host."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent/'SOURCE'
WINDOWS_GCC = Path(r'C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py')
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
if os.name == 'nt':
    if not WINDOWS_GCC.is_file():
        parser.error('Shared GCC launcher is missing')
    compiler = [sys.executable, str(WINDOWS_GCC), '--']
else:
    compiler = ['gcc']
out = args.out.resolve()
out.mkdir(parents=True, exist_ok=False)
runtime_env = os.environ.copy()
if os.name == 'nt':
    runtime_env['PATH'] = r'C:\msys64\ucrt64\bin'+os.pathsep+runtime_env.get('PATH','')
rows = []
for name, source in (
        ('paired','mdpair_host.c'), ('controllers','mdctl_host.c'),
        ('transport','mdhost/driver.c'), ('synth','synth_host.c')):
    exe = out/(name+('.exe' if os.name == 'nt' else ''))
    command = compiler+['-x','c','-std=c99','-DSYNTH_HOST',
        '-I',str(HERE/'mdhost'),'-I',str(SOURCE),str(HERE/source),
        str(SOURCE/'MIDISYN.C'),str(SOURCE/'INSTR.C'),'-o',str(exe)]
    built = subprocess.run(command, capture_output=True, text=True, timeout=30)
    log = built.stdout+built.stderr
    row = {'name': name, 'command': command, 'compile': built.returncode, 'passed': False}
    if built.returncode == 0:
        ran = subprocess.run([str(exe)], env=runtime_env,
                             capture_output=True, text=True, timeout=15)
        log += ran.stdout+ran.stderr
        row.update(exit=ran.returncode, passed=ran.returncode==0, output=ran.stdout.strip())
    (out/(name+'.LOG')).write_text(log)
    rows.append(row)
    print(json.dumps(row))
sources = ('MIDICLK.C','MIDISYN.C','MIDISYN.H','INSTR.C','INSTR.H',
           'TCXBASS.H','LTPBASE.H','LTMATH.H','MIDIMSG.H','PITNOTES.H')
(out/'RESULT.JSON').write_text(json.dumps({
    'scope': 'Actual foreground MIDI source; fake provider is not native timing evidence',
    'source_sha256': {name: hashlib.sha256((SOURCE/name).read_bytes()).hexdigest()
                      for name in sources},
    'results': rows}, indent=2)+'\n')
raise SystemExit(0 if all(row['passed'] for row in rows) else 1)
