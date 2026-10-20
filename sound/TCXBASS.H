#ifndef TCXBASS_H
#define TCXBASS_H
/* Additive local candidate; TCFRAME and TCBURST v1 are unchanged. */
#include "PITLINK.H"
#define TCX_VERSION 0x0100
#define TCX_AVAILABLE 0
#define TCX_DISABLED 1
#define TCX_UNAVAILABLE 2
#define TCX_BUSY 3
#define TCX_SUSPENDED 4
typedef struct {
    WORD size,version,status,minDivisor,maxDivisor,reserved;
} TCXINFO;
typedef struct {
    DWORD at;
    WORD count;
    BYTE data[TCB_WRITES];
    WORD pitDivisor;
} TCXBURST;
WORD TCAPI TCXGetCaps(WORD,TCXINFO FAR *,WORD);
WORD TCAPI TCXOpen(WORD);
WORD TCAPI TCXPush(TCXBURST FAR *);
WORD TCAPI TCXSetBass(WORD);
WORD TCAPI TCXStatus(void);
/* Additive complete-frame variant. Configure only before any frame queues. */
typedef struct {TCFRAME frame;WORD pitDivisor;} TCXFRAME;
WORD TCAPI TCXFrameMode(WORD);
WORD TCAPI TCXFramePush(TCXFRAME FAR *);
#endif
