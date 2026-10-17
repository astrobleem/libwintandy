# Windows 3.0 native player preview

Download [WINLIB30.ZIP](WINLIB30.ZIP?raw=1) and read its INSTALL.MD before running.
The archive includes corresponding source, build recipes, the tested program
set, three original synthetic QA score assets, native logs and an actual window crop.
No Microsoft files, driver, tools, fonts, ROMs or third-party songs are included.

The native Tandy HX 8088 / 640 KB / stock-CGA Windows 3.0 real-mode GUI case
passed 58 checks with zero failures. Free memory was 292,992 bytes before and
after, and the bridge restored the original vectors and returned cleanly to
DOS. See [QUALIFY.JSON](QUALIFY.JSON). This is a bounded emulator result;
physical hardware, listening/fidelity and calibrated CPU timing are untested.

The exact external TSOUND prerequisite is the driver already published in
[PITWIN](https://github.com/astrobleem/oemsound-tandy/releases/tag/sound-exp-v0.1.0-pit.20261007),
SHA-256 7461d227846830f3ff604c40cf31ad0e3fb34122a13de6269e86de02cc4209e5.
Keep that dependency's published source/license. Installation is manual in a
matching disposable guest; the launcher checks compatibility and installs no
Windows driver. MOD/VGM/VGZ, automatic bass and PCjr audio remain unfinished.

Archive: 183,144 bytes, 111 entries, SHA-256
`d4d73d2d66439d8f0fdb3463027dbec1acbe37d3725aa65fb08894b9e69b4eeb`.
The original source-only snapshot remains unchanged at commit 80b1431.

This evidence correction preserves all nine runtime files byte-for-byte and
retains the existing native 58-check result; unchanged runtime was not rerun.
The archive now supplies the exact saved eleven-product paired build recipe,
commands, input hashes and logs. Its six-product standalone player recipe is
identified separately. Historical source-only records are clearly scoped.
TESTS/NATIVE/MAKEGUIQA.PY recreates the five additional first-party synthetic
GUI fixtures in an explicit empty directory. They are test assets, separate
from the three synthetic QA examples in RUNTIME. No third-party music or
sample payload is added.
