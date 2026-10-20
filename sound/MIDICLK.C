/* Shared Win16 timestamped MIDI producer. Foreground renderer only;
   no MIDI/DOS/Windows/synth function is called from the raw timer IRQ. */
#define WINVER 0x0300
#define MD_IMPLEMENTATION
#include <windows.h>
#include "MDAPI.H"
#include "MIDISYN.H"
#include "LTMATH.H"
#include "MIDIMSG.H"
#include "TIMECLK.H"
#include "TCBURST.H"
#include "TCXBASS.H"
#include "TCNOISE.H"
#include "LWMOD.H"
typedef WORD(FAR PASCAL *ZERO)(void);typedef WORD(FAR PASCAL *PUT)(TCFRAME FAR *);typedef WORD(FAR PASCAL *DEBUG)(WORD);typedef DWORD(FAR PASCAL *CLOCK)(void);
typedef struct {DWORD at,msg;} EVENT;
static EVENT events[1024];static WORD ehead,ecount,ah,acount,an[1024],frameN,lastSent;
static MSYN synth;static TCFRAME f;static MDINFO info;
static HTASK owner;static WORD handle,serial=1,haveFrame,pending,eof,sealed,started,haveTime,committed,policy,seeded,voiceFlags;static DWORD origin,when,watermark,lastAt,nextAt,lastClock,fraction,seedAt,clockMS,lastOutput;
static WORD livePending;static PUT rlive;
typedef WORD(FAR PASCAL *CONFIG)(WORD);
typedef WORD(FAR PASCAL *XPUT)(TCXFRAME FAR *);
typedef WORD(FAR PASCAL *XCAP)(WORD,TCXINFO FAR *,WORD);
static CONFIG speakerConfig;static XPUT speakerPush;static ZERO speakerStatus;static XCAP speakerCaps;
static WORD requestedSpeakerMode,renderSpeakerMode,extendedFrame;
static TCXFRAME extended;static TCXINFO speakerCap;
typedef WORD(FAR PASCAL *NGET)(WORD,TCNINFO FAR *,WORD);
typedef WORD(FAR PASCAL *NPUT)(TCXFRAME FAR *,WORD);
typedef WORD(FAR PASCAL *NLIVE)(TCFRAME FAR *,WORD);
static NGET noiseCaps;static NPUT noisePush;static NLIVE noiseLive;
static TCNINFO noiseCap;static WORD noiseEvent;
/* Exact foreground cycle conversion. The usual 8..55ms step needs one
   division; large/backward jumps retain the shared absolute conversion. */
static DWORD cycleMS,cycleValue;static WORD cycleFraction;
static DWORD cyclesFor(DWORD ms){DWORD delta=ms-cycleMS,n,q;
 if(ms>=cycleMS&&delta<1200UL){
  if(delta){n=delta*3579545UL+cycleFraction;q=lt_div16(n,3000);cycleValue+=q;cycleFraction=(WORD)(n-q*3000UL);}
 }else{cycleValue=MSCycles(ms);n=ms%3000UL;cycleFraction=(WORD)((n*545UL)%3000UL);}
 cycleMS=ms;return cycleValue;}
