# Windows MIDI producer compatibility module

This directory contains the Windows 3.0 foreground MIDI producer used by the
RAVE workflow. It emits timestamped PSG/PIT state through RAWCLK and retains
the accepted musical and output behavior. It includes no instrument bank and
calls neither Windows nor the synth from the raw timer interrupt.

The one sound provider is OEMSound's
[canonical driver source](https://github.com/astrobleem/oemsound-tandy/tree/main/src/win30/DRIVER).
Its `BUILD.PY` builds `SOUND.DRV` with the existing MSC6/MASM/LINK4 toolchain.
Install the complete current OEMSound package by running its native DOS
`SETUP.EXE`; Setup selects the Windows installation and installs the shared
driver and application files. No driver swap is required between applications.

`TIMECLK.C` acquires the installed SOUND module's raw PSG lease after
`OpenSound` and releases it after its callback detaches. Its optional speaker
path acquires the same driver's independent `TANDYPITLEASE`. `PITLINK.H` is
the consumer declaration for that API; the provider is implemented only in
OEMSound `src/win30/DRIVER`.

`SOURCE.json` records the current source hashes and retained source basis.
Build consumers also need the separately published `libtandy` sound headers;
they are not duplicated here. `CORE-MIT.TXT` applies to the first-party
producer core. No Windows SDK, compiler, ROM, private song, recording or
instrument bank is included.
