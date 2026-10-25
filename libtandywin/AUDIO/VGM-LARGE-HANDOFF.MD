# Large VGM/VGZ handoff (for the native rebuild)

This is the historical PR8 source handoff. The integrated player, foreground
progress/Cancel, automatic temporary-file handling and measured native build
are described in [FORMATS30.MD](FORMATS30.MD). Its packet TESTS.JSN records
the current qualification; the host-only results below retain their original
scope. The original TEMP-directory wording below does not impose a user
configuration step.

Separate from the MIDI work (PRs #6/#7, untouched). Branch
`vgm/large-files-20261025`, from `main` at `2b90b70`. Two separable commits:

| Commit | Change |
|--------|--------|
| `8d491a0` | Plain VGM above 60,000 bytes plays. Own group bound for VGM. |
| `2a55736` | VGZ of any size within the bound: inflate through a 32 KiB ring into a managed temp file. |

Apply `8d491a0` alone if the VGZ part is not wanted.

## What to rebuild

- **`MINIPLAY.EXE`** (links `FMTAPP.OBJ FMTCORE.OBJ` and, through them, the
  VGM/VGMGRP/GUNZIP objects). No DLL, `RAWCLK.DLL`, `SOUND.DRV` or installer
  change, and no RAWCLK interface change.
- `GZIPGUI.EXE` links `GUNZIP.OBJ`; the old `gz_decode` API and results are
  unchanged, so rebuilding it is optional.
- Changed build inputs: `FMTAPP.C`, `FMTCORE.C/.H`, `GUNZIP.C/.H`. No new
  files under `SOURCE`, so no dependency-list additions. `FORMATS30.MD` text
  updated.
- Overlap with the unified packet: only `MINIPLAY.EXE` itself. The MIDI DLLs
  from #6/#7 are separate.

## Behavior

- **Plain VGM:** the 60,000-byte refusal is gone. Offsets and lengths were
  already 32-bit in `VGM.C`/`VGMGRP.C`/`FMTCORE.C`; the reader keeps its
  512-byte cache. Nothing is loaded whole.
- **VGZ:** `FmtOpen` inflates the whole member before sound is acquired into
  a `GetTempFileName("VGZ")` file in the TEMP directory, verifying CRC32 and
  ISIZE, then plays that file like a plain VGM (cache reset on the swap).
  Preflight, the replay after it, and the loop jump therefore stay random
  access with no re-inflation inside `FmtPump`. `FmtClose` closes and deletes
  the temp file on Stop, EOF, next open, error and exit (`WM_CLOSE` and
  `WM_ENDSESSION` already call `FmtClose`).
- Decoder: same code, new `gz_decode_stream` (exact 32768-byte ring, 4 KiB
  sink calls). `gz_decode` is unchanged. A sink failure returns 5.
- Failures give a reason string and leave no handle, memory or temp file:
  no TEMP, cannot create, disk full, corrupt/truncated/CRC/ISIZE/trailing
  input, output beyond the bound.

## Remaining limits, and why

| Limit | Value | Reason |
|-------|-------|--------|
| Playback length | 2^31-1 PIT cycles, about 30 min, including the loop repeat | `TCPush` refuses timestamps above 2^31-1 and `due()` compares signed. A real hardware-path limit. |
| VGM groups | 224,970 | Derived: groups are at least 9,546 cycles apart inside that horizon (`FX_VGM_GROUPS`). Not a separate cap. MOD keeps 65,535. |
| Plain stream size | 16 MiB (`FMT_MAX_PLAIN`) | Policy only: bounds the preflight scan and the temp disk a VGZ can claim. Offsets are 32-bit and `_llseek` takes a signed LONG, so it can be raised. Not a format limit. |
| Free disk | inflated size | VGZ needs that much space in TEMP. |
| Diagnostics | `FmtDebug` 29/40 stay 16-bit; 42/43/44 return the high words | Debug only. |

## Memory

- Old VGZ: 60,000-byte FAR buffer held for the whole song, plus 512-byte cache.
- New VGZ: 32 KiB FAR ring only during open (freed before sound), plus the
  512-byte cache; the inflated song lives on disk. Estimated DGROUP change
  about +280 bytes (temp name and `OFSTRUCT`, less two pointers) and about
  +60 bytes of stack in `gz_decode`; the native map is the real number.
- Plain VGM: no change (512-byte cache).

## Tests (host only, not 8088 timing or audio)

`HOST/RUN_VGMBIG.PY` generates the inputs and runs everything. Results:

- Small supported files give identical write streams and timing before and
  after (golden dump of every `INPUTS` file plus `COPPER.VGM/.VGZ`).
- Synthetic 189,499-byte file, loop at offset 70,064 (past 65,535), writes
  across 512-byte and 64 KiB boundaries: parses, plays through the real
  `FMTAPP.C` open/pump/close, 21,001 groups plain or 34,241 with the loop
  repeat. The repeat replays the loop section with identical bytes and
  offsets, shifted by one loop length (within one PIT cycle).
- `BIG.VGM` equals `BIG0/1/9.VGZ` (gzip levels 0, 1, 9) burst for burst. The
  ring wraps about six times at level 9; a 3 MB stress file also matches.
- Corrupt input is refused before sound with zero handles, allocations or
  temp files left: truncation, CRC, ISIZE, trailing byte, flipped data, and a
  valid gzip of an unsupported-chip VGM. Also no TEMP, disk full, busy sound,
  missing RAWCLK, and Stop mid-song then reopen.
- The same host test against the old `FMTAPP.C` fails, so it detects the
  fix.
- Open Watcom 16-bit compile of the changed sources: clean except a Watcom
  FAR-pointer artifact on the `gz_decode_stream` call (same as the old
  `gz_decode` call; MSC builds it as `_far`).

## Local songs

No locally available Tandy-profile VGM exceeds 64 KiB, so the generated file
is the large-file proof. The local larger files are chip-profile refusals,
not size failures:

| File | Bytes | Result |
|------|------:|--------|
| castlev.vgm | 78,691 | other chip present |
| jvb.vgm | 110,855 | SN width 16 / flags 5 |
| skate.vgm | 132,088 | SN width 16 / flags 5 |
| zak.vgm | 157,214 | SN width 16 / flags 5 |
| ghostb.vgm | 36,620 | SN width 16 / flags 5 |
| maniac.vgm | 35,602 | SN width 16 / flags 5 |
| 1on1.vgm | 8,532 | SN width 16 / flags 5 |
| skatejam.vgm | 6,271 | SN clock/variant (clock 0) |
| jvb2.vgm | 9,295 | not feedback 6 / width 15 |
| Sega-Tune.vgm | n/a | not feedback 6 / width 15 |

These need an explicit SN-variant adapter, which is out of scope.
`COPPER.VGM/.VGZ` (4,066 bytes) play and match.

## Native acceptance (dot)

1. Rebuild `MINIPLAY.EXE`; check the map for DGROUP and stack.
2. Plain VGM above 64 KiB opens and plays to the end; Pause/Resume works.
3. A looped file above 64 KiB loops at the right point, without a gap or a
   "Playback buffer ran late".
4. The same song as VGZ: record the time from Play to first sound (inflate
   plus preflight; the UI is blocked), the TEMP location and free space.
5. After Stop, after the next song, at EOF and after closing the app, no
   `VGZ*.TMP` is left in TEMP.
6. A truncated or corrupt VGZ is refused with a message, no sound acquired.
7. Unplug TEMP / fill the disk (or point TEMP at a read-only location): clear
   message, no stuck file.
