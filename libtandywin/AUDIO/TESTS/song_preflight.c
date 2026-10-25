/* Developer tool: classify every channel event of SMF files the way the MCITNDY
   preflight does, then apply the admitted ones to the renderer. */
#include <stdio.h>
#include <stdlib.h>
#include "MIDISYN.H"
#include "MIDISKIP.H"
#include "SMF.H"
static unsigned char buf[60000];
int main(int argc,char **argv){int k;
 for(k=1;k<argc;k++){FILE *f=fopen(argv[k],"rb");unsigned n;SMF_EVENT e;static MSYN s;
  unsigned ok=0,skip=0,bad=0,fail=0,ctl[128],i,first=0;int r;DWORD m;
  if(!f){puts("cannot open");continue;}n=(unsigned)fread(buf,1,sizeof(buf),f);fclose(f);
  for(i=0;i<128;i++)ctl[i]=0;
  printf("%s: %u bytes ",argv[k],n);
  if(n>32768U){puts("REFUSED size>32768 (MC_BADFILE)");continue;}
  if(!smf_open(buf,n)){puts("REFUSED smf_open");continue;}
  MSReset(&s);s.flags=1;
  while((r=smf_next(&e))==1){m=(DWORD)e.status|((DWORD)e.data1<<8)|((DWORD)e.data2<<16);
   if((e.status&0xf0)==0xb0)ctl[e.data1&127]++;
   if(MSSkipMessage(m)){skip++;continue;}
   if(!MSValidMessage(m,0)){if(!bad)first=e.status|(e.data1<<8);bad++;continue;}
   ok++;if(MSMessage(&s,e.ms,m))fail++;}
  printf("ok=%u skipped=%u REFUSED=%u applyfail=%u",ok,skip,bad,fail);
  if(bad)printf(" (first %04x)",first);
  printf(" cc:");for(i=0;i<128;i++)if(ctl[i])printf(" %u:%u",i,ctl[i]);puts("");
  smf_close();}
 return 0;}
