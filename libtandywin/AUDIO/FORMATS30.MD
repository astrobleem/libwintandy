# Tandy Winamp VGM, VGZ and MOD playback

MINIPLAY is the native Windows 3.0 real-mode Tandy Winamp player. The packaged
DOS Setup installs its runtime dependencies and the shared SOUND driver.
Start the installed player with `C:\WINAMP\GO`, then use Add or the filename
edit to select a file. The playlist holds 32 entries.

## Opening and playback

Plain VGM streams from disk through a 512-byte cache. VGZ expands through a
32 KiB FAR history ring into an automatically named temporary file, then
uses the same reader and playback path as plain VGM. The ring is freed
before sound is acquired. The user need not set TEMP or run a decompressor.

Opening reports expansion progress, then Checking file while validating the
stream. Both phases process foreground messages. Cancel stops an open, and
Close cancels and waits for its decoder to unwind before releasing memory
and closing the window. A failed temporary-file deletion retains its name
for cleanup retry. Stop, EOF, another open and normal exit remove the file.
No decoding, file I/O, allocation or Windows API call runs inside the IRQ.

Play validates the complete supported stream before acquiring sound.
Pause freezes the source clock and mutes; Resume restores the effective PSG
registers and resumes that clock. Stop, EOF and Close release the existing
RAWCLK owner. MIDI, TND and SND retain their current providers; this change
adds no timer owner or sound driver.

## VGM profile

The supported SN stream uses the Tandy chip's register behavior: 3,579,545 Hz,
feedback 6, 15-bit SN76496 and zero flags, within VGM 1.00â€“1.71 syntax.
The reader checks headers, source lengths and each command before playback.
Unsupported chips, stereo writes, data blocks, PCM and unsupported commands
are refused before sound acquisition. There is no SN variant adapter.

Plain or expanded VGM may occupy up to 16 MiB. The bound limits preflight
work and temporary disk space; file offsets remain 32-bit. Total playback
must fit the existing signed RAWCLK timestamp horizon of 2^31âˆ’1 PIT cycles,
about 30 minutes including the loop repeat. A looped song plays its initial
traversal and one repeat, preserving fractional timing at the boundary.

Writes at one timestamp stay in source order, including repeated noise
control writes, with at most 16 writes per group. Distinct groups require
at least 9,546 PIT cycles, the ceiling of 8 ms. Wait conversion retains the
exact 3,579,545/132,300 PIT-cycle ratio per 44.1 kHz sample. No timestamp,
pitch or register-stream substitution is performed.

VGZ supports stored, fixed-Huffman and dynamic-Huffman DEFLATE blocks.
CRC32 and ISIZE are checked. Concatenated members, trailing bytes, gzip
header CRC16 and overlong optional headers are outside the supported
profile. Corrupt data, unavailable memory and failed writes produce a
specific refusal and unwind the open.

## MOD profile

MOD supports `M.K.` and `4CHN`, finetune-zero instruments and the parser's
bounded effects, ending within 65,535 tracker ticks. Any source lane may
obtain a free tone; it keeps that tone until inaudible. More than three
simultaneously audible lanes are refused. This is a PSG square-wave
arrangement, with no sample-timbre reproduction or voice stealing.

The PAL convention uses one waveform cycle per 32 sample clocks:
`PSG divisor = round(MOD period * 7159090 / 7093789)`. Volume maps to
attenuation. Tick timing uses `17897725/(6*BPM)` PIT cycles with Q16 carry.
Playback ends at F00 or order end; Play starts again from the beginning.

## Buffering and native build

The foreground producer fills the existing 512-group RAWCLK ring. Its
reserve is about 8.53 seconds at 60 Hz or 4.096 seconds at minimum spacing.
Late admission and overdue groups are explicit playback failures; timing
is not shifted to conceal them. Pause/resume restores effective registers
and deliberately restarts noise; uninterrupted explicit source noise writes
remain intact.

The MSC6 build uses `/G0 /Gw /Gs /AS /W3 /Od`. The integrated MINIPLAY is
54,877 bytes, with 47,514 bytes of code and 15,600 bytes of initialized plus
uninitialized DGROUP. Heap and stack remain 2,048 and 8,192 bytes. Compared
with the published MIDI player this adds 2,848 file bytes, 2,496 code bytes
and 592 DGROUP bytes. Only MINIPLAY changes among executable runtime files;
SOUND, RAWCLK, the MIDI DLLs, launcher, installer and current clients retain
their published bytes.

Current native and host results belong in the packet's `TESTS.JSN`. The
older BUILD04/GZIP06 maps and fixture reports remain historical records of
their own binaries and do not describe the current player. The source
handoff in VGM-LARGE-HANDOFF.MD records the original PR8 implementation.

Existing component notices remain applicable. Synthetic host fixtures need
Python only during developer testing; the DOS/Windows consumer path does
not use it. No physical-hardware timing or listening result follows from
host or emulator assertions.