static HMODULE raw,pin;static char path[144];static ZERO ropen,rstart,rclose,rend,rresume,rpause,runpause,rflush,rrearm,rmask;static PUT rput;static DEBUG rd;static CLOCK rc;
static WORD valid(WORD h){return owner&&owner==GetCurrentTask()&&h==handle;}
static WORD load(void){WORD n,i,last=0;HMODULE self=GetModuleHandle("MIDICLK");n=GetModuleFileName(self,path,sizeof(path));if(!n||n>=sizeof(path)-13)return 1;pin=LoadLibrary(path);if(pin<32){pin=0;return 1;}
 for(i=0;i<n;i++)if(path[i]=='\\'||path[i]=='/')last=i+1;lstrcpy(path+last,"RAWCLK.DLL");raw=LoadLibrary(path);if(raw<32){raw=0;return 1;}
 if(!LWModuleMatches(raw,path))return 1;
 ropen=(ZERO)GetProcAddress(raw,(LPSTR)13L);rput=(PUT)GetProcAddress(raw,(LPSTR)2L);rstart=(ZERO)GetProcAddress(raw,(LPSTR)3L);rend=(ZERO)GetProcAddress(raw,(LPSTR)4L);rclose=(ZERO)GetProcAddress(raw,(LPSTR)5L);rd=(DEBUG)GetProcAddress(raw,(LPSTR)6L);rresume=(ZERO)GetProcAddress(raw,(LPSTR)11L);rc=(CLOCK)GetProcAddress(raw,(LPSTR)12L);rpause=(ZERO)GetProcAddress(raw,(LPSTR)14L);runpause=(ZERO)GetProcAddress(raw,(LPSTR)15L);rflush=(ZERO)GetProcAddress(raw,(LPSTR)16L);rrearm=(ZERO)GetProcAddress(raw,(LPSTR)17L);
 rmask=(ZERO)GetProcAddress(raw,(LPSTR)18L);rlive=(PUT)GetProcAddress(raw,(LPSTR)19L);
 speakerConfig=(CONFIG)GetProcAddress(raw,(LPSTR)29L);
 speakerPush=(XPUT)GetProcAddress(raw,(LPSTR)30L);
 speakerStatus=(ZERO)GetProcAddress(raw,(LPSTR)28L);
 speakerCaps=(XCAP)GetProcAddress(raw,(LPSTR)24L);
 noiseCaps=(NGET)GetProcAddress(raw,(LPSTR)31L);
 noisePush=(NPUT)GetProcAddress(raw,(LPSTR)32L);
 noiseLive=(NLIVE)GetProcAddress(raw,(LPSTR)33L);
 if(!noiseCaps||!noisePush||!noiseLive||
    noiseCaps(TCN_VERSION,&noiseCap,sizeof(noiseCap))||
    noiseCap.size!=sizeof(noiseCap)||noiseCap.version!=TCN_VERSION||
    noiseCap.features!=TCN_EXPLICIT_NOISE||noiseCap.frameBytes!=sizeof(TCXFRAME)||
    noiseCap.eventMask!=TCN_RETRIGGER||noiseCap.reserved)return 1;
 return !(rmask&&ropen&&rput&&rstart&&rend&&rclose&&rd&&rresume&&rc&&rpause&&runpause&&rflush&&rrearm);}
static void init(DWORD at){livePending=0;MSReset(&synth);synth.flags=voiceFlags;MSSpeakerMode(&synth,renderSpeakerMode);ehead=ecount=ah=acount=pending=eof=sealed=lastSent=haveTime=committed=seeded=0;haveFrame=0;origin=when=watermark=lastAt=at;lastClock=fraction=cycleMS=cycleValue=0;cycleFraction=0;clockMS=lastOutput=at;info.state=MD_OPEN;info.flags=(voiceFlags<<8)|policy;info.error=0;info.queued=0;info.position=at;info.accepted=info.delivered=info.coalesced=0;info.generation++;if(!info.generation)info.generation=1;}
static void advanceClock(void){DWORD d,n,r,now,q;
 if(info.state!=MD_RUNNING)return;now=rc();d=now-lastClock;lastClock=now;
 while(d){n=d>1000000UL?1000000UL:d;r=n*3000UL+fraction;q=lt_clock_div(r);clockMS+=q;info.position=clockMS;fraction=r-q*3579545UL;d-=n;}
 if(eof&&(LONG)(info.position-watermark)>0)info.position=watermark;
}
static void update(void){WORD delta;if(info.state!=MD_RUNNING)return;advanceClock();
 delta=rd(2)-lastSent;lastSent+=delta;while(delta--&&acount){info.delivered+=an[ah];ah=(ah+1)&1023;acount--;}}
