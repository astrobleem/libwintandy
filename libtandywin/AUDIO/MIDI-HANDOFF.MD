# MIDI controller and file-compat handoff (for the native rebuild)

This is the historical source handoff. See [current native MIDI results](TESTS/NATIVE/MIDI5.MD)
for the compiled engine, source pins and measured results.

Two stacked source changes. Neither ships binaries: dot owns the MSC6/8088
rebuild and the unified player packet.

| Part | Branch / PR | Commit |
|------|-------------|--------|
| Controllers: CC64, CC7/CC11, bend, CC120/121/123 | `midi/controllers-20261024`, PR #6 | `32a938a` (head) |
| File compat: streamed SMF, low-note fold, bank fallback | `midi/file-compat-20261025`, stacked on PR #6 | `c02941b` (head) |

## Rebuild

- Rebuild `MIDICLK.DLL` and `MCITNDY.DLL` from `SOURCE`. They are the only
  `.RSP` files that link the changed objects. Put both beside `MINIPLAY.EXE`
  and the other runtime DLLs in the existing packet. No second package.
- Add to the packet source dependencies: `LTPBASE.H LTMATH.H MIDIMSG.H
  PITNOTES.H MIDISKIP.H`. `SMF.H`, `SMF.C`, `MIDISYN.*`, `INSTR.*` and
  `PERIODS.H` also changed.
- Not touched: installer, `SOUND.DRV`, timer ownership, `RAWCLK.DLL`.
- Watch on 8088: (1) `MCITNDY` DGROUP now carries 16 x (64-byte window +
  state) of parser data; (2) `MCITNDY` keeps the song file handle open for
  the whole play; (3) the open-time validation pass walks every event, so
  time the open of the biggest song (`BLONDIE` is 17578 events); (4) the
  parser seeks and reads about once per 10 events while playing.

## Audible checklist (about 5 minutes)

Copy `TESTS\FIXTURES\*.MID` to the target. 120 bpm: one beat is 0.5 s.
Play each, then Stop, then play the next with no restart.

1. `SUSTAIN.MID`: three-note chord. Notes are released at beat 2.5 but keep
   sounding until the pedal lifts at beat 5, then silence. A single note at
   beat 6 sounds and ends normally.
2. `VOLEXPR.MID`: two held tones. The second (channel 1) is clearly quieter
   from the start. Its level steps down at beats 3, 4 and 4.5, and is
   back at the level of the first tone at beat 5.
3. `BEND.MID`: one held note. It glides up, then down, then returns to the
   original pitch at beat 5. The range is a whole tone (+/-2 semitones).
4. `ALLOFF.MID`: part A, a note still sounds after beat 2 (all-notes-off
   with pedal down) and stops at beat 3 when the pedal lifts. Part B, the
   note at beat 6 is cut at once at beat 7 (all-sound-off), no pedal tail.
5. `STEAL.MID`: pedal-held chord, then a higher note at beat 3 takes over the
   oldest pedal-held voice, quieter (channel 1 volume); the other two chord
   notes keep sounding.
   Drum hits on beats 1-5. Silence once the pedal lifts at beat 5.
6. `EXTRAS.MID`: opens and plays two notes. Aftertouch, RPN and the other
   extras are silently ignored. No error, no stuck note.
7. Stop mid-`SUSTAIN.MID` (pedal down). It goes silent. Play `BEND.MID`
   right after: it starts at the centre pitch, full level. Repeat opening
   `SUSTAIN.MID` twice. Any leftover pedal, bend or volume is a fail.
8. Let a short song run to the end with the pedal down (end of
   `SUSTAIN.MID` is fine). Silence at the end, no tone left on.
9. Open one real song from each size class (see table). Bass lines sound an
   octave or more up on a PSG tone. Nothing is rejected at open.

Fail = stuck tone after Stop/end/reopen, a refused open, a bend or volume
carrying into the next song, a pedal that never releases.

## Source behavior that the checklist assumes

- Per-channel state: CC7, CC11, bend, pedal, program. Cleared on every
  open/reset. Reset-all-controllers keeps CC7 and clears CC11, bend and pedal.
- End-of-file always seals a full mute, even with the pedal down.
- Fixed bend range +/-2 semitones. Aftertouch and RPN/NRPN are ignored.
- Bank select is ignored; the one supported bank stays active and program
  changes still apply.
- Notes below 45 keep their original key in MIDI state and sound
  octave-folded on a PSG tone. This is a fallback, not speaker output.
  Real speaker-bass routing needs the existing audio owner, not a new timer.
- Drum notes outside 35..81 are skipped (no PSG voice). Notes 97..127
  play.

## Local songs, host parse and render (not yet heard on hardware)

`TESTS\song_preflight.c`: opens each file through the streamed reader and
applies the admitted events to the renderer. Old = packet before PR #6.

| Song | Bytes | Old | Now | Channel events | Skipped | Folded low notes | Length |
|------|------:|-----|-----|---------------:|--------:|-----------------:|-------:|
| STREAM.MID | 3100 | opens | opens | 1024 | 0 | 0 | 10 s |
| Gremlins-Title.mid | 842 | refused (CC7/CC10) | opens | 203 | 0 | 0 | 7 s |
| b2f.mid | 2002 | refused (low notes) | opens | 472 | 0 | 27 | 34 s |
| uprising.midi.mid | 21211 | refused (low notes) | opens | 6535 | 0 | 639 | 397 s |
| CANYON.MID | 33883 | refused (size) | opens | 8874 | 9 | 749 | 121 s |
| Weezer_-_Buddy_Holly.mid | 36220 | refused (size) | opens | 10870 | 8 | 856 | 175 s |
| jump-van-halen.mid | 51761 | refused (size) | opens | 11974 | 40 | 772 | 236 s |
| BLONDIE.mid | 57716 | refused (size) | opens (4 padding bytes ignored) | 17578 | 0 | 711 | 200 s |

Streamed and in-memory reads give identical event sequences for the six
fixtures, CANYON, Weezer and BLONDIE (`TESTS\smf_stream_host.c`).

## Host tests (all pass)

`mdctl_host.c` 1733 checks (flat and player configuration, low-note fold,
admission), `mdhost\driver.c` 1493, `synth_host.c` 10029, `live_host.c` 18,
`smf_stream_host.c`. Open Watcom 16-bit compile of `SMF`, `MIDICLK`,
`MCIPSG`, `MCITNDY` is clean (syntax proxy only, not MSC6).
