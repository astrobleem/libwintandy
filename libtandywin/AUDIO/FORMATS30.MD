# MINIPLAY format integration candidate

This isolated branch adds a foreground VGM/VGZ and bounded MOD producer to
MINIPLAY. The accepted core preview is a separate artifact. Its recorded fixture qualification passes; review the supported profile and
costs before adopting this opt-in preview.

MINIPLAY dispatches `.VGM` and `.VGZ` to the same decoder, sniffing gzip by
content; `.MOD` selects the PSG arrangement. Play opens a write-denied file,
scans the entire supported stream, initializes replay, negotiates RAWCLK,
then acquires sound. Stop, switching inputs, restart, EOF and Close release
the same owner through acknowledged TCClose. Pause freezes the source clock
and mutes; Resume restores the effective registers and resumes that clock.
The Pause button applies to the new formats. Existing MIDI/TND/AGI paths stay
on their prior providers.

## Supported source profiles

VGM accepts the first-party parser's SN command/wait subset, version
1.00-1.71 syntax, with a strict target profile of 3,579,545 Hz, feedback 6,
15-bit SN76496 and zero flags. Unsupported chips, variants, stereo writes,
data blocks, PCM commands, malformed headers and unsupported commands are
refused before sound acquisition. A plain stream (VGM, or inflated VGZ) is
limited to 16 MiB (FMT_MAX_PLAIN, a policy bound on the preflight scan and temp
disk) and to 2^31-1 PIT cycles of playback, about 30 minutes including the loop
repeat, because RAWCLK timestamps are refused above that. The output is the Tandy chip's native register behavior, including
tone-2-coupled noise and its zero-period behavior; this is not chip emulation.

All writes at a source timestamp form one group, retaining byte order and
every explicit noise-control write. Maximum group size is 16 writes. Distinct
groups require at least 9,546 PIT cycles (the ceiling of 8 ms). Faster groups
are rejected with their source offset. No merging, pitch conversion or
automatic substitution into an older note/frame format occurs. Waits retain
the exact 3,579,545/132,300 PIT-cycle ratio per 44.1 kHz sample. A looped VGM
plays its initial traversal plus one bounded loop repeat, preserving fractional
time across that boundary. Zero-duration/invalid loops are refused.

VGZ is inflated in foreground, before sound is acquired, through a 32 KiB
history ring into an automatically named DOS temp file (GetTempFileName in
the TEMP directory), which Stop, EOF, the next open and exit delete. The
player then reads that file like a plain VGM, so the replay after preflight
and the loop jump need no re-inflation. Open takes the inflate time on an
8088 and the UI is blocked meanwhile. The first-party C decoder supports stored, fixed-Huffman and
dynamic-Huffman DEFLATE blocks, verifies CRC32 and ISIZE, rejects concatenated
members/trailing bytes, and caps optional name/comment lengths. Header CRC16
is outside this bounded profile and is refused. Python is only used to build
synthetic test inputs; it is not needed by the Windows player.

MOD accepts `M.K.` and `4CHN` sources, the parser's documented bounded effects,
finetune-zero used instruments, and a song ending within 65,535 tracker ticks.
Referenced patterns and the complete playback path are checked first.
Internal loops that exceed the bound, unsupported effects and more than three
simultaneously audible logical lanes are refused. Any of the four source
lanes may obtain a free tone; a lane retains that tone until it becomes
inaudible. There is no voice stealing or implicit fourth-lane noise mapping.

This is a square-wave PSG arrangement, not sampled MOD playback. Sample bytes
are skipped and original sample timbres are not reproduced. The explicit
finetune-zero PAL convention treats one waveform cycle as 32 sample clocks:
`PSG divisor = round(MOD period * 7159090 / 7093789)`. Out-of-range active
divisors are refused. Tracker volume maps to attenuation. Tick timing uses
17897725/(6*BPM) PIT cycles with Q16 fractional carry; cumulative truncation
is less than one PIT cycle over the 65,535-tick bound, including tempo changes.
Playback stops at F00 or order end; Play restarts from the beginning.

