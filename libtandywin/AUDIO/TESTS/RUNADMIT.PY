"""Test actual one-pass SMF/MCI foreground admission on the host."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parent / 'SOURCE'
LAUNCHER = Path(r'C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py')
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--song', type=Path, action='append', default=[])
a = p.parse_args()
if os.name == 'nt':
    if not LAUNCHER.is_file():
        p.error('Shared Windows GCC launcher is missing')
    compiler = [sys.executable, str(LAUNCHER), '--']
else:
    compiler = ['gcc']
out = a.out.resolve()
out.mkdir(parents=True, exist_ok=False)
exe = out / ('admission.exe' if os.name == 'nt' else 'admission')
command = compiler + ['-x', 'c', '-std=c89', '-DSMF_HOST',
    '-Wno-int-to-pointer-cast', '-Wno-pointer-to-int-cast',
    '-I', str(HERE / 'mcihost'), '-I', str(SOURCE),
    str(HERE / 'mci_admission_host.c'), str(SOURCE / 'SMF.C'), '-o', str(exe)]
built = subprocess.run(command, capture_output=True, text=True, timeout=30)
row = {'command': command, 'compile': built.returncode, 'passed': False}
log = built.stdout + built.stderr
if built.returncode == 0:
    env = os.environ.copy()
    if os.name == 'nt':
        env['PATH'] = r'C:\msys64\ucrt64\bin' + os.pathsep + env.get('PATH', '')
    ran = subprocess.run([str(exe)]+[str(song.resolve()) for song in a.song],
                         env=env, capture_output=True, text=True, timeout=15)
    log += ran.stdout + ran.stderr
    row.update(exit=ran.returncode, output=ran.stdout.strip(), passed=ran.returncode == 0)
(out / 'ADMISSION.LOG').write_text(log)
row['retained_songs'] = {str(song.resolve()): {'bytes': song.stat().st_size,
    'sha256': hashlib.sha256(song.read_bytes()).hexdigest()} for song in a.song}
row['source_sha256'] = {n: hashlib.sha256((SOURCE/n).read_bytes()).hexdigest()
                        for n in ('SMF.C', 'SMF.H', 'MCITNDY.C')}
(out / 'RESULT.JSON').write_text(json.dumps(row, indent=2) + '\n')
print(json.dumps(row))
raise SystemExit(0 if row['passed'] else 1)
