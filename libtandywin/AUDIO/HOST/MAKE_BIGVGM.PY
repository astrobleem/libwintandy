"""Developer tool: deterministic synthetic VGM/VGZ above 64 KiB for host tests.

Writes BIG.VGM (looped, loop start beyond offset 65535), BIG0.VGZ, BIG1.VGZ
and BIG9.VGZ (gzip levels 0/1/9 of the same bytes) and PLAIN.VGM (no loop).
Tandy SN76496 profile, version 1.50, only PSG writes and waits. No samples or
third-party music; the register values come from a fixed LCG.
Usage: python MAKE_BIGVGM.PY OUTDIR
"""
import gzip
import struct
import sys
from pathlib import Path

GROUPS = 21000
LOOP_AT_BYTE = 70000     # first group boundary at/after this data offset
WAITS = (441, 735, 882, 1323)   # samples; all >= 8 ms


def build(loop):
    seed = 12345
    data = bytearray()
    loop_off = None
    samples = 0
    loop_samples = 0
    for g in range(GROUPS):
        if loop and loop_off is None and len(data) >= LOOP_AT_BYTE:
            loop_off = len(data)
        seed = (seed * 1103515245 + 12345) & 0x7fffffff
        ch = seed % 3
        period = 40 + (seed >> 8) % 900
        att = (seed >> 4) % 15
        writes = [0x80 | (ch << 5) | (period & 15), period >> 4,
                  0x90 | (ch << 5) | att]
        if g % 97 == 0:
            writes.append(0xE0 | ((seed >> 12) & 7))
        for w in writes:
            data += bytes((0x50, w))
        wait = WAITS[(seed >> 16) & 3]
        data += b"\x61" + struct.pack("<H", wait)
        samples += wait
        if loop_off is not None:
            loop_samples += wait
    data += b"\x66"
    size = 0x40 + len(data)
    header = bytearray(0x40)
    header[0:4] = b"Vgm "
    struct.pack_into("<I", header, 0x04, size - 4)
    struct.pack_into("<I", header, 0x08, 0x150)
    struct.pack_into("<I", header, 0x0C, 3579545)
    struct.pack_into("<I", header, 0x18, samples)
    if loop_off is not None:
        struct.pack_into("<I", header, 0x1C, 0x40 + loop_off - 0x1C)
        struct.pack_into("<I", header, 0x20, loop_samples)
    struct.pack_into("<H", header, 0x28, 6)
    header[0x2A] = 15
    return bytes(header) + bytes(data), loop_off


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(".")
    out.mkdir(parents=True, exist_ok=True)
    big, loop_off = build(True)
    (out / "BIG.VGM").write_bytes(big)
    (out / "PLAIN.VGM").write_bytes(build(False)[0])
    for level in (0, 1, 9):
        (out / ("BIG%d.VGZ" % level)).write_bytes(
            gzip.compress(big, compresslevel=level, mtime=0))
    bad = out / "bad"
    bad.mkdir(exist_ok=True)
    good = gzip.compress(big, compresslevel=9, mtime=0)
    cases = {}
    cases["TRUNC.VGZ"] = good[:-1]
    cases["CUTMID.VGZ"] = good[:len(good) // 2]
    cases["CRC.VGZ"] = good[:-8] + bytes((good[-8] ^ 1,)) + good[-7:]
    cases["ISIZE.VGZ"] = good[:-4] + bytes((good[-4] ^ 1,)) + good[-3:]
    cases["TRAIL.VGZ"] = good + bytes(1)
    cases["FLIP.VGZ"] = good[:len(good) // 2] + bytes(
        (good[len(good) // 2] ^ 0x55,)) + good[len(good) // 2 + 1:]
    unsup = bytearray(big)
    struct.pack_into("<I", unsup, 0x2C, 7670454)   # YM2612 clock: other chip
    cases["CHIP.VGZ"] = gzip.compress(bytes(unsup), compresslevel=9, mtime=0)
    for name, data in cases.items():
        (bad / name).write_bytes(data)
    print("BIG.VGM %d bytes, loop data offset %d" % (len(big), 0x40 + loop_off))


if __name__ == "__main__":
    main()
