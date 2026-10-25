/* Actual MIDICLK producer + synth, fake paired provider; no native claim. */
#include <stdio.h>
#include <stdlib.h>
#include "mdhost/windows.h"
#include "../SOURCE/MIDICLK.C"

static WORD task=1,held,running,activeClock,backendStatus,ended;
static WORD capsMode=1,hardwareFree=1,npairs,nframes,retries,liveCalls;
static TCXPAIR pairs[1024],livePair;
static TCFRAME frames[1024];
static DWORD ticks;
static unsigned checks;

static void ck(int yes,const char *why){
 checks++;if(!yes){fprintf(stderr,"FAIL %s check%u\n",why,checks);exit(1);}}
HTASK GetCurrentTask(void){return task;}
HMODULE GetModuleHandle(const char *n){(void)n;return 33;}
WORD GetModuleFileName(HMODULE m,char *p,WORD n){
 (void)m;(void)n;strcpy(p,"C:\\MMLAB\\MIDICLK.DLL");return (WORD)strlen(p);}
HMODULE LoadLibrary(const char *n){(void)n;return 34;}
WORD FreeLibrary(HMODULE m){(void)m;return 0;}
static WORD rawOpen(void){
 if(held)return 1;held=1;running=activeClock=npairs=nframes=retries=liveCalls=0;
 ticks=0;ended=0;backendStatus=hardwareFree?TCX_AVAILABLE:TCX_BUSY;return 0;}
static WORD rawPut(TCFRAME *p){if(nframes==1024)return 2;frames[nframes++]=*p;return 0;}
static WORD rawStart(void){running=activeClock=1;return 0;}
static WORD rawEnd(void){if(!npairs)ended=1;return 0;}
static WORD rawClose(void){held=running=activeClock=0;return 0;}
static WORD rawDebug(WORD n){return n==1?running:n==20?activeClock:n==7?ended:0;}
static DWORD rawClock(void){return ticks;}
static WORD rawPause(void){activeClock=0;return 0;}
static WORD rawResume(void){activeClock=1;return 0;}
static WORD rawFlush(void){running=activeClock=npairs=nframes=ended=0;ticks=0;return 0;}
static WORD rawMask(void){return 0;}
static WORD rawLive(TCFRAME *p){frames[0]=*p;nframes=1;return 0;}
static WORD pairGet(WORD version,TCXPAIRINFO *out,WORD bytes){
 if(version!=TCXP_VERSION||bytes!=sizeof(*out))return 3;
 out->size=sizeof(*out);out->version=TCXP_VERSION;
 out->features=capsMode==2?TCXP_SHARED_NOISE:
  TCXP_SHARED_NOISE|TCXP_ATOMIC_LIVE|TCXP_FALLBACK;
 out->frameBytes=sizeof(TCXPAIR);out->minDivisor=151;
 out->maxDivisor=65535;out->reserved=0;return 0;}
static WORD frameMode(WORD enabled){return !held||enabled!=1;}
static WORD bassSet(WORD enabled){
 if(!held||enabled!=1)return 1;retries++;
 backendStatus=hardwareFree?TCX_AVAILABLE:TCX_BUSY;return 0;}
static WORD bassStatus(void){return backendStatus;}
static WORD pairPut(TCXPAIR *p){if(npairs==1024)return 2;pairs[npairs++]=*p;return 0;}
static WORD pairLive(TCXPAIR *p){livePair=*p;liveCalls++;npairs=0;return 0;}
void *GetProcAddress(HMODULE module,LPSTR name){
 (void)module;switch((uintptr_t)name){
 case 13:return rawOpen;case 2:return rawPut;case 3:case 17:return rawStart;
 case 4:return rawEnd;case 5:return rawClose;case 6:return rawDebug;
 case 11:case 15:return rawResume;case 12:return rawClock;case 14:return rawPause;
 case 16:return rawFlush;case 18:return rawMask;case 19:return rawLive;
 case 27:return bassSet;case 28:return bassStatus;case 29:return frameMode;
 case 34:return capsMode?pairGet:0;case 35:return pairPut;case 36:return pairLive;
 }return 0;}

