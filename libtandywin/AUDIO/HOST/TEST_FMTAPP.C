/* Host test of the real FMTAPP.C open/pump/close path with fake Win16 file,
   memory and RAWCLK services. Not a timing or audio test.
   Usage: test_fmtapp TMPDIR REF.VGM --good F... [--bad F...] */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "../SOURCE/FMTAPP.C"

#define MAXH 16
static FILE *files[MAXH];static int openHandles,tempMade,tempLive;
static char tmpdir[300],tempNames[64][320];static int tempCount;
static DWORD writeLimit,written,allocs;static int noTemp,failLoad,busy;
static unsigned queued,hwEnded,closes,starts;static unsigned long fnv,bursts;
static int checks,failures;
#define C(x) do{checks++;if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);failures++;}}while(0)

static HFILE mkh(FILE *f){int i;if(!f)return HFILE_ERROR;for(i=1;i<MAXH;i++)if(!files[i]){files[i]=f;openHandles++;return i;}fclose(f);return HFILE_ERROR;}
HFILE _lopen(const char *p,int m){return mkh(fopen(p,m&2?"r+b":"rb"));}
HFILE _lcreat(const char *p,int a){(void)a;return mkh(fopen(p,"w+b"));}
LONG _llseek(HFILE h,LONG o,int w){if(h<1||h>=MAXH||!files[h])return -1;if(fseek(files[h],o,w==2?SEEK_END:w==1?SEEK_CUR:SEEK_SET))return -1;return ftell(files[h]);}
WORD _lread(HFILE h,void *b,WORD n){if(h<1||h>=MAXH||!files[h])return 0xffff;return (WORD)fread(b,1,n,files[h]);}
WORD _lwrite(HFILE h,const void *b,WORD n){if(h<1||h>=MAXH||!files[h])return 0xffff;
 if(writeLimit&&written+n>writeLimit){n=(WORD)(writeLimit>written?writeLimit-written:0);}
 written+=n;return (WORD)fwrite(b,1,n,files[h]);}
HFILE _lclose(HFILE h){if(h<1||h>=MAXH||!files[h])return HFILE_ERROR;fclose(files[h]);files[h]=0;openHandles--;return 0;}
int GetTempFileName(BYTE d,const char *pre,WORD u,char *out){FILE *f;(void)d;(void)u;if(noTemp)return 0;
 sprintf(out,"%s/%s%04x.TMP",tmpdir,pre,++tempMade);f=fopen(out,"wb");if(!f)return 0;fclose(f);
 strcpy(tempNames[tempCount++],out);tempLive++;return 1;}
HFILE OpenFile(const char *p,OFSTRUCT *o,WORD f){(void)o;if(f&OF_DELETE){if(!remove(p))tempLive--;return 1;}return HFILE_ERROR;}
HGLOBAL GlobalAlloc(WORD f,DWORD n){void *p=malloc(n);(void)f;if(p)allocs++;return (HGLOBAL)p;}
void *GlobalLock(HGLOBAL h){return (void *)h;}int GlobalUnlock(HGLOBAL h){(void)h;return 0;}
HGLOBAL GlobalFree(HGLOBAL h){free((void *)h);allocs--;return 0;}
HTASK GetCurrentTask(void){return 7;}
static WORD fOpen(void){return busy?1:0;}
static WORD fCaps(WORD v,TCBINFO *i,WORD n){(void)v;(void)n;i->size=sizeof(*i);i->version=TCB_VERSION;i->features=TCB_ORDERED|TCB_RAW_NOISE;i->capacity=TCB_CAPACITY;i->maxWrites=16;i->reserved=0;i->minCycles=FX_MIN;i->clockNumerator=3579545UL;i->clockDenominator=3;return 0;}
static void mix(unsigned long v){fnv=(fnv^v)*16777619UL;}
static WORD fPut(TCBURST *b){unsigned i;if(queued>=TCB_CAPACITY)return 2;mix(b->at);mix(b->count);for(i=0;i<b->count;i++)mix(b->data[i]);bursts++;queued++;return 0;}
static WORD fEnd(void){hwEnded=1;return 0;}static WORD fStart(void){starts++;return 0;}static WORD fClose(void){closes++;return 0;}
static WORD fDbg(WORD q){if(q==0)return (WORD)queued;if(q==7)return hwEnded&&!queued;return 0;}
static WORD fZero(void){return 0;}static DWORD fClock(void){return 0;}
HMODULE LoadLibrary(const char *p){(void)p;return failLoad?0:33;}void FreeLibrary(HMODULE m){(void)m;}
void *GetProcAddress(HMODULE m,LPSTR p){(void)m;switch((uintptr_t)p){case 20:return fCaps;case 21:return fOpen;case 22:return fPut;case 23:return fEnd;
 case 3:return fStart;case 5:return fClose;case 6:return fDbg;case 11:case 14:case 15:return fZero;case 12:return fClock;}return 0;}

static unsigned char *refbuf;static unsigned long refsize;
static int refread(void *u,unsigned long at,unsigned char *o,unsigned short n){(void)u;if(at>refsize||n>refsize-at)return 0;memcpy(o,refbuf+at,n);return 1;}
static unsigned long refhash,refbursts;static void loopcheck(void);
static void reference(const char *p){FILE *f=fopen(p,"rb");static FXCORE c;FXBURST b;unsigned i;int r;
 if(!f){perror(p);exit(2);}fseek(f,0,SEEK_END);refsize=ftell(f);fseek(f,0,SEEK_SET);refbuf=malloc(refsize);fread(refbuf,1,refsize,f);fclose(f);
 C(FXOpen(&c,refread,0,refsize,FX_VGM)==0);fnv=2166136261UL;
 while((r=FXNext(&c,&b))>0){unsigned j;mix(b.at);mix(b.count);for(j=0;j<b.count;j++)mix(b.data[j]);refbursts++;(void)i;}
 C(r==0);refhash=fnv;if(c.looped)loopcheck();printf("reference: %lu bytes, %lu bursts, hash %08lx, looped %u\n",refsize,refbursts,refhash,c.looped);}

