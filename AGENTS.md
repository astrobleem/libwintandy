# Contributor guidance

This is the Windows 3.0 real-mode integration library for 8086/8088.
The shared sound provider is OEMSound `src/win30/DRIVER/` (`SOUND.DRV`);
the player uses MCITNDY -> MIDICLK -> RAWCLK and its existing speaker
transport. Do not add a second sound driver or timer owner for instruments.

- Selectable PSG bank behavior belongs first in canonical libtandy
  `sound/INSTR.C` / `MIDISYN.C`. The player's vendor is
  `libtandywin/AUDIO/SOURCE/INSTR.*` / `MIDISYN.*`. This repository's
  separate `sound/` is producer compatibility code, not that synth copy.
- Preserve Default and Chamber exactly. Developer host A/B auditions use
  identical events and gain; envelope/gain tuning needs user direction. No accepted song edits, fake SoundFont
  claims or consumer Python/modern-PC preparation. Developer Python is fine.
- `MIDICLK.C:MDVoiceMode` controls envelope/vibrato flags. `MSYN.bank` /
  `MSBank` select Default (0) or Chamber (1). Windows uses dedicated
  `MDInstrumentBank` (ordinal19), `MDSeedBank` (20) and `MciBank` (905).
  `MIDISKIP.H` skips unsupported file bank selects; `MIDIMSG.H` rejects
  nonzero CC0/32 at the lower API. Keep those standard MIDI messages
  separate from the explicit instrument choice.
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
- Read `docs/BANKS.TXT` for the qualified bank behavior and evidence scope.
  Preserve current VGM streaming/progress/temp cleanup. Native execution
  requires the shared lease, inventory, private inputs, bounded runs and
  owned cleanup. Use isolated current-main checkouts; publication requires
  explicit approval. Keep per-app Beats/Fiddle changes out of shared work.

`docs/BUILD.TXT` is the build guide; no `BUILDING` file exists. The detailed
bank source map lives in canonical libtandy `docs/PSG-BANKS.md`.
