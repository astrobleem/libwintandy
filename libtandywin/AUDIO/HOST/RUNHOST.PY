"""Run C89 tests on the packaged production sources (32-bit unsigned long ABI)."""
from pathlib import Path
import argparse,hashlib,json,os,shutil,subprocess
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--cc',default=shutil.which('gcc') or 'C:/msys64/ucrt64/bin/gcc.exe')
p.add_argument('--out',type=Path,required=True)
a=p.parse_args();root=Path(__file__).resolve().parents[1];s=root/'SOURCE';h=root/'HOST'
out=a.out.resolve()
if out.exists():p.error('--out must be new')
out.mkdir(parents=True);env=os.environ.copy();cc=Path(a.cc).resolve()
env['PATH']=str(cc.parent)+os.pathsep+env.get('PATH','')
cases=[('formats',[h/'TEST_FMTS.C']+[s/(n+'.C') for n in ['FMTCORE','VGM','VGMGRP','MODPLAY','GUNZIP']]),
 ('burst',[h/'TEST_BURST.C']),('mod',[h/'TEST_MOD.C',s/'MODPLAY.C'])]
rows=[]
for name,files in cases:
 exe=out/(name+'.exe');cmd=[str(cc),'-x','c','-std=c89','-Wall','-Wextra','-pedantic','-I',str(s),'-I',str(h/'STUB')]+[str(x) for x in files]+['-o',str(exe)]
 r=subprocess.run(cmd,env=env,capture_output=True,text=True,timeout=25)
 log=r.stdout+r.stderr;row={'case':name,'command':cmd,'compile':r.returncode,'passed':False}
 if r.returncode==0:
  r=subprocess.run([str(exe)],cwd=root/'INPUTS',env=env,capture_output=True,text=True,timeout=15)
  log+='\n'+r.stdout+r.stderr;row.update(exit=r.returncode,passed=r.returncode==0,output=r.stdout)
 (out/(name+'.LOG')).write_text(log);rows.append(row);print(json.dumps(row))
(out/'RESULT.JSON').write_text(json.dumps({'results':rows,'scope':'Packaged production source; host fake hardware is not native timing proof'},indent=2))
assert all(x['passed'] for x in rows)
