#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "../SOURCE/MIDICLK.C"
static TCFRAME liveFrame;static unsigned flushes;
static WORD task=1,lease,running,paused,done,ending,sentN,nframes,reads,physical;static DWORD ticks;static TCFRAME frames[1024],observed[2048];static unsigned recording,observedN;
HTASK GetCurrentTask(void){return task;}HMODULE GetModuleHandle(const char *n){return 33;}WORD GetModuleFileName(HMODULE m,char *p,WORD n){strcpy(p,"C:\\MMLAB\\MIDICLK.DLL");return strlen(p);}HMODULE LoadLibrary(const char *n){return 34;}WORD FreeLibrary(HMODULE m){return 0;}
static WORD openraw(void){if(lease)return 1;lease=1;sentN=nframes=running=done=ending=paused=0;ticks=0;return 0;}
static WORD putraw(TCFRAME *p){if(nframes==1024)return 2;frames[nframes++]=*p;if(recording&&observedN<2048)observed[observedN++]=*p;return 0;}static WORD startraw(void){running=1;return 0;}static WORD endraw(void){ending=1;return 0;}static WORD closeraw(void){lease=running=0;return 0;}static WORD debugraw(WORD q){reads++;if(q==2)return sentN;if(q==7)return done;if(q==12)return physical;return 0;}static DWORD clockraw(void){reads++;return ticks;}static WORD pauseraw(void){paused=1;return 0;}static WORD unpauseraw(void){paused=0;return 0;}static WORD flushraw(void){flushes++;sentN=nframes=running=ending=done=paused=0;ticks=0;return 0;}static WORD maskraw(void){return 0;}
static WORD applylive(TCFRAME *p){if(!lease)return 1;liveFrame=*p;sentN=1;nframes=done=ending=paused=0;ticks=0;physical++;return 0;}
void *GetProcAddress(HMODULE m,LPSTR p){switch((uintptr_t)p){case 13:return openraw;case 2:return putraw;case 3:case 17:return startraw;case 4:return endraw;case 5:return closeraw;case 6:return debugraw;case 11:return startraw;case 12:return clockraw;case 14:return pauseraw;case 15:return unpauseraw;case 16:return flushraw;case 18:return maskraw;case 19:return applylive;}return 0;}
#include "../SOURCE/MIDISKIP.H"
/* Controller behaviour of the packaged MIDICLK path: events in, PSG frames
   out. Voice mode 0 (flat, no envelope) so only controllers move output. */
static unsigned tests;
static void ck(int ok,const char *name){tests++;if(!ok){fprintf(stderr,"FAIL %s test %u\n",name,tests);exit(1);}}
static WORD h;static DWORD gen;static MDINFO inf;
typedef struct {DWORD at;DWORD msg;} EV;
#define M(s,ch,a,b) ((DWORD)((s)|(ch))|((DWORD)(a)<<8)|((DWORD)(b)<<16))
#define ON(ch,n) M(0x90,ch,n,127)
#define OFF(ch,n) M(0x80,ch,n,0)
#define CC(ch,c,v) M(0xb0,ch,c,v)
#define BEND(ch,v) M(0xe0,ch,(v)&127,(v)>>7)
static void play(const EV *e,unsigned n,DWORD end){unsigned i;
 ck(MDReset(h,0)==0,"reset");MDStatus(h,&inf,sizeof(inf));gen=inf.generation;
 ck(MDVoiceMode(h,0)==0,"flat");
 for(i=0;i<n;i++)ck(MDPush(h,gen,e[i].at,e[i].msg)==0,"push");
 ck(MDCommit(h,gen,end,1)==0&&MDStart(h)==0,"start");}
static const TCFRAME *at(DWORD ms){unsigned i,r=0;DWORD c=MSCycles(ms);
 for(i=0;i<nframes;i++)if(frames[i].at<=c)r=i;return &frames[r];}
