# Windows MIDI producer compatibility module

This directory preserves the frozen Windows 3.0 MIDI producer used by the RAVE full-song preparation workflow. It is a separate `sound/` module and does not replace or rename the historical `libtandywin/AUDIO` playback preview or the existing audio/driver implementation.

`MIDICLK.C` is a foreground scheduler around the shared MIDI synth and the existing RAWCLK output interfaces. It emits timestamped PSG/PIT state through the provider contract; it does not install a MIDI or DOS timer handler, call Windows or the synth from the raw timer interrupt, or include an instrument bank. The producer is intended for the RAVE preparer compatibility path, not as a general new Windows application API.

The module was copied byte-for-byte from the approved optimized-9 frozen source snapshot. `SOURCE.json` records every included file's SHA-256 and its origin. Build consumers also need the separately published `libtandy` sound headers listed as external dependencies in the RAVE pin; they are not duplicated here. `CORE-MIT.TXT` applies to the first-party producer core, and `DRIVER/LICENSE` retains the GPL license for the local derivative PIT lease header. Observe both notices when redistributing this source.

No Windows SDK, compiler, ROM, external TSOUND driver, private song, recording, or instrument bank is included. Historical audio examples under `libtandywin/AUDIO` keep their existing scope and qualification labels.
