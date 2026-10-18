# libwintandy

**libwintandy** provides small shared components for 8086 Windows 3.0
applications. The DOS library is **libtandy** and remains a separate project.
The historical [`libtandywin/`](libtandywin/) source directory, header names
and API names remain compatible; see [naming and compatibility](docs/NAMING.TXT).

* `LWTANDY.H` exposes the existing USER repaint helpers.
* `LWAUDIO.H` forwards the app-local MCI/raw-player interfaces.
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

See [format architecture](libtandywin/AUDIO/FORMATS30.MD),
[RAWCLK burst API](libtandywin/AUDIO/BURST-API.MD), [build instructions](docs/BUILD.TXT)
and [adoption notes](docs/ADOPT.TXT). Project source is MIT; the RAWFILE
format/table attribution retains LICENSE-TNDY.TXT. No OS/SDK/toolchain/fonts/ROMs, external TSOUND driver,
third-party songs, sample media or emulator images are bundled.

Native validation is recorded author-run evidence, not public CI or an independent reviewer rerun.