static unsigned vol(DWORD ms,unsigned t){return (at(ms)->volume>>(t*4))&15;}
static void E(EV *a,unsigned i,DWORD t,DWORD m){a[i].at=t;a[i].msg=m;}
int main(void){unsigned i,j;EV a[16];WORD x;
 ck(MDOpen(&h)==0,"open");
 E(a,0,100,ON(0,60));E(a,1,200,OFF(0,60));play(a,2,300);
 ck(vol(150,0)==0&&vol(250,0)==15,"baseline on/off");
 x=at(150)->p[0];
 /* sustain */
 E(a,0,50,CC(0,64,127));E(a,1,100,ON(0,60));E(a,2,200,OFF(0,60));E(a,3,300,CC(0,64,0));play(a,4,400);
 ck(vol(150,0)==0&&vol(250,0)==0&&vol(350,0)==15,"sustain hold+lift");
 E(a,0,50,CC(1,64,127));E(a,1,100,ON(0,60));E(a,2,200,OFF(0,60));play(a,3,300);
 ck(vol(250,0)==15,"sustain per channel");
 E(a,0,50,CC(0,64,63));E(a,1,100,ON(0,60));E(a,2,200,OFF(0,60));play(a,3,300);
 ck(vol(250,0)==15,"sustain threshold");
 /* CC7 / CC11 */
 E(a,0,50,CC(0,7,64));E(a,1,100,ON(0,60));play(a,2,300);ck(vol(150,0)==3,"CC7=64");
 E(a,0,50,CC(0,11,64));play(a,2,300);ck(vol(150,0)==3,"CC11=64");
 E(a,0,30,CC(0,7,64));E(a,1,50,CC(0,11,64));E(a,2,100,ON(0,60));play(a,3,300);ck(vol(150,0)==6,"CC7*CC11");
 E(a,0,50,CC(0,7,0));E(a,1,100,ON(0,60));play(a,2,300);ck(vol(150,0)==15,"CC7=0 silent");
 E(a,0,100,ON(0,60));E(a,1,200,CC(0,11,64));E(a,2,300,CC(0,11,127));play(a,3,400);
 ck(vol(150,0)==0&&vol(250,0)==3&&vol(350,0)==0,"live expression");
 E(a,0,30,CC(0,7,127));E(a,1,40,CC(1,7,32));E(a,2,100,ON(0,60));E(a,3,110,ON(1,67));play(a,4,300);
 ck(vol(150,0)==0&&vol(150,1)==6&&at(150)->p[0]!=at(150)->p[1],"two channels");
 /* pitch bend */
 E(a,0,50,BEND(0,16383));E(a,1,100,ON(0,60));play(a,2,300);ck(at(150)->p[0]<x,"bend up");
 E(a,0,50,BEND(0,0));play(a,2,300);ck(at(150)->p[0]>x,"bend down");
 E(a,0,50,BEND(0,8192));play(a,2,300);ck(at(150)->p[0]==x,"bend centre");
 E(a,0,100,ON(0,60));E(a,1,200,BEND(0,16383));E(a,2,300,BEND(0,8192));play(a,3,400);
 ck(at(150)->p[0]==x&&at(250)->p[0]<x&&at(350)->p[0]==x,"live bend");
 E(a,0,50,BEND(1,16383));E(a,1,100,ON(0,60));play(a,2,300);ck(at(150)->p[0]==x,"bend per channel");
 /* CC123 vs CC120 with pedal */
 E(a,0,50,CC(0,64,127));E(a,1,100,ON(0,60));E(a,2,200,CC(0,123,0));E(a,3,300,CC(0,64,0));play(a,4,400);
 ck(vol(150,0)==0&&vol(250,0)==0&&vol(350,0)==15,"CC123 pedal down");
 E(a,2,200,CC(0,120,0));play(a,4,400);ck(vol(250,0)==15,"CC120 pedal down");
 E(a,0,100,ON(0,60));E(a,1,200,CC(0,123,0));play(a,2,300);ck(vol(250,0)==15,"CC123 no pedal");
 E(a,0,100,ON(0,60));E(a,1,110,ON(1,64));E(a,2,200,CC(0,123,0));play(a,3,300);
 ck(vol(250,0)==15&&vol(250,1)==0,"CC123 scoped");
 /* CC121 */
 E(a,0,20,CC(0,7,64));E(a,1,30,CC(0,11,32));E(a,2,40,BEND(0,16383));E(a,3,50,CC(0,64,127));
 E(a,4,60,CC(0,121,0));E(a,5,100,ON(0,60));E(a,6,200,OFF(0,60));play(a,7,300);
 ck(vol(150,0)==3,"CC121 keeps CC7, resets CC11");
 ck(at(150)->p[0]==x,"CC121 resets bend");
 ck(vol(250,0)==15,"CC121 clears pedal");
 E(a,0,50,CC(0,64,127));E(a,1,100,ON(0,60));E(a,2,200,OFF(0,60));E(a,3,300,CC(0,121,0));play(a,4,400);
 ck(vol(250,0)==0&&vol(350,0)==15,"CC121 lifts held");
 /* stealing: pedal-held voice goes first; new owner's controllers apply */
 E(a,0,10,CC(0,64,127));E(a,1,20,CC(1,7,32));E(a,2,100,ON(0,60));E(a,3,110,ON(0,62));E(a,4,120,ON(0,64));
 E(a,5,200,OFF(0,62));E(a,6,300,ON(1,72));E(a,7,400,CC(0,64,0));play(a,8,500);
 ck(vol(250,0)==0&&vol(250,1)==0&&vol(250,2)==0,"three held");
 ck(vol(350,0)==0&&vol(350,1)==6&&vol(350,2)==0,"steal released voice, new channel gain");
 ck(vol(450,0)==0&&vol(450,1)==6&&vol(450,2)==0,"pedal lift leaves stolen voice");
 /* plain steal protects lowest key */
 E(a,0,100,ON(0,45));E(a,1,110,ON(0,64));E(a,2,120,ON(0,67));E(a,3,130,ON(0,72));play(a,4,300);
 ck(vol(200,0)==0&&vol(200,1)==0&&vol(200,2)==0,"steal sounding");
 /* percussion and program change */
 E(a,0,100,M(0x90,9,38,127));E(a,1,110,ON(0,60));play(a,2,300);
 ck(at(150)->noise!=0&&vol(150,3)!=15&&vol(150,0)==0,"percussion");
 E(a,0,30,CC(9,7,64));E(a,1,100,M(0x90,9,38,127));play(a,2,300);
 ck(vol(150,3)!=15,"drum CC7");
 E(a,0,50,M(0xc0,0,16,0));E(a,1,100,ON(0,60));play(a,2,300);ck(vol(150,0)==0,"program change");
 /* EOF with pedal down */
 E(a,0,50,CC(0,64,127));E(a,1,100,ON(0,60));E(a,2,200,OFF(0,60));play(a,3,400);
 ck(vol(250,0)==0,"pedal held at end");ck(frames[nframes-1].volume==65535,"EOF sealed mute");
 /* Stop/reopen and reset: defaults return */
 E(a,0,20,CC(0,7,32));E(a,1,30,BEND(0,16383));E(a,2,40,CC(0,64,127));play(a,3,100);
 ck(MDClose(h)==0&&MDOpen(&h)==0,"close/open");
 E(a,0,100,ON(0,60));E(a,1,200,OFF(0,60));play(a,2,300);
 ck(vol(150,0)==0&&at(150)->p[0]==x&&vol(250,0)==15,"reopen defaults");
 E(a,0,20,CC(0,7,32));E(a,1,30,BEND(0,16383));E(a,2,40,CC(0,64,127));play(a,3,100);
 E(a,0,100,ON(0,60));E(a,1,200,OFF(0,60));play(a,2,300);
 ck(vol(150,0)==0&&at(150)->p[0]==x&&vol(250,0)==15,"reset defaults");
 /* admission */
 {MSYN s;unsigned c,v;DWORD m;
  static const unsigned ccs[]={0,1,7,10,11,32,64,91,93,120,121,123};
  MSReset(&s);
  for(i=0;i<16;i++){
   for(j=0;j<sizeof(ccs)/sizeof(*ccs);j++)for(v=0;v<128;v+=127){m=CC(i,ccs[j],v);
    if(MSValidMessage(m,0))ck(MSMessage(&s,100,m)==0,"valid CC applies");}
   for(v=0;v<16384;v+=4095)ck(MSValidMessage(BEND(i,v),0)&&MSMessage(&s,100,BEND(i,v))==0,"bend applies");
   ck(MSMessage(&s,100,M(0xc0,i,5,0))==0,"program applies");}
  for(c=0;c<128;c++){m=CC(0,c,64);ck(MSValidMessage(m,0)||MSSkipMessage(m)||c==0||c==32,"CC classified");}
  ck(MSSkipMessage(M(0xa0,0,60,40))&&MSSkipMessage(M(0xd0,0,40,0))&&MSSkipMessage(CC(0,6,12))
   &&MSSkipMessage(CC(0,38,0))&&MSSkipMessage(CC(0,100,0))&&MSSkipMessage(CC(0,101,0))
   &&MSSkipMessage(CC(0,98,0))&&MSSkipMessage(CC(0,71,64)),"skip list");
  ck(!MSSkipMessage(CC(0,7,100))&&!MSSkipMessage(CC(0,64,127))&&!MSSkipMessage(BEND(0,100))
   &&!MSSkipMessage(CC(0,0,0)),"valid not skipped");
  ck(!MSSkipMessage(CC(0,0,1))&&!MSValidMessage(CC(0,0,1),0)&&!MSSkipMessage(CC(0,32,5))
   &&!MSValidMessage(ON(0,44),0)&&!MSSkipMessage(ON(0,44))&&!MSValidMessage(M(0x90,9,82,100),0),"refused list");
  MDReset(h,0);MDStatus(h,&inf,sizeof(inf));gen=inf.generation;
  ck(MDPush(h,gen,10000,M(0xa0,0,60,40))==MD_BAD&&MDPush(h,gen,10000,CC(0,6,0))==MD_BAD,"push refuses unskipped");}
 ck(MDClose(h)==0,"close");
 printf("PASS controller path checks=%u\n",tests);return 0;}
