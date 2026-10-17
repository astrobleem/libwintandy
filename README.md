# libwintandy

Small shared components for 8086 Windows 3.0 applications. The historical
`libtandywin` directory and existing API names remain compatible.

* `LWTANDY.H` exposes the unchanged USER repaint helpers. TShell's isolated
  adoption changes its include/build boundary only; 24 original host checks
  pass and expanded source is identical.
* `LWAUDIO.H` forwards the existing app-local MCI/raw-player declarations.
  The isolated MINIPLAY (styled Tandy-Winamp) candidate uses this facade.
  Reused R4 provider source is in libtandywin/AUDIO, with its original event
  formats, timing and exclusive ownership. No replacement scheduler is added.

This is a source candidate. No native binaries are included. Fresh MSC6 player,
RAWQA and five raw-port rebuilds and native Windows3.0 checks remain required
before new binary publication. R4's header collision and historical evidence
limits are documented in libtandywin/AUDIO/FACADE/ERRATUM.JSON and STATUS.TXT.
Host checks cover source behavior and layouts; they do not certify IRQ timing,
Windows pixels, the Win16 ABI, or physical hardware.

UI code uses USER/GDI and has no Tandy display dependency. Current R4 audio
admission/backend is qualified for its exact Tandy HX fixture only; PCjr audio
is pending. The application API can remain the same when a PCjr backend is
qualified. No PCjr Setup/JRGRAB, Windows3.1 or protected-mode work is included.

See docs/BUILD.TXT and docs/ADOPT.TXT. No Windows files, SDK, fonts, tools,
ROMs, emulator, songs or captured media are bundled. Project source is MIT;
the RAWFILE format/table attribution retains LICENSE-TNDY.TXT as well.