#define MSG(s,c,a,b) ((DWORD)((s)|(c))|((DWORD)(a)<<8)|((DWORD)(b)<<16))
#define ON(n) MSG(0x90,0,n,127)
#define OFF(n) MSG(0x80,0,n,0)
#define CC(c,v) MSG(0xb0,0,c,v)
#define BEND(v) MSG(0xe0,0,(v)&127,(v)>>7)
static WORD h;static MDINFO state;
static void openMode(WORD caps){
 capsMode=caps;ck(MDOpen(&h)==0,"open");
 ck(MDVersion()==0x00080100UL,"original MD version");
 ck(MDStatus(h,&state,sizeof(state))==0&&state.flags==0x100,"default voice flags1");
 ck(MDVoiceMode(h,0)==0&&MDTiming(h,1)==0,"flat focused rendering");}
static void push(DWORD at,DWORD message){
 ck(MDPush(h,state.generation,at,message)==0,"original event admitted");
 ck(events[(ehead+ecount-1)&1023].msg==message,"original event preserved");}
static void start(DWORD end){
 ck(MDCommit(h,state.generation,end,MD_EOF)==0&&MDStart(h)==0,"start prepared output");}
static const TCXPAIR *at(DWORD ms){WORD i,index=0;
 for(i=0;i<npairs;i++)if(pairs[i].frame.at<=MSCycles(ms))index=i;return &pairs[index];}
static WORD fallbackTone(const TCXPAIR *p){WORD i;
 for(i=0;i<3;i++)if(((p->fallbackVolume>>(i*4))&15)!=15)return p->fallbackP[i];
 return 0;}
static void closeMode(void){ck(MDClose(h)==0&&!held,"close releases same owner");}