static WORD build(void){WORD i;DWORD edge,env;EVENT *e;
 if(ecount&&(LONG)(events[ehead].at-when)<0)when=events[ehead].at;
 if(policy!=2&&haveFrame&&(LONG)(when-lastOutput)<8L)when=lastOutput+8UL;
 if(((LONG)(when-watermark)>0&&!eof)||(!haveTime&&!eof)||(!eof&&watermark-when<8UL))return 2;if(ecount&&(LONG)(events[ehead].at-when)>0&&events[ehead].at-when<8UL){when=events[ehead].at;if(!eof&&((LONG)(when-watermark)>0||watermark-when<8UL))return 2;}MSRender(&synth,when-origin);frameN=noiseEvent=0;
 while(ecount){e=&events[ehead];if((LONG)(e->at-when)>0)break;if(MSMessageDeferred(&synth,when-origin,e->msg))return 1;
 if((e->msg&255UL)==0x99UL&&((e->msg>>16)&127UL))noiseEvent|=TCN_RETRIGGER;
 if(e->at!=when)info.coalesced++;ehead=(ehead+1)&1023;ecount--;frameN++;}
 MSRender(&synth,when-origin);if(policy==2){DWORD outputAt=when;WORD lead;update();lead=(!haveFrame||(LONG)(lastOutput-clockMS)<120L)?250:8;if(info.state==MD_RUNNING&&(LONG)(outputAt-clockMS)<(LONG)lead)outputAt=clockMS+lead;if(haveFrame&&(LONG)(outputAt-lastOutput)<8L)outputAt=lastOutput+8UL;lastOutput=outputAt;f.at=cyclesFor(outputAt-origin);}else {lastOutput=when;f.at=cyclesFor(when-origin);}f.mask=15;f.noise=synth.noise;f.volume=0;
 for(i=0;i<4;i++){f.volume|=synth.volume[i]<<(i*4);if(i<3)f.p[i]=synth.p[i];}
 if(eof&&(LONG)(when-watermark)>=0){f.volume=65535;sealed=1;nextAt=when;}
 else {edge=when+8UL;env=origin+(synth.renderQuantum+1UL)*55UL;nextAt=env;
  if(ecount&&(LONG)(events[ehead].at-env)<8L)nextAt=events[ehead].at;
  if(policy!=2&&(LONG)(nextAt-edge)<0)nextAt=edge;if(eof&&(LONG)(nextAt-watermark)>0)nextAt=watermark;
  if(policy!=2&&(LONG)(nextAt-edge)<0){nextAt=edge;}}
 pending=1;return 0;}
static WORD fill(WORD budget){WORD r;while(budget--){if(acount==1024)return 0;if(!pending){if(sealed)return 0;r=build();if(r==2)return 0;if(r)return 1;}
 extended.frame=f;
 extended.pitDivisor=extendedFrame&&!sealed?synth.speakerDivisor:0;
 r=noisePush(&extended,noiseEvent);if(r==2)return 0;if(r)return 1;haveFrame=1;an[(ah+acount)&1023]=frameN;acount++;pending=0;when=nextAt;if(sealed)return rend()?1:0;}return 0;}
WORD MDAPI MDOpen(WORD FAR *out){HMODULE p;if(!out)return MD_BAD;if(owner)return MD_BUSY;if(load()){if(raw)FreeLibrary(raw);raw=0;p=pin;pin=0;if(p)FreeLibrary(p);return MD_IO;}
 if(ropen()){FreeLibrary(raw);raw=0;p=pin;pin=0;if(p)FreeLibrary(p);return MD_BUSY;}owner=GetCurrentTask();handle=serial++;if(!serial)serial=1;info.size=sizeof(info);info.version=MD_VERSION;info.capacity=1024;info.minSpacingMs=8;info.psgTones=3;info.noiseVoices=1;info.flags=0;started=policy=requestedSpeakerMode=renderSpeakerMode=extendedFrame=0;voiceFlags=3;init(0);*out=handle;return 0;}