/* Loop continuity: the repeat must replay the loop section byte-for-byte,
   shifted by one loop length (+-1 PIT cycle of carried fraction). */
static void loopcheck(void){static FXCORE c;static FXBURST rec[40000];static unsigned long off[40000];FXBURST b;unsigned n=0,i,a=0,k;int r;unsigned long shift=0,end1=0;
 FXOpen(&c,refread,0,refsize,FX_VGM);
 while((r=FXNext(&c,&b))>0&&n<40000){rec[n]=b;off[n]=c.sourceOffset;n++;}
 C(r==0&&c.looped&&c.loopOffset>65535UL);
 /* first traversal ends where the source offset first goes backwards to the loop start */
 for(i=1;i<n;i++)if(off[i]<off[i-1]){a=i;break;}
 if(!a)return;
 for(i=0;i<a&&off[i]<c.loopOffset;i++);
 /* i = first burst of the loop section in pass one; a = first burst of pass two */
 C(n-a==a-i||n-a==a-i+1);shift=rec[a].at-rec[i].at;(void)end1;
 for(k=0;k<n-a&&k<a-i;k++){unsigned j;long d=(long)(rec[a+k].at-rec[i+k].at)-(long)shift;
  if(rec[a+k].count!=rec[i+k].count||(d>1||d<-1)||off[a+k]!=off[i+k]){C(0);break;}
  for(j=0;j<rec[a+k].count;j++)if(rec[a+k].data[j]!=rec[i+k].data[j]){C(0);break;}}
 printf("loop check: loop start offset %lu (> 65535), loop section %u bursts, repeat shift %lu cycles\n",c.loopOffset,a-i,shift);}

static int clean(const char *what){int i,ok=1;
 if(openHandles){printf("  %s: %d handles open\n",what,openHandles);ok=0;}
 if(tempLive){printf("  %s: %d temp files live\n",what,tempLive);ok=0;}
 if(allocs){printf("  %s: %lu allocations live\n",what,(unsigned long)allocs);ok=0;}
 for(i=0;i<tempCount;i++){FILE *f=fopen(tempNames[i],"rb");if(f){fclose(f);printf("  %s: %s remains\n",what,tempNames[i]);ok=0;}}
 return ok;}
static void reset(void){queued=hwEnded=0;bursts=0;fnv=2166136261UL;noTemp=failLoad=busy=0;writeLimit=written=0;}
/* Returns FmtOpen's code, or 100 if it opened but did not play to EOF. */
static int play(const char *p,unsigned stopAfter){WORD r;unsigned n=0;reset();r=FmtOpen((LPSTR)p,FX_VGM);
 if(r){C(!FmtClose());return r;}
 for(;;){r=FmtPump();queued=0;if(r!=1)break;if(stopAfter&&++n>=stopAfter){C(!FmtClose());return 0;}if(n>2000000U)return 100;}
 C(!FmtClose());return r==0?0:100;}

int main(int argc,char **argv){int i,bad=0;char path[400];
 if(argc<5||strcmp(argv[3],"--good")){fprintf(stderr,"usage\n");return 2;}
 strcpy(tmpdir,argv[1]);FmtInit((LPSTR)"C:\\");reference(argv[2]);
 for(i=4;i<argc;i++){
  if(!strcmp(argv[i],"--bad")){bad=1;continue;}
  strcpy(path,argv[i]);
  if(!bad){int r=play(path,0);
   printf("%s: open/play=%d bursts=%lu hash=%08lx %s\n",path,r,bursts,fnv,fnv==refhash&&bursts==refbursts?"MATCH":"DIFF");
   C(r==0);C(fnv==refhash);C(bursts==refbursts);C(clean(path));
   /* Stop mid-song, then reopen the same song: resources released, replay matches. */
   r=play(path,40);C(r==0);C(clean("stop"));
   r=play(path,0);C(r==0&&fnv==refhash);C(clean("reopen"));
  }else{int r=play(path,0);printf("%s: refused code=%d reason=\"%s\"\n",path,r,FmtReason());C(r!=0&&r!=100);C(clean(path));}
 }
 /* Failure injection, on the first VGZ among the good inputs. */
 for(i=4;i<argc&&strcmp(argv[i],"--bad");i++){int r;size_t n=strlen(argv[i]);
  if(n<4||strcmp(argv[i]+n-4,".VGZ"))continue;
  strcpy(path,argv[i]);
#define INJECT(label,setup,expect) do{reset();setup;r=FmtOpen((LPSTR)path,FX_VGM);printf("%s: code=%d \"%s\"\n",label,r,FmtReason());C(expect);C(!FmtClose());C(clean(label));}while(0)
  INJECT("no RAWCLK",failLoad=1,r==8);
  INJECT("sound busy",busy=1,r==7);
  INJECT("disk full",writeLimit=5000,r!=0);
  INJECT("no temp dir",noTemp=1,r!=0);
  break;
 }
 printf("%s checks=%d failures=%d\n",failures?"FAIL":"PASS",checks,failures);return failures!=0;}
