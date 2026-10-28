/* Actual RAWCLK, three virtual hours, physical DWORD wraps and no extra I/O. */
#define SPEAKER_CLOCK 1
#define main BurstBaselineMain
#include "TEST_BURST.C"
#undef main
int main(void)
{
    TCXBURST b;unsigned long i;DWORD period=286363200UL,physical,logical,oldBase;
    WORD n,rebases=0,oldTrace,oldNoise,oldPit,oldWrites;
    C(BurstBaselineMain()==0);C(TCBRebase(1)==1);
    C(TCXOpen(1)==0);now=0;memset(&b,0,sizeof(b));b.count=1;b.data[0]=0x90;b.pitDivisor=1000;
    C(TCXPush(&b)==0);b.at=119318UL;b.count=0;b.pitDivisor=1001;C(TCXPush(&b)==0);
    C(TCStart()==0);tick(0);C(TCBRebase(0)==3);
    for(i=1;i<=108000UL;++i){
        tick((DWORD)(i*119318UL));C(!error&&count==0);C(bassLast==1000+(i&1));
        b.at=TCClock()+119318UL;b.pitDivisor=(WORD)(1000+((i+1)&1));C(TCXPush(&b)==0);
        if(TCClock()>=period*2UL){
            physical=bursts[head].at;logical=queuedAt(head);oldBase=burstBase;
            oldTrace=traceCount;oldWrites=writes;oldNoise=noiseWrites;oldPit=pitDivisor;n=count;
            C(TCBRebase(clockCycles+1)==3);C(burstBase==oldBase);
            task=2;C(TCBRebase(period)==1);task=1;
            C(TCBRebase(period)==0);++rebases;
            C(burstBase==(DWORD)(oldBase+period)&&bursts[head].at==physical);
            C(queuedAt(head)==logical-period&&count==n&&traceCount==oldTrace&&writes==oldWrites);
            C(noiseWrites==oldNoise&&pitDivisor==oldPit&&hi&&owner);
        }
    }
    C(rebases>=40&&burstBase!=0);C(TCPause()==0);oldTrace=traceCount;oldWrites=writes;
    physical=bursts[head].at;logical=queuedAt(head);C(TCBRebase(1000)==0);
    C(!hi&&!pitDivisor&&traceCount==oldTrace&&writes==oldWrites&&bursts[head].at==physical&&queuedAt(head)==logical-1000);
    C(TCUnpause()==0);C(TCClose()==0);C(!owner&&!sound&&!pitToken&&!unsafe);
    C(TCBOpen()==0&&burstBase==0);C(TCClose()==0);
    /* A pending FULL retry and the published ring retain exact deadlines. */
    C(TCBOpen()==0);memset(&b,0,sizeof(b));b.count=1;b.data[0]=0xf8;
    for(i=0;i<TCB_CAPACITY;++i){b.at=(DWORD)i*TCB_MIN_CYCLES;C(TCBPush((TCBURST *)&b)==0);}
    now=0;C(TCStart()==0);tick(0);
    b.at=(DWORD)TCB_CAPACITY*TCB_MIN_CYCLES;C(TCBPush((TCBURST *)&b)==0);
    b.at+=TCB_MIN_CYCLES;C(TCBPush((TCBURST *)&b)==2);
    tick(TCB_MIN_CYCLES);C(count==TCB_CAPACITY-1);
    physical=bursts[head].at;logical=queuedAt(head);oldWrites=writes;
    C(TCBRebase(5000UL)==0);b.at-=5000UL;
    C(bursts[head].at==physical&&queuedAt(head)==logical-5000UL&&writes==oldWrites);
    C(TCBPush((TCBURST *)&b)==0&&count==TCB_CAPACITY);
    b.at+=TCB_MIN_CYCLES;C(TCBPush((TCBURST *)&b)==2);C(TCClose()==0);
    C(TCOpen()==0);oldBase=burstBase;C(TCBRebase(1)==1&&burstBase==oldBase);C(TCClose()==0);
    printf("PASS rolling RAWCLK three-hour/wrap/ownership checks=%d failures=%d rebases=%u\n",checks,failures,rebases);
    return failures!=0;
}
