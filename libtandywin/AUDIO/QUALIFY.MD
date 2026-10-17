# Current gzip compatibility revision

Current GZIP06 MINIPLAY passes 88 focused author-run native checks: all nine
decoder vectors, accepted 31/32-entry VGZ playback to exact EOF/hash/count,
standard VGZ regression and actual reserved-distance refusal before SOUND.
Both dynamic and fixed reserved 30/31 cases emit the leading literal then
return Huffman error 3, with the output canary intact. The current host suite
passes 160 format checks, 3271 backend checks and the MOD suite.

Native elapsed 124.437 seconds; outer shared gate released, owned process stopped,
source image unchanged, original vectors restored and clean Windows 0/DOS text
return. MEMORY free_before=257280 free_after=257280

MSC6/G0 build: 14.563 seconds. MINIPLAY remains 47,997 bytes, SHA256
`6e94d2b8f41cdd7c9dcb2b5f5849f2df1c0713dff1fa10128dbc27f7f19130ab`. Compared with BUILD04, one comparison immediate changes 30 to 32 and one
NE checksum byte changes. All other executable bytes are identical. File,
segment, data and stack sizes are unchanged. No IRQ/owner change, CPU-percent,
listening or physical-audio claim is made. Current records are TESTS/GZIP06;
all referenced current-run screenshots are included.

The original 171-check baseline below belongs to BUILD04's earlier binary
identity. It was not rerun in full on the header-only revision. Its source
GUNZIP and 115-check host test are retained under TESTS/BASELINE. The existing
RAWCLK/MIDI/MCI/launcher dependencies are unchanged. These are recorded
author-run results, not public CI or reviewer reruns.

# Original BUILD04 qualification (historical baseline)

The exact BUILD04 production runtime passed all four bounded native cases in
the Tandy HX 8088 4.772728 MHz, 640 KB, stock CGA, Windows 3.0 real-mode fixture.
Each case used a fresh disposable seed clone. No CF access was involved.

| Case | Checks / failures | Host elapsed seconds |
|---|---:|---:|
| Interactive controls | 30 / 0 | 262.188 |
| EOF and refusal | 43 / 0 | 118.203 |
| Long streams, loop, maximum burst | 40 / 0 | 222.531 |
| Legacy MIDI/TND/AGI regression | 58 / 0 | 175.984 |

Total: 171 checks, zero failures. Every case completed within its 280-second
watchdog, restored the original BIOS/INT2F vectors, returned Windows exit 0
and DOS text mode, stopped its owned emulator and released the shared lease.
The private source image SHA-256 remained
`b8a2693afdeca383a5d6a5d3834494284eaa46cab70c5269ee2cce61be3f187f`.
No OS image is included. TESTS/NATIVE contains extracted logs and receipts;
MINIPLAY.PNG is an actual unedited emulator frame, not a mockup.

Interactive checks cover source opening, exclusive ownership, pause freezing
both source byte count and playback clock, resume, switching, Stop, restart
and closing during playback. EOF/refusal checks cover VGM, VGZ and the MOD
arrangement, corrupt gzip, an over-fast stream, and four simultaneously
audible MOD lanes. Unsupported inputs are refused before sound ownership.

Long VGM and VGZ each emitted exactly 2,411 source bytes with rolling hash
9,118. The bounded repeated loop emitted 142 bytes, hash 62,286 and six
intentional noise-control writes. The maximum 16-byte burst emitted all 16
bytes with hash 3,585. Short VGM/VGZ each emitted 131 bytes with hash 54,622
and three explicit noise writes. These are diagnostic WORD hashes/counters,
not cryptographic audio hashes. The legacy 58-check harness was retained
unchanged from the pinned core fixture, not freshly rebuilt for this run.

Free-space readings before/after: interactive 261,344/261,344; EOF/refusal
261,216/261,216; full stream 261,344/261,344; legacy 261,152/261,152 bytes.
These are per-harness leak checks, not a cross-harness resident-memory test.
The minimum sampled foreground queue depth in the interactive case was 308
groups; sampling does not establish an IRQ-time minimum watermark.

The packaged actual production sources pass 115 producer/decoder checks,
3,271 RAWCLK checks, and the synthetic MOD parser suite with C89 UCRT GCC.
The IRQ fake observed zero Windows API calls. Host coverage includes 1,600
groups across ring wrap, FULL retry without partial publication, late refusal,
terminal/horizon limits, repeated noise bytes and the old TCFRAME layout/path.
HOST/RUNHOST.PY reproduces these checks with a new output directory. Current
portable readers assume a 32-bit unsigned long target, as in MSC6 and Windows
UCRT; LP64 hosts require an explicitly reviewed integer typedef adapter.

Actual compile/link commands, compiler logs, input hashes, maps and receipts
are in TESTS/BUILD/BUILD04 and HARNESS05. BUILD04 compiled production code;
HARNESS05 separately built the split harnesses. The public input manifests
retain hashes for the distributed first-party source whitelist and saved batch.
Unused historical private inventory rows were omitted; the complete preparation
manifests remain local. Host log/command paths were normalized to remove private
workspace names. Qualification values and tested source/binary bytes are unchanged.
See AUDIT.JSON for the publication audit. Win16 products were marked expected-version 0300h
in their NE headers after linking; no executable code bytes were patched.

BUILD/FORMAT30.PY is the consolidated reproduction recipe for the two changed
production targets and four QA harnesses. It has not itself been rerun under
DOSBox; the saved exact batches are the actual successful build evidence.
Unchanged MIDI/MCI/launcher dependencies retain their prior pinned native
identities. BUILD/DEPENDENCIES holds separate historical dependency recipes.

Measured file/map cost relative to the previous preview: RAWCLK file +2,048
bytes (10,814 total), code +1,840 (9,136), DGROUP +11,296 (28,002). MINIPLAY
file +25,888 (47,997), initialized/BSS data +5,984 (10,496), stack +4,096
(8,192). VGZ additionally allocates one locked FAR buffer of at most 60,000
bytes; VGM/MOD use the bounded file cache. There is no calibrated CPU-percent
measurement, listening test, physical audio capture, PCjr, protected-mode or
Windows 3.1 qualification. Parent physical CF/performance results are separate
and this sound preview makes no physical speedup claim.

FORMAT30 is an opt-in draft. Review the ABI/profile and costs, try original
supported fixtures in a disposable matching guest, then adopt the player
independently of the existing core preview PRs. Broader MOD/sample support,
automatic bass/PIT2, Calculator and video/input work remain separate stages.
No R-Type converter or its code was used; its access block was not bypassed.
