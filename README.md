# libwintandy

Small shared components for 8086 Windows 3.0 applications. The first component
is the existing USER/GDI repaint helper, now exposed by `LWTANDY.H`. Historical
`libtandywin` directory and `Ltw` API names remain compatible.

This initial source package contains the UI component only. Audio adoption is
proposed in docs/AUDIOAPI.TXT; no audio implementation or player binary is
included. DOS libtandy remains a separate project.

Include your own licensed Windows 3.0 WINDOWS.H, then LWTANDY.H. Compile
libtandywin/LTWUI.C into each caller. See libtandywin/README.TXT for contracts
and docs/BUILD.TXT for host and Win16 build recipes.

The 24 original host mock checks pass against the unchanged implementation.
TShell's old and new includes produced identical host preprocessor output.
These checks establish USER call behavior and an unchanged expansion, not a
fresh native Windows build, pixel equivalence, or physical hardware coverage.
Native checks are pending an emulator slot. See docs/ADOPT.TXT.

All files in this source package are first-party source, original mocks,
metadata, or documentation under LICENSE. No Windows headers, fonts, media,
compiler, emulator, third-party music, or native binaries are bundled.
