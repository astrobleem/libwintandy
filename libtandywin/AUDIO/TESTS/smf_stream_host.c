/* Streaming vs in-memory SMF read must yield identical event sequences.
   Usage: smf_stream_host file.mid ... (files up to 65535 bytes). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SMF.H"
static FILE *fp;static unsigned long reads,maxread;
static unsigned rd(SMF_ULONG off,unsigned char *d,unsigned n){
 reads++;if(n>maxread)maxread=n;
 if(fseek(fp,(long)off,SEEK_SET))return 0;return fread(d,1,n,fp)==n?n:0;}
static unsigned char mem[65535U];
int main(int argc,char **argv){int k,bad=0;
 for(k=1;k<argc;k++){unsigned n;SMF_EVENT a,b;static SMF_EVENT ea[20000];unsigned na=0,i;int r,ok=1;
  fp=fopen(argv[k],"rb");if(!fp){puts("cannot open");return 1;}
  n=(unsigned)fread(mem,1,sizeof(mem),fp);reads=maxread=0;
  if(!smf_open(mem,n)){printf("%s: memory open failed: %s\n",argv[k],smf_error());bad++;continue;}
  while((r=smf_next(&a))==1&&na<20000)ea[na++]=a;
  smf_close();
  if(!smf_open_stream(rd,(SMF_ULONG)n)){printf("%s: stream open failed: %s\n",argv[k],smf_error());bad++;continue;}
  for(i=0;i<na&&ok;i++){if(smf_next(&b)!=1||memcmp(&ea[i],&b,sizeof(b)))ok=0;}
  if(ok&&smf_next(&b)!=0)ok=0;
  printf("%s: %s events=%u reads=%lu maxread=%lu\n",argv[k],ok?"MATCH":"MISMATCH",na,reads,maxread);
  if(!ok)bad++;smf_close();fclose(fp);}
 printf("%s\n",bad?"FAIL":"PASS");return bad!=0;}