## Ownership, buffering and recovery

RAWCLK still owns the SOUND lease, broker attachment and timer. There is no
second timer owner, parser, file I/O, allocation or Windows API call in IRQ.
The IRQ consumes at most one prepared group and emits at most 16 source bytes.
The foreground producer primes and refills a 512-group ring, stopping on FULL.
That reserve is about 8.53 seconds at 60 Hz or 4.096 seconds at minimum spacing.
It cannot cover arbitrary foreground stalls. Late admission and multiple
overdue groups are explicit failures; timestamps are never shifted to recover.

The first native attempt exposed a short-gap adaptive scheduling fallback to
65,535 cycles. Burst mode now uses the qualified minimum for a short predicted
gap and for an empty/last-group lookahead. Legacy frame mode retains its old
behavior. A second attempt exposed insufficient foreground reserve; the
128-group prototype was replaced by the negotiated 512-group ring.

Effective raw registers and the latched register are cached. Normal tone or
attenuation writes never invent a noise-control write. Explicit repeated
noise writes are preserved. Pause/resume or DOS restoration rewrites complete
hardware state and intentionally restarts noise; LFSR phase continuity across
those boundaries is not promised. Stop/EOF mute by attenuation and release
ownership. The guarded TPLAY/TBRIDGE binaries and external TSOUND prerequisite
are unchanged.

## Evidence and cost

`TESTS/GZIP06/CURRENT-HOST/RESULT.JSON` records actual candidate C tests: 160 producer/decoder
checks and 3,271 RAWCLK checks, including ring wrap beyond 1,024 groups, FULL
retry, late refusal, terminal groups, noise bytes and legacy TCFRAME. The IRQ
test observed zero Windows API calls. Tests use original synthetic data.

Saved MSC6 `/G0 /Gw /Gs /AS /W3 /Od` compile/link results are in
`TESTS/BUILD/BUILD04`. RAWCLK is 10,814 bytes (+2,048); MINIPLAY is 47,997
bytes (+25,888). RAWCLK map code grows from 7,296 to 9,136 bytes; initialized
plus uninitialized DGROUP grows from 16,706 to 28,002 bytes (+11,296).
The app stack increases from 4,096 to 8,192 bytes. Only VGZ allocates
a 32 KiB inflate ring, and only until open finishes (this replaced a 60,000-byte
output buffer held for the whole song); plain VGM/MOD use a 512-byte file cache.
These are measured file/map costs, not calibrated CPU percentages.
The app's initialized/uninitialized data grows from 4,512 to 10,496 bytes
(+5,984); the stack is accounted separately. The interactive native case
returned to the same 261,344-byte free-space reading after playback. The
EOF/refusal case likewise returned to its 261,216-byte starting reading;
different harnesses are not compared as a total resident-memory benchmark.

The original BUILD04 runtime passed171 native checks. The current header-only
revision passes 88 focused native checks; see GZIP-HDIST.MD and QUALIFY.MD. Read
QUALIFY.MD and each TESTS/NATIVE RESULT.JSON for the exact scope and identities.
Failed attempts and watchdog-incomplete cases remain labelled in TESTS/HISTORY.
No physical-hardware or listening/fidelity claim follows from host tests.

## Specification and license boundaries

The original C integration and synthetic fixtures use this component's MIT
license. Existing notices remain applicable. No R-Type converter, commercial
music, sampled media, OS/compiler/fonts/ROMs or disk image is part of this work.
The external TSOUND component retains its separate published license.

Format references: [VGM 1.71 specification](https://github.com/vgmrips/vgmplay-legacy/blob/master/VGMPlay/vgmspec171.txt),
[DEFLATE RFC1951](https://www.rfc-editor.org/rfc/rfc1951),
[gzip RFC1952](https://www.rfc-editor.org/rfc/rfc1952).

Current gzip revision build/native details are in TESTS/GZIP06; BUILD04 maps below describe the unchanged baseline sizes. GZIP06 adds no production file/data/stack bytes.
