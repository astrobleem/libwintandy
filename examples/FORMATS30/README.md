# Current gzip compatibility preview

The current MINIPLAY accepts dynamic Huffman headers declaring 31 or 32
distance entries, while still rejecting reserved distance symbols 30/31 if
actually decoded. The buffers already cover the wider header; production
source changes one character. The fresh 47,997-byte MINIPLAY differs from the
baseline in that comparison byte and its NE checksum only, with no file/data/
stack growth and no timer/ownership change.

Download [LibWinTandy-Win30-Formats-Gzip-Fix-Preview-20261017.zip](LibWinTandy-Win30-Formats-Gzip-Fix-Preview-20261017.zip): 312963 bytes,227 files, SHA256
`44990d4546b3eaf5808e2b3f2746d438bbf9962d78cc36a8e276e0f7abfdedde`. The [manifest](MANIFEST.JSON) lists its contents and hashes.
It contains corresponding first-party source, tested binaries, original
synthetic fixtures/generators and saved build/test evidence. No Windows,
SDK/toolchain, driver, ROM/font, emulator image, third-party song or sample
payload is included. See [audit](../../libtandywin/AUDIO/AUDIT.JSON).

Recorded author-run validation: 160 format checks, 3271 backend checks and MOD
suite; fresh MSC6/G0 rebuild; 88 focused native decoder/MINIPLAY UI checks,
zero failures. All 31/32 header positives and actual reserved-symbol negatives
are covered, with exact playback hash/count and safe refusal. This is not
public CI or a reviewer rerun. See [current revision](../../libtandywin/AUDIO/GZIP-HDIST.MD)
and [qualification/costs](../../libtandywin/AUDIO/QUALIFY.MD).

The [original format preview](LibWinTandy-Win30-Formats-Native-Preview-20261017.zip)
and BASELINE-PACKAGE.JSON retain the earlier BUILD04 binary/171-check record.
Those 171 checks were not rerun in full on the header-only revision. MOD remains
a square-wave arrangement, with no sampled-timbre or listening-fidelity claim.
PCjr, Windows3.1 and calibrated CPU/physical-audio qualification remain separate.
