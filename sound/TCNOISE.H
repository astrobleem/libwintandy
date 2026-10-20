#ifndef TCNOISE_H
#define TCNOISE_H
/* Additive RAWCLK v1. Existing frames, ordinals and literal bursts unchanged. */
#define TCN_VERSION 0x0100
#define TCN_EXPLICIT_NOISE 1
#define TCN_RETRIGGER 1
typedef struct {WORD size,version,features,frameBytes,eventMask,reserved;} TCNINFO;
/* TCXFRAME is also used without PIT: pitDivisor must then be zero. */
WORD TCAPI TCNGetCaps(WORD,TCNINFO FAR *,WORD);
WORD TCAPI TCNFramePush(TCXFRAME FAR *,WORD);
WORD TCAPI TCNLiveApply(TCFRAME FAR *,WORD);
#endif