WORD MDAPI MDPush(WORD h,DWORD gen,DWORD at,DWORD msg){
 if(!valid(h))return MD_BUSY;if(gen!=info.generation)return MD_STALE;if(policy!=2)advanceClock();if(policy!=2&&info.state==MD_RUNNING&&(LONG)(at-clockMS)<8L)return MD_LATE;if(eof||info.error||((LONG)(at-origin)<0)||at-origin>2147483647UL)return MD_BAD;
 if(committed&&((LONG)(at-watermark)<0||(at==watermark&&(LONG)(when-at)>0)))return MD_BAD;if(haveTime&&(LONG)(at-lastAt)<0)return MD_BAD;if(policy==0&&haveTime&&at!=lastAt&&at-lastAt<8UL)return MD_SPACING;if(ecount==1024)return MD_FULL;
 if(!MSValidMessage(msg,renderSpeakerMode))return MD_BAD;
 events[(ehead+ecount)&1023].at=at;events[(ehead+ecount)&1023].msg=msg;ecount++;lastAt=at;haveTime=1;info.accepted++;return 0;}
WORD MDAPI MDCommit(WORD h,DWORD gen,DWORD through,WORD flags){if(!valid(h))return MD_BUSY;if(gen!=info.generation)return MD_STALE;if(eof||flags>1||(LONG)(through-watermark)<0||(LONG)(through-origin)<0||through-origin>2147483647UL)return MD_BAD;

 if((flags&MD_EOF)&&haveTime&&(LONG)(through-lastAt)<0)return MD_BAD;watermark=through;committed=1;if(flags&MD_EOF)eof=1;return 0;}
WORD MDAPI MDStart(WORD h){WORD r;if(!valid(h))return MD_BUSY;if(info.state!=MD_OPEN)return MD_BAD;
 if(livePending){
  if(!pending&&build())return MD_IO;
  if(!pending)return MD_IO;
  if(noiseLive(&f,noiseEvent))return MD_BUSY;
  haveFrame=1;ah=0;an[0]=frameN;acount=1;pending=0;when=nextAt;
  if(fill(8)){info.error=MD_IO;info.state=MD_FAILED;return MD_IO;}
  livePending=0;
  if(!started){r=rstart();if(r)return MD_BUSY;}
  started=1;info.state=MD_RUNNING;return 0;
 }
 if(fill(1024)||!acount)return MD_IO;r=started?rrearm():rstart();if(r)return MD_BUSY;started=1;info.state=MD_RUNNING;return 0;}
WORD MDAPI MDPump(WORD h){if(!valid(h))return MD_BUSY;update();if(info.state==MD_PAUSED||info.state==MD_OPEN)return 0;
 if(rd(6)){info.error=MD_IO;info.state=MD_FAILED;return MD_IO;}if(rd(7)){info.state=MD_DONE;info.position=watermark;return 0;}
 if(rd(1)&&!rd(20))rresume();if(fill(8)){info.error=MD_IO;info.state=MD_FAILED;return MD_IO;}return 0;}
WORD MDAPI MDPause(WORD h){if(!valid(h)||info.state!=MD_RUNNING)return MD_BAD;update();if(rpause())return MD_BUSY;info.state=MD_PAUSED;return 0;}
WORD MDAPI MDResume(WORD h){if(!valid(h)||info.state!=MD_PAUSED)return MD_BAD;if(runpause())return MD_BUSY;info.state=MD_RUNNING;return 0;}
WORD MDAPI MDReset(WORD h,DWORD at){if(!valid(h))return MD_BUSY;if(rflush())return MD_BUSY;init(at);return 0;}
WORD MDAPI MDClose(WORD h){HMODULE p;if(!valid(h))return MD_BUSY;if(rclose())return MD_BUSY;owner=0;handle=0;started=0;FreeLibrary(raw);raw=0;p=pin;pin=0;if(p)FreeLibrary(p);return 0;}
WORD MDAPI MDStatus(WORD h,MDINFO FAR *out,WORD size){WORD i;BYTE *s;BYTE FAR *d;if(!valid(h))return MD_BUSY;if(!out||size<sizeof(info))return MD_BAD;MDPump(h);info.queued=ecount;s=(BYTE *)&info;d=(BYTE FAR *)out;for(i=0;i<sizeof(info);i++)d[i]=s[i];return 0;}
int MDAPI WEP(int reason){return 1;}

