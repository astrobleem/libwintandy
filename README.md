# libwintandy

**libwintandy** provides small shared components for 8086 Windows 3.0
applications. The DOS library is **libtandy** and remains a separate project.
The historical [`libtandywin/`](libtandywin/) source directory, header names
and API names remain compatible; see [naming and compatibility](docs/NAMING.TXT).
The canonical shared sound driver is built in
[OEMSound](https://github.com/astrobleem/oemsound-tandy/tree/main/src/win30/DRIVER).
This library contains its consumer interfaces.

* [Placement service](libtandywin/PLACE/README.TXT) supplies caller-owned normal
  window rectangles and recovery for the Windows XT window-moving desktop.
  It preserves minimized/maximized state without changing USER.EXE.
* [Bounded canvas/damage helper](libtandywin/CANVAS/README.TXT) is an opt-in
  caller-owned GDI module with optional monochrome surfaces;
  [CANVAS30](examples/CANVAS30/README.TXT) is a separate small consumer.
* `LWTANDY.H` exposes the existing USER repaint helpers.
* `LWAUDIO.H` forwards the app-local MCI/raw-player interfaces.
* [TPLAY](docs/TPLAYXT.TXT) supplies the guarded Windows XT HPXY/XRSYNC launcher
  used by [Tandy Winamp](https://github.com/astrobleem/oemsound-tandy).
* [Current MIDI services and native results](libtandywin/AUDIO/TESTS/NATIVE/MIDI8.MD)
  cover controllers, streamed SMF and paired speaker/PSG playback. Consumer
  packages use the stable [Windows XT downloads](https://github.com/astrobleem/oemdisplay-tandy/releases/tag/windows-xt).
* [Default/Chamber banks](docs/BANKS.TXT) provide explicit selection,
  persistent player choice and per-voice live reseeding through the existing
  SOUND/RAWCLK ownership path.
* MINIPLAY's separate [format playback preview](examples/FORMATS30/README.md)
  adds foreground VGM/VGZ and bounded MOD PSG arrangements through a negotiated
  ordered-burst RAWCLK interface. The old TCFRAME ABI and timer owner remain.

The current gzip-header revision passes 88 focused native checks in the Tandy
HX8088/640KB/stockCGA/Windows3.0 real-mode fixture, plus 160 format host checks,
3271 backend checks and the MOD suite. Its original BUILD04 baseline retains
the separate 171-check record; that full suite was not rerun on the header-only
revision. See [gzip compatibility](libtandywin/AUDIO/GZIP-HDIST.MD). Source, original synthetic
fixtures, host tests, build evidence and a source-inclusive runtime archive
are available for review. [Qualification and measured costs](libtandywin/AUDIO/QUALIFY.MD)
state the precise limits. This is an opt-in draft; earlier facade/source-only
records remain labelled historical and the existing core preview is separate.

UI helpers use USER/GDI. The current audio fixture does not qualify PCjr,
Windows 3.1 or protected mode. MOD uses square-wave tone substitutes rather
than reproducing sampled timbres. Listening, physical audio and calibrated
CPU performance are outside the recorded result.

The separate `sound/` directory contains a pinned Windows 3.0 MIDI producer
compatibility module used by the RAVE host preparation workflow. It is kept
separate from the current Windows player engine in
`libtandywin/AUDIO/SOURCE/`; it is not that player's vendored synth. See
`sound/README.md` and `sound/SOURCE.json`
for the producer scope, dependency boundary, file hashes and license notices.

See [format architecture](libtandywin/AUDIO/FORMATS30.MD),
[RAWCLK burst API](libtandywin/AUDIO/BURST-API.MD), [build instructions](docs/BUILD.TXT)
and [adoption notes](docs/ADOPT.TXT). Project source is MIT; the RAWFILE
format/table attribution retains LICENSE-TNDY.TXT. No OS/SDK/toolchain/fonts/ROMs, external sound driver,
third-party songs, sample media or emulator images are bundled.

Native validation is recorded author-run evidence, not public CI or an independent reviewer rerun.
