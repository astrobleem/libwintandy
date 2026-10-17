#define TC_HOST 1
#include "../SOURCE/TIMECLK.C"
#include <stdio.h>
static int failures,checks,inIRQ,unsafe;static WORD traceBytes[4096],traceCount,hi,task=1,sound;
static DWORD now;
#define C(x) do{checks++;if(!(x)){printf("FAIL line%d: %s\n",__LINE__,#x);failures++;}}while(0)
static void api(void){if(inIRQ)unsafe++;}
HTASK GetCurrentTask(void){api();return task;}HMODULE GetModuleHandle(const char *p){api();(void)p;return 33;}
WORD GetModuleFileName(HMODULE h,char *p,WORD n){api();(void)h;(void)n;strcpy(p,"RAWCLK.DLL");return 10;}
HMODULE LoadLibrary(const char *p){api();(void)p;return 33;}WORD FreeLibrary(HMODULE h){api();(void)h;return 0;}
FARPROC GetProcAddress(HMODULE h,LPSTR p){api();(void)h;(void)p;return NullEntry;}
WORD GetWinFlags(void){api();return 0;}int OpenSound(void){api();if(sound)return -1;sound=1;return 3;}
void CloseSound(void){api();sound=0;}void StopSound(void){api();}void Yield(void){api();}
WORD CreateSystemTimer(WORD n,FARPROC p){api();(void)n;(void)p;return 7;}void KillSystemTimer(WORD n){api();(void)n;}
void TimerEntry(void){}void NullEntry(void){}
WORD HiInstall(WORD n){(void)n;hi=1;return 0;}WORD HiStop(void){hi=0;return 0;}
DWORD HiRead(void){return now;}WORD HiStatus(void){return 0;}WORD HiMetric(WORD n){return n==8?1:n==9?hi:0;}
WORD HiRestart(void){hi=1;return 0;}WORD HiDetach(void){return 0;}WORD HiPeriod(void){return 9546;}
void HiPrepare(WORD n){(void)n;}void HiProgram(WORD n){(void)n;}void HiAudio(WORD n){(void)n;}
WORD SaveIRQ(void){return 0;}void RestoreIRQ(WORD n){(void)n;}void PsgByte(WORD n){if(traceCount<4096)traceBytes[traceCount++]=n;}
DWORD ReadStamp(void){return now;}
static void tick(DWORD t){now=t;inIRQ=1;Tick();inIRQ=0;}
int main(void){TCBINFO cap;TCBURST b,copy;TCFRAME f;WORD i,n;
 C(sizeof(TCFRAME)==16);C(sizeof(TCBURST)==24||sizeof(TCBURST)==22); /* host DWORD alignment differs */
 C(TCBGetCaps(TCB_VERSION,&cap,sizeof(cap))==0);C(cap.minCycles==9546UL&&cap.maxWrites==16);C(owner==0&&sound==0);
 C(TCBGetCaps(2,&cap,sizeof(cap))==3);C(TCBGetCaps(TCB_VERSION,&cap,1)==3);
 C(TCBOpen()==0);C(TCOpen()==1);C(burstMode==1);C(burstRegs[7]==15);
 C(interval(1)==9546);active=1;C(nextInterval(0)==9546);active=0;
 b.at=0;b.count=4;b.data[0]=0xe7;b.data[1]=0xf4;b.data[2]=0xc0;b.data[3]=2;C(TCBPush(&b)==0);
 b.at=9545;b.count=1;b.data[0]=0xf8;copy=b;n=count;C(TCBPush(&b)==4);C(count==n&&!memcmp(&b,&copy,sizeof(b)));
 b.at=9546;C(TCBPush(&b)==0);b.at=19092;b.count=2;b.data[0]=b.data[1]=0xe7;C(TCBPush(&b)==0);
 C(TCEnd()==1);C(TCBEnd()==0);traceCount=0;C(TCStart()==0);tick(0);C(noiseWrites==1);C(burstRegs[4]==32);C(traceCount==4);
 tick(9546);C(noiseWrites==1);C(burstRegs[7]==8);C(traceBytes[4]==0xf8);tick(19092);C(noiseWrites==3);C(finished==1&&active==0);C(unsafe==0);C(TCClose()==0);C(sound==0&&owner==0);
 C(TCBOpen()==0);b.count=1;b.data[0]=0xf8;
 for(i=0;i<TCB_CAPACITY;i++){b.at=(DWORD)i*TCB_MIN_CYCLES;C(TCBPush(&b)==0);}
 b.at=(DWORD)TCB_CAPACITY*TCB_MIN_CYCLES;copy=b;n=tail;C(TCBPush(&b)==2);C(count==TCB_CAPACITY&&tail==n&&published==b.at-TCB_MIN_CYCLES);C(!memcmp(&copy,&b,sizeof(b)));
 task=2;C(TCBPush(&b)==1);C(TCClose()==1);task=1;C(TCClose()==0);
 C(TCBOpen()==0);b.at=0;b.count=1;b.data[0]=0xe7;C(TCBPush(&b)==0);b.at=100000;C(TCBPush(&b)==0);C(TCStart()==0);tick(0);C(noiseWrites==1);
 C(TCPause()==0);C(TCUnpause()==0);tick(1);C(noiseWrites==2);C(burstRegs[6]==7);C(TCClose()==0);
 C(TCBOpen()==0);b.count=1;b.data[0]=0xf8;
 for(i=0;i<TCB_CAPACITY;i++){b.at=(DWORD)i*TCB_MIN_CYCLES;C(TCBPush(&b)==0);}C(TCStart()==0);tick(0);
 for(i=TCB_CAPACITY;i<1600;i++){b.at=(DWORD)i*TCB_MIN_CYCLES;C(TCBPush(&b)==0);tick((DWORD)(i-TCB_CAPACITY+1)*TCB_MIN_CYCLES);C(error==0);}
 C(TCBEnd()==0);while(count)tick(now+TCB_MIN_CYCLES);C(burstWriteCount==1600);C(finished==1);C(TCClose()==0);
 C(TCBOpen()==0);b.at=0;C(TCBPush(&b)==0);b.at=TCB_MIN_CYCLES;C(TCBPush(&b)==0);C(TCStart()==0);tick(0);tick(TCB_MIN_CYCLES);tick(3UL*TCB_MIN_CYCLES);
 b.at=2UL*TCB_MIN_CYCLES;C(TCBPush(&b)==4);C(count==0&&error==0);b.at=4UL*TCB_MIN_CYCLES;C(TCBPush(&b)==0);tick(b.at);C(TCBEnd()==0);C(TCClose()==0);
 C(TCOpen()==0);memset(&f,0,sizeof(f));f.volume=65535;f.mask=15;C(TCPush(&f)==0);C(TCBPush(&b)==1);C(TCEnd()==0);C(TCStart()==0);tick(0);C(finished==1);C(TCClose()==0);
 printf("actual RAWCLK burst: %d checks, %d failures, unsafe IRQ calls=%d\n",checks,failures,unsafe);return failures!=0;
}
