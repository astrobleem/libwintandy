# Current native MIDI engine

[MIDI8.JSN](MIDI8.JSN) is the exact current engine acceptance receipt.
Microsoft C6 compiled source `379752ed7b1a3b64702dc3fea5b40d35e98197ff`;
all 33 compiled inputs match that source. BUILD8, OBS10 and the complete
RUN10 supply all 239 successful assertions, with zero failures. Earlier
partial or failed runs contribute no acceptance assertions.

The engine targets 8086/8088 Windows 3.0 real mode. Recorded execution uses
DOSBox-X `8086_prefetch`, 640 KB and Windows 3.0 real mode; these are emulator
results, not physical hardware measurements. The canonical 9752-byte
SOUND.DRV remains unchanged.

## Memory and interfaces

The two mutually exclusive RAWCLK queue formats now share storage, retaining
1024 complete-state frames or 512 literal bursts. This saves 11264 bytes of
fixed data. RAWCLK's IRQ code, callback data and timer pin remain fixed.
MIDICLK renders in the foreground; its code can move and reload, while its
data remains fixed and shared. Task ownership, timer lifecycle, queue
capacities, the TCFRAME ABI and the additive paired-state API are unchanged.

Actual NE minimum allocations, in bytes:

| Runtime | CODE | DGROUP | CODE allocation |
|---|---:|---:|---|
| RAWCLK.DLL | 13800 | 27168 | Fixed |
| MIDICLK.DLL | 14352 | 12018 | Movable, discardable, preloaded |
| MCITNDY.DLL | 10948 | 3398 | Fixed |

The summed CODE/DGROUP minima are 81684 bytes, 12706 above the published
three-DLL baseline of 68978. Fixed demand is 67332 bytes, 1646 below that
baseline. These are segment minima; Windows loader and heap overhead is
separate. Each DLL specifies zero local heap and zero private stack.

| Runtime | File bytes | SHA256 prefix |
|---|---:|---|
| RAWCLK.DLL | 15464 | `a4d369e876be` |
| MIDICLK.DLL | 16576 | `b4eef7624120` |
| MCITNDY.DLL | 13036 | `133ac74490ee` |
| SOUND.DRV | 9752 | `ef501c64b805` |

Full binary hashes, source pins, segment flags and raw evidence identities
are in the receipt. `MDVersion` remains `0x00080100`, with default voice flags 1.

## Native results

RUN10 covers original low-key speaker/PSG fallback, busy handoff and recovery,
live EOF/reset/restart, sustain, volume/expression, pitch bend and controller
reset. Seven MCI fixtures include the 70065-byte LONGMETA file, with complete
validation, real EOF notification and cleanup.

The same run launches the actual DOS executable, observes its return marker
and verifies delivered playback after Windows resumes. Actual MINIPLAY
Play, three moving/repaint iterations, Stop and Close pass. Sound/PIT
ownership is released; Windows returns normally and bridge postflight passes.
The owned emulator process stopped normally and the shared lease was released.

This records engine and actual application acceptance. Exact current-package
Repair/GO/postflight is recorded separately by OEMSound. Public host CI is
separate from these author-run native results.