int main(void){const TCXPAIR *p;MSYN original,shadow;WORD i;
 /* Older and incomplete providers keep the ordinary, original-key fallback. */
 for(i=0;i<2;i++){
  openMode(i?2:0);ck(!paired,"unverified provider uses PSG path");
  push(100,ON(33));push(200,OFF(33));start(300);
  ck(!npairs&&nframes,"ordinary frames emitted");
  ck(synth.key[0]==34&&synth.speakerMode==2,"ordinary source key retained");closeMode();
 }
 openMode(1);ck(paired&&sizeof(TCXPAIR)==28,"verified paired ABI");
 push(100,ON(33));push(110,MSG(0x90,9,38,127));push(200,OFF(33));start(300);p=at(150);
 ck(npairs&&!nframes,"paired output only");
 ck(p->pitDivisor==21694&&!p->flags,"original A1 pitch prepared for PIT");
 ck((p->frame.volume&4095)==4095&&fallbackTone(p)==1017,"prepared PSG alternative");
 ck(p->frame.noise==5&&(p->frame.volume&0xf000)==(p->fallbackVolume&0xf000),"shared percussion state");
 ck(pairs[npairs-1].pitDivisor==0&&pairs[npairs-1].frame.volume==65535&&
    pairs[npairs-1].fallbackVolume==65535,"EOF mutes both outputs");closeMode();
 for(i=0;i<2;i++){
  WORD note=i?13:0;openMode(1);push(100,ON(note));start(300);p=at(150);
  ck(p->flags==TCXP_FORCE_FOLD&&!p->pitDivisor,"0..13 never octave-raised on PIT");
  ck(fallbackTone(p)>0&&synth.speakerNote==note+1,"unrepresentable original key retained");
  ck(folded.key[0]==note+1,"shadow retains original key");closeMode();
 }
 openMode(1);push(50,BEND(0));push(100,ON(14));
 push(200,BEND(8192));push(250,CC(7,0));push(300,CC(7,127));start(400);
 p=at(150);ck(p->flags==TCXP_FORCE_FOLD&&!p->pitDivisor,"out-of-range bend forces fold");
 p=at(225);ck(!p->flags&&p->pitDivisor==65009,"representable original note14 restores PIT");
 p=at(275);ck(!p->flags&&!p->pitDivisor&&p->fallbackVolume==65535,"controller zero mutes both");
 p=at(350);ck(!p->flags&&p->pitDivisor==65009,"controller rearm preserves original pitch");closeMode();
 openMode(1);push(50,CC(64,127));push(100,ON(33));push(200,OFF(33));
 push(300,CC(121,0));start(400);
 ck(at(250)->pitDivisor==21694&&fallbackTone(at(250))==1017,"pedal holds both states");
 ck(!at(350)->pitDivisor&&at(350)->fallbackVolume==65535,"controller reset lifts both states");
 ck(!synth.speakerNote&&!synth.speakerFallback&&!folded.note[0],"no stale primary/shadow voice");closeMode();
 /* A capability retry is foreground work; queued pairs stay complete. */
 hardwareFree=0;openMode(1);push(100,ON(33));start(400);
 ck(paired&&backendStatus==TCX_BUSY&&at(150)->pitDivisor==21694,
    "busy lease keeps prepared original and fallback");
 ck(MDPump(h)==0&&retries>0,"busy retry in foreground");
 hardwareFree=1;ck(MDPump(h)==0&&backendStatus==TCX_AVAILABLE,"lease handoff retry");closeMode();
 /* Atomic live replacement carries a complete pair and retains event keys. */
 openMode(1);push(100,ON(33));ck(MDCommit(h,state.generation,400,0)==0&&MDStart(h)==0,"prime live clock");
 ck(MDBeginLive(h,500)==0,"begin same-owner live update");
 ck(MDStatus(h,&state,sizeof(state))==0,"read live generation");
 ck(MDSeed(h,state.generation,450,ON(0))==0,"original unsupported bass seeded");
 push(500,ON(64));ck(MDCommit(h,state.generation,700,MD_EOF)==0&&MDStart(h)==0,"commit live pair");
 ck(liveCalls==1&&!livePair.frame.at&&livePair.flags==TCXP_FORCE_FOLD&&
    !livePair.pitDivisor&&fallbackTone(&livePair)>0,"atomic live force-fallback pair");
 ck(MDReset(h,0)==0&&MDBeginLive(h,0)==0,"live after acknowledged reset");
 ck(MDStatus(h,&state,sizeof(state))==0,"reset live generation");
 push(0,ON(14));ck(MDCommit(h,state.generation,100,MD_EOF)==0&&MDStart(h)==0,
    "reset live rearms same clock");
 ck(activeClock&&livePair.pitDivisor==65009,"reset live primary pitch");
 ck(MDReset(h,0)==0&&MDBeginLive(h,0)==0,"empty live replacement");
 ck(MDStatus(h,&state,sizeof(state))==0,"empty live generation");
 push(0,OFF(14));ck(MDCommit(h,state.generation,0,MD_EOF)==0&&MDStart(h)==0,
    "immediate live EOF completes");
 ck(!livePair.pitDivisor&&livePair.frame.volume==65535&&
    livePair.fallbackVolume==65535&&ended,"immediate live EOF mutes both");
 backendStatus=TCX_BUSY;i=retries;
 ck(MDPump(h)==0&&info.state==MD_DONE&&retries==i,"completed EOF never reacquires PIT");closeMode();
 /* Pure synth state clears unsupported pitch and bend fallback on release. */
 MSReset(&original);MSReset(&shadow);MSSpeakerMode(&original,1);MSSpeakerMode(&shadow,2);
 original.flags=shadow.flags=0;
 ck(!MSMessage(&original,0,ON(0))&&!MSMessage(&shadow,0,ON(0)),"dual original event");
 ck(original.speakerFallback&&original.speakerNote==1&&shadow.key[0]==1,"note0 state identity");
 ck(!MSMessage(&original,1,CC(120,0))&&!MSMessage(&shadow,1,CC(120,0)),"all sound off both");
 ck(!original.speakerNote&&!original.speakerFallback&&shadow.volume[0]==15,"all sound off clears both");
 printf("PASS paired MIDI producer checks=%u; foreground source/fake provider\n",checks);return 0;
}
