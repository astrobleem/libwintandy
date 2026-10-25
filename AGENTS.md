# Contributor guidance

This is the Windows 3.0 real-mode integration library for 8086/8088.
The shared sound provider is OEMSound `src/win30/DRIVER/` (`SOUND.DRV`);
the player uses MCITNDY -> MIDICLK -> RAWCLK and its existing speaker
transport. Do not add a second sound driver or timer owner for instruments.

- Selectable PSG bank behavior belongs first in canonical libtandy
  `sound/INSTR.C` / `MIDISYN.C`. The player's vendor is
  `libtandywin/AUDIO/SOURCE/INSTR.*` / `MIDISYN.*`. This repository's
  separate `sound/` is producer compatibility code, not that synth copy.
- Preserve Default exactly; add one contrasting bank and developer host
  A/B audition with identical events. No accepted song edits, fake SoundFont
  claims or consumer Python/modern-PC preparation. Developer Python is fine.
- Current hooks: `MDAPI.H`, `MIDICLK.DEF`, `MIDICLK.C:MDVoiceMode` control
  envelope/vibrato flags only. `MSYN` has no bank selector. `MIDISKIP.H`
  skips unsupported file bank selects; `MIDIMSG.H` rejects nonzero CC0/32
  at the lower-level API. Do not describe these as selectable banks.
- Preserve the newer Windows paired-speaker fallback and streamed SMF
  behavior when porting canonical changes. OEMSound's matching mirror is
  `experimental/WINLIB30/SOURCE/`; its `tools/WINAMP/PACK.PY` and
  `VERIFY.PY` consume that mirror. Coordinate focused source updates.
- C89, fixed storage and integer arithmetic in production. Respect DS/SS,
  near/far pointers and existing export ordinals. Synth work is foreground;
  RAWCLK timer/IRQ ownership stays unchanged. Account for both `synth` and
  `folded` state in MIDICLK and measure native CODE/DGROUP growth against
  `libtandywin/AUDIO/TESTS/NATIVE/MIDI8.MD`.
- Read `docs/BUILD.TXT` before running checks. On Chad's Windows host use
  only `C:\Users\chad\Documents\Codex\toolchains\gcc-ucrt64.py`, never
  raw GCC/cc1. Use `-x c` for uppercase `.C`. Host checks use mocks and
  do not qualify native ABI, code size, timing or audibility.
- For the current documentation/bank handoff, do not launch emulators,
  change the active VGM player/format/decompression work, or publish
  without explicit approval. Use isolated checkouts of current main.

`docs/BUILD.TXT` is the build guide; no `BUILDING` file exists. The detailed
bank source map lives in canonical libtandy `docs/PSG-BANKS.md`.
