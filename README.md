# libwintandy

Small shared components for 8086 Windows 3.0 applications. The historical
`libtandywin` directory and existing API names remain compatible.

* `LWTANDY.H` exposes the existing USER repaint helpers.
* `LWAUDIO.H` forwards the app-local MCI/raw-player interfaces.
* MINIPLAY's separate [format playback preview](examples/FORMATS30/README.md)
  adds foreground VGM/VGZ and bounded MOD PSG arrangements through a negotiated
  ordered-burst RAWCLK interface. The old TCFRAME ABI and timer owner remain.

The format preview passed 171 native checks in the recorded Tandy HX 8088,
640 KB, stock CGA, Windows 3.0 real-mode fixture. Source, original synthetic
fixtures, host tests, build evidence and a source-inclusive runtime archive
are available for review. [Qualification and measured costs](libtandywin/AUDIO/QUALIFY.MD)
state the precise limits. This is an opt-in draft; earlier facade/source-only
records remain labelled historical and the existing core preview is separate.

UI helpers use USER/GDI. The current audio fixture does not qualify PCjr,
Windows 3.1 or protected mode. MOD uses square-wave tone substitutes rather
than reproducing sampled timbres. Listening, physical audio and calibrated
CPU performance are outside the recorded result.

See [format architecture](libtandywin/AUDIO/FORMATS30.MD),
[RAWCLK burst API](libtandywin/AUDIO/BURST-API.MD), docs/BUILD.TXT and
docs/ADOPT.TXT. Project source is MIT; the RAWFILE format/table attribution
retains LICENSE-TNDY.TXT. No OS/SDK/toolchain/fonts/ROMs, external TSOUND driver,
third-party songs, sample media or emulator images are bundled.

Native validation is recorded author-run evidence, not public CI or an independent reviewer rerun.
