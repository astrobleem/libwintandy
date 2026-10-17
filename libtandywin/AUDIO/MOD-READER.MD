# ProTracker MOD foreground reader

This directory contains a C89 parsing and row/tick foundation for four-channel
ProTracker files carrying the `M.K.` or `4CHN` signature. The parser API is used by the separate FMTCORE/FMTAPP MINIPLAY adapter; it does not call audio, DOS, Windows, or timer
services.

## Reading a file

Provide a `MOD_ReadAt` callback that reads an exact byte range from the selected
source, and call `mod_open`. The module stores the callback and its user value.
The reader loads the 1084-byte header only. Each call to `mod_player_tick`
fetches one 16-byte row through that callback, so the caller can adapt a file or
other seekable source without keeping pattern data in memory. Sample payloads
are length-checked but never read. No allocation is performed; the largest
temporary object is the 1084-byte header buffer in `mod_open`.

Initialize a `MOD_Player` with `mod_player_init`, then request one
`MOD_TickFrame` at a time. A return value of 1 is a tick, 0 is a stopped song
(or `F00`), and -1 reports a structural or unsupported-effect error. Error
records include order, row, channel, command, and parameter where applicable.
Call `mod_preflight` after `mod_open` and before acquiring audio ownership. It
scans all patterns referenced by the order table, rejects unsupported effects,
used nonzero finetunes, invalid instruments/break rows, more than three
simultaneously audible logical lanes, and periods outside the selected PSG
divisor range. It then simulates to `F00` or song-order end, with a hard maximum
of 65,535 ticks; internal order cycles that do not terminate within the chosen
bound fail explicitly. The caller owns file lifetime, playback stop policy,
PSG voice assignment, timer scheduling, and output.

## Preserved event model and patch mapping

Every snapshot retains all four logical source channels, their raw MOD period,
original 1-based instrument number, effect command, and parameter. The parser
does not assign source channel 4 to PSG noise or discard it; a later allocator
must decide how four MOD lanes fit three PSG tone voices and noise.

The explicit initial patch table is `instrument n -> patch ID n-1` for each of
the 31 header instruments. Every entry is tagged
`MOD_PATCH_SAMPLE_PLACEHOLDER`: the ID preserves which original instrument was
selected, while its placeholder patch means “generic square-tone substitute.”
This is a deterministic identity mapping, not a claim that a MOD sample has
been converted into a PSG instrument. The sample waveform, loop playback,
finetune correction, sample offset, retrigger, panning, and other sample-level
behavior are not reproduced. Header volume and loop metadata are retained for
adapters or later work.

The selected base finetune-zero pitch convention converts MOD period `p` to a
PSG tone divisor with
`round(p * 3,579,545 * 2 / 7,093,789)`, using the conventional 32-sample-cycle
PAL tuning basis. `mod_period_to_psg` implements this ratio without overflowing
32-bit intermediates and rejects divisors outside 1..1023. This maps note pitch
through the selected clocks; it does not emulate sample playback rate or
waveforms. Preflight rejects used instruments with nonzero finetune instead of
silently shifting their pitch.

Tick duration is returned as `17,897,725 / (6 * BPM)` PIT cycles, matching the
shared R4 clock of `3,579,545 / 3` Hz and the ProTracker `2.5 / BPM` second tick.
The denominator field contains `6 * BPM` (up to 1530). A producer must decide
how to carry rational remainder when tempo changes alter that denominator; this
parser reports the exact per-tick ratio but does not implement cross-tempo
remainder conversion. Speed controls ticks per row; BPM controls tick duration.
Default ProTracker values are speed 6 and BPM 125. Song order restarts at the
header restart order when that order lies within song length; otherwise
playback stops at the end.

## Current effects

Implemented effects are deliberately limited to semantics that can be emitted
as channel state without pretending to reproduce sample playback:

| Command | Current behavior |
| --- | --- |
| `0xy` | Arpeggio using integer semitone ratios; returns to the base period at the next row; finetune is not applied. |
| `1xx`, `2xx` | Period slides on ticks after tick 0, clamped to 1..4095. |
| `Axy` | Volume slide on ticks after tick 0; if both nibbles are set, the up nibble wins. |
| `Bxx` | Jump to order `xx`, row 0. |
| `Cxx` | Set volume, clamped to 64. |
| `Dxy` | Packed-BCD row break, accepted only for rows 00..63. |
| `E60`, `E6x` | Set pattern-loop start and repeat the row range `x` times. |
| `ECx` | Cut the keyed note at tick `x`; `EC0` cuts at row start. |
| `F01..F1F`, `F20..FFF` | Set speed or BPM. `F00` stops playback. |

Every other command, including tone portamento, vibrato, tremolo, sample offset,
retrigger, pattern delay, note delay, panning, finetune commands, and waveform
commands, returns `MOD_ERR_UNSUPPORTED_EFFECT` with its row location and raw
command. `E6x` pattern loops, `Bxx` jumps, and `Dxx` breaks are represented as
flow changes after the current row. When conflicting global-flow effects occur
on one row, the last channel in scan order wins within each flow category;
order jump takes precedence over row break, which takes precedence over a
pattern-loop jump. This ordering should be revisited against the selected
reference before claiming complete tracker compatibility.

## Validation boundary

`test_modplay.c` builds its own zero-sample synthetic modules in memory; it
contains no copyrighted music. It checks the public integer widths, supported
signatures and length validation, effect rejection, tick ratio, volume slide,
order jump/break, a finite `E60`/`E62` sequence that revisits the marker,
arpeggio reset, and slide persistence. Run `python run_host_tests.py` to compile
as C89 with warnings enabled and run the fixture executable. The runner uses a
process-local UCRT PATH, a 15-second timeout for each step, captures compiler
and test logs, and cleans its executable with a temporary directory. GCC C89
and these host fixtures do not establish Win16 compiler compatibility, real
module compatibility, sound quality, or integration with MINIPLAY or the R4
backend. No real MOD fixture or named `TNDYTracker IE.PAS` source was present in
the narrowly checked workspace trees at implementation time; the reference
path was left for the parent integration task to resolve.