WORD MDAPI MDSeed(WORD h,DWORD gen,DWORD at,DWORD msg){if(!valid(h))return MD_BUSY;if(gen!=info.generation)return MD_STALE;if(info.state!=MD_OPEN||haveTime||committed||acount||(LONG)(at-origin)>0||origin-at>2147483647UL||(seeded&&(LONG)(at-seedAt)<0))return MD_BAD;if(MSMessage(&synth,at-origin,msg))return MD_BAD;seeded=1;seedAt=at;return 0;}
WORD MDAPI MDTiming(WORD h,WORD mode){if(!valid(h))return MD_BUSY;if(info.state!=MD_OPEN||haveTime||committed||mode>2)return MD_BAD;policy=mode;info.flags=(voiceFlags<<8)|mode;return 0;}

DWORD MDAPI MDVersion(void){return 0x00080101UL;}

WORD MDAPI MDClock(WORD h,DWORD FAR *out){if(!valid(h))return MD_BUSY;if(!out)return MD_BAD;update();*out=clockMS;return 0;}
WORD MDAPI MDVoiceMode(WORD h,WORD flags){if(!valid(h))return MD_BUSY;if(info.state!=MD_OPEN||haveTime||committed||flags==2||(flags&~3))return MD_BAD;voiceFlags=synth.flags=flags;MSInvalidate(&synth);info.flags=(flags<<8)|policy;return 0;}

WORD MDAPI MDDebug(WORD h,WORD q){if(!valid(h))return 0xffff;if(q==0)return rmask();if(q==1)return 1;if(q==2)return rd(12);if(q==3)return rd(6);if(q==4)return synth.speakerDivisor;
 if(q==5)return synth.speakerNote;if(q==6)return synth.speakerAttacks;if(q==8)return synth.flags;
 if(q==7)return synth.bassFolded;return 0xffff;}

/* Raw timeline remains live while owner stages seed/events. MDStart performs
   the atomic prepared-frame replacement and immediate complete-state output.
   MDReset/MDClose abort staging with the ordinary acknowledged mute/flush. */
WORD MDAPI MDBeginLive(WORD h,DWORD at){if(!valid(h))return MD_BUSY;
 if(extendedFrame||!rlive||info.state==MD_PAUSED||info.state==MD_FAILED||info.state==MD_DONE)return MD_BAD;
 init(at);livePending=1;return 0;}

WORD MDAPI MDSpeakerMode(WORD h,WORD mode){WORD status;
 if(!valid(h))return MD_BUSY;
 if(mode>2||info.state!=MD_OPEN||haveTime||committed||acount)return MD_BAD;
 if(requestedSpeakerMode)return requestedSpeakerMode==mode?0:MD_BUSY;
 if(!mode)return 0;
 requestedSpeakerMode=mode;status=TCX_UNAVAILABLE;
 if(speakerConfig&&speakerPush&&speakerStatus&&speakerCaps&&
    !speakerCaps(TCX_VERSION,&speakerCap,sizeof(speakerCap))&&
    speakerCap.size==sizeof(speakerCap)&&speakerCap.version==TCX_VERSION){
  if(speakerConfig(mode==1)){requestedSpeakerMode=0;return MD_IO;}
  extendedFrame=1;status=speakerStatus();
 }
 renderSpeakerMode=mode==1&&status==TCX_AVAILABLE?1:2;
 MSSpeakerMode(&synth,renderSpeakerMode);return 0;
}
WORD MDAPI MDSpeakerStatus(WORD h){
 if(!valid(h)||!requestedSpeakerMode)return TCX_UNAVAILABLE;
 if(requestedSpeakerMode==2)return TCX_DISABLED;
 return extendedFrame&&speakerStatus?speakerStatus():TCX_UNAVAILABLE;
}
