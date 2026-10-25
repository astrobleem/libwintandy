/* Developer tool: classify every channel event of SMF files the way the MCITNDY
   preflight does, then apply the admitted ones to the renderer. */
#include <stdio.h>
#include <stdlib.h>
#include "MIDISYN.H"
#include "MIDISKIP.H"
#include "SMF.H"
static FILE *fp;
static unsigned rd(SMF_ULONG off,unsigned char *d,unsigned n){
 if(fseek(fp,(long)off,SEEK_SET))return 0;return fread(d,1,n,fp)==n?n:0;}
int main(int argc,char **argv){int k;
 for(k=1;k<argc;k++){long n;SMF_EVENT e;static MSYN s;
  unsigned ok=0,skip=0,bad=0,fail=0,ctl[128],i,first=0;int r;DWORD m;
  fp=fopen(argv[k],"rb");if(!fp){puts("cannot open");continue;}fseek(fp,0,SEEK_END);n=ftell(fp);
  for(i=0;i<128;i++)ctl[i]=0;
  printf("%s: %ld bytes ",argv[k],n);
  if(!smf_open_stream(rd,(SMF_ULONG)n)){printf("REFUSED smf_open: %s\n",smf_error());fclose(fp);continue;}
  MSReset(&s);s.flags=1;MSSpeakerMode(&s,2);
  while((r=smf_next(&e))==1){m=(DWORD)e.status|((DWORD)e.data1<<8)|((DWORD)e.data2<<16);
   if((e.status&0xf0)==0xb0)ctl[e.data1&127]++;
   if(MSSkipMessage(m)){skip++;continue;}
   if(!MSValidMessage(m,1)){if(!bad)first=e.status|(e.data1<<8);bad++;continue;}
   ok++;if(MSMessage(&s,e.ms,m))fail++;}
  printf("ok=%u skipped=%u REFUSED=%u applyfail=%u",ok,skip,bad,fail);
  if(bad)printf(" (first %04x)",first);
  printf(" folded=%u dur=%lus events=%u cc:",s.bassFolded,(unsigned long)smf_duration_ms()/1000,smf_event_count());for(i=0;i<128;i++)if(ctl[i])printf(" %u:%u",i,ctl[i]);puts("");
  smf_close();fclose(fp);}
 return 0;}
