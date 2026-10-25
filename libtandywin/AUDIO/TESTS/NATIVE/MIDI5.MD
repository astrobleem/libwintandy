# Current native MIDI engine

[MIDI5.JSN](MIDI5.JSN) is the exact scoped engine acceptance receipt. Microsoft
C6 compiled source `f48e41e1d66fbcd99c6867e0abae24ae9e0c15af`; all 33 inputs
match reviewed source head `36e6be7dc5f5138c0a42d1d8995fa43fda782243`.
This publication adds evidence and documentation without changing those inputs.

The engine targets 8086/8088 Windows 3.0 real mode with the same canonical
SOUND.DRV used by desktop alerts and other sound clients. Recorded native
execution and all timings here use DOSBox-X `8086_prefetch`, 640 KB and
Windows 3.0 real mode; they are emulator results, not physical hardware results.
MIDI controllers
include sustain, volume/expression, pitch bend and CC120/121/123. SMF format
0/1 input is streamed; syntax and event admission are validated in one scan.
Original low-note keys remain intact. A prepared speaker voice and folded PSG
alternative share the existing clock and ownership; busy or unavailable PIT
output selects the prepared alternative. MIDI rendering and lease acquisition
stay in the foreground.

`MDVersion` remains `0x00080100`, with default voice flags 1. The TCFRAME ABI
is unchanged. Additive RAWCLK ordinals 34/35/36 negotiate and submit the
28-byte paired state, including atomic live replacement. Full binary hashes,
native NE sizes and source pins are in the receipt.

| Runtime | Bytes | SHA256 prefix | Relationship |
|---|---:|---|---|
| RAWCLK.DLL | 15464 | `c5ef93b36ad0` | Unchanged from the `5d9f76d` RAW/MID build |
| MIDICLK.DLL | 16576 | `a07e38e060c8` | Unchanged from the `5d9f76d` RAW/MID build |
| MCITNDY.DLL | 13036 | `133ac74490ee` | Current one-pass admission build |
| SOUND.DRV | 9752 | `ef501c64b805` | Existing canonical driver, unchanged |

## Native result partition

The receipt counts 161 successful scoped assertions. Earlier raw run failures
remain recorded; only the named results below are inherited for identical
RAWCLK/MIDICLK bytes. No earlier MCI fixture results are inherited.

| Evidence | Assertions | Scope |
|---|---:|---|
| RUN1, inherited | 23 | Original-key 0/13 fallback, key 14 PIT pitch, busy handoff, fresh live EOF and reset/restart |
| RUN2, inherited | 11 | Sustain, CC7, CC121 and bend state |
| RUN3 phase 6, inherited | 22 | Pause, actual DOS transition, delivered resume and cleanup |
| RUN6, current MCI | 80 | Six controller fixtures plus 70065-byte LONGMETA, real EOF notification and cleanup |
| RUN5, current MCI and actual player | 25 | MINIPLAY/STREAM Play, moving/repainting, Stop, observed Close and cleanup |

The seven current MCI fixtures include a track header at offset 70039 in
LONGMETA. The actual player run measured a warm GUI Play call of 714 ms,
followed by four repaint iterations over 4449 ms and successful Stop/Close,
sound/PIT release and normal Windows return.

## Supplemental song timings

These are measured command latencies in the recorded emulator fixture,
not full-song playback durations or calibrated hardware performance.

| Original input | Bytes | Events | Previous Open, ms | Current Open, ms | Current Play, ms |
|---|---:|---:|---:|---:|---:|
| STREAM.MID | 3100 | 1024 | 220 | 110 | 384 |
| BLONDIE.MID | 57716 | 17578 | 4339 | 2361 | 495 |

BLONDIE is represented by name and SHA256
`bd67338aff0948cedfbf3f37fe46e0449c4a224218202346395212746952a620`;
its song payload is not included. Both timing runs reported MCI error 0 and
delivered events during the four-second observation.

TIMING7 remains a raw FAIL with one assertion: BLONDIE's first MC_STOP returned
289 (busy), then MC_CLOSE returned 0 and released sound/PIT ownership. The
published player's Stop path uses that Stop/Close sequence and retains its
device on a failed Close. This supplemental measurement is excluded from the
161 assertions and is not relabelled as a full PASS.

This receipt qualifies the engine interfaces and the recorded actual app
path. Exact current-package repair, normal GO and desktop-exit acceptance are
recorded separately by OEMSound. It does not assert package READY or physical
hardware qualification. Public host CI is separate from these native runs.
