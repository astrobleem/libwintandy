# RAWCLK ordered-burst API v1

TCFRAME remains 16 bytes and ordinals 1–19 retain their declarations and frame
semantics. Clients must discover all four new exports and query capability
before acquiring sound. An older DLL is unavailable for this input path;
clients must not lower raw writes into old noise-retriggering TCFRAME masks.

| Ordinal | Entry | Contract |
|---|---|---|
|20|TCBGetCaps(version, FAR out, bytes)|No ownership change. Exact size/version required.|
|21|TCBOpen()|Acquire the existing exclusive SOUND/broker owner in raw mode.|
|22|TCBPush(FAR burst)|Validate and copy one complete group, then publish atomically.|
|23|TCBEnd()|Declare final publication; mute after the last queued deadline.|

All exports use FAR PASCAL; DLL implementations load their own DS. `TCBINFO`
is 24 bytes with Win16 two-byte alignment: six WORDs (size, version, features,
capacity, maxWrites, reserved), followed by three DWORDs (minCycles,
clockNumerator, clockDenominator). V1 returns version 0100h, feature bits 1|2,
capacity 512, maxWrites 16, reserved 0, minimum 9546 cycles, and clock ratio
3579545/3 Hz. The size argument must equal `sizeof(TCBINFO)`.

`TCBURST` is 22 bytes under the same alignment:

```c
DWORD at;       /* absolute PIT cycles from this playback epoch */
WORD count;     /* 0..16; zero is a terminal/time-only group */
BYTE data[16];  /* ordered raw bytes, unused suffix ignored */
```

The DLL never retains client pointers. It validates the whole request before
publishing. FULL leaves queue publication state and the caller's request
unchanged; retry that same pending request. Only one producer task may own
the stream. Timestamps must be ordered, at most 7FFFFFFFh, and separated by
the minimum even for time-only groups. Late active-stream admission is refused.

| Return | Meaning |
|---|---|
|0|Accepted/success|
|1|Owner or lifecycle mismatch|
|2|Ring FULL; no group accepted|
|3|Invalid capability argument or group|
|4|Unsupported ordering, spacing, horizon or already-late admission|

After priming, use existing TCStart. Common TCClock, TCDebug, TCPause,
TCUnpause, TCResume and TCClose apply to the same stream/owner. The format
adapter uses acknowledged Close followed by Open for restart or switching.
Frame-specific writer/LiveApply/End semantics are not part of this burst
contract. No new timer, broker protocol or borrowed-owner operation is added.

Raw debug selectors 22–28 report mode, intentional noise-control write count
(including explicit restoration), latched register, effective noise control,
noise attenuation, rolling source-byte hash and source-byte count. Restoration
bytes are excluded from the latter two counters. Hash update per source byte:
`h = rol16(h,5) XOR byte`. These are diagnostic WORD counters, not long-song
cryptographic integrity checks.

Preflight runs against a source kept open with write denial through playback.
All VGM command support, grouping, full MOD simulation, gzip integrity and
bounded timing checks precede TCBOpen. Runtime failures can still arise from
I/O faults, foreign hardware changes or foreground stalls. The player reports
them and safely closes; it does not silently retime or discard events.
