#!/usr/bin/env python3
"""Developer-only generator for deterministic MIDI player fixtures.

Writes small Standard MIDI Files: format 0, one track, 480 PPQN,
tempo 500000 us/qn (120 bpm, 1 beat = 500 ms).

Usage: python MAKE_MIDI_FIXTURES.PY [OUTDIR]
Default OUTDIR is the FIXTURES folder next to this script.
Stdlib only. No timestamps or random data, so output is repeatable.
"""
import os
import sys

PPQN = 480
TEMPO_US = 500000
TEMPO_META = b'\xff\x51\x03' + TEMPO_US.to_bytes(3, 'big')
END_OF_TRACK = b'\xff\x2f\x00'
PROGRAM_ORGAN = 16
MELODIC_LO = 45
MELODIC_HI = 96
DRUM_CH = 9
DRUM_LO = 35
DRUM_HI = 81
MAX_FILE_SIZE = 4096


def T(beats):
    """Convert a beat position to an absolute tick."""
    return int(round(beats * PPQN))


def vlq(value):
    """Encode a non-negative integer as a MIDI variable-length quantity."""
    assert value >= 0
    out = [value & 0x7F]
    value >>= 7
    while value:
        out.append(0x80 | (value & 0x7F))
        value >>= 7
    return bytes(reversed(out))


def check_msg(msg):
    """Validate one channel message against the fixture rules."""
    status = msg[0]
    kind = status & 0xF0
    ch = status & 0x0F
    data = msg[1:]
    expected = {0x80: 2, 0x90: 2, 0xA0: 2, 0xB0: 2,
                0xC0: 1, 0xD0: 1, 0xE0: 2}[kind]
    assert len(data) == expected, msg.hex()
    for byte in data:
        assert 0 <= byte <= 127, msg.hex()
    if kind in (0x80, 0x90, 0xA0):
        note = data[0]
        if ch == DRUM_CH:
            assert DRUM_LO <= note <= DRUM_HI, msg.hex()
        else:
            assert MELODIC_LO <= note <= MELODIC_HI, msg.hex()


def note_on(ch, note, vel=100):
    return bytes([0x90 | ch, note, vel])


def note_off(ch, note, vel=64):
    return bytes([0x80 | ch, note, vel])


def cc(ch, ctrl, value):
    return bytes([0xB0 | ch, ctrl, value])


def program(ch, prog):
    return bytes([0xC0 | ch, prog])


def chan_at(ch, value):
    return bytes([0xD0 | ch, value])


def poly_at(ch, note, value):
    return bytes([0xA0 | ch, note, value])


def bend(ch, value):
    """Pitch bend: 14-bit value, LSB data byte first, then MSB."""
    assert 0 <= value <= 16383
    return bytes([0xE0 | ch, value & 0x7F, (value >> 7) & 0x7F])


def build_track(events, end_tick):
    """Sort events stably by tick and encode them as MTrk data."""
    body = bytearray()
    prev = 0
    for tick, msg in sorted(events, key=lambda e: e[0]):
        assert tick >= prev
        if 0x80 <= msg[0] <= 0xEF:
            check_msg(msg)
        body += vlq(tick - prev) + msg
        prev = tick
    assert end_tick >= prev
    body += vlq(end_tick - prev) + END_OF_TRACK
    return bytes(body)


def write_smf(path, events, end_tick):
    """Write MThd plus one MTrk. Tempo meta is placed at tick 0."""
    all_events = [(0, TEMPO_META)] + list(events)
    track = build_track(all_events, end_tick)
    header = (b'MThd' + (6).to_bytes(4, 'big') +
              (0).to_bytes(2, 'big') + (1).to_bytes(2, 'big') +
              PPQN.to_bytes(2, 'big'))
    chunk = b'MTrk' + len(track).to_bytes(4, 'big') + track
    data = header + chunk
    assert len(data) < MAX_FILE_SIZE
    with open(path, 'wb') as f:
        f.write(data)


def build_sustain():
    ev = [
        (0, cc(0, 64, 127)),
        (0, program(0, PROGRAM_ORGAN)),
        (T(1), note_on(0, 60)),
        (T(1.5), note_on(0, 64)),
        (T(2), note_on(0, 67)),
        (T(2.5), note_off(0, 60)),
        (T(2.5), note_off(0, 64)),
        (T(2.5), note_off(0, 67)),
        (T(5), cc(0, 64, 0)),
        (T(6), note_on(0, 72)),
        (T(6.5), note_off(0, 72)),
    ]
    return ev, T(7)


def build_volexpr():
    ev = [
        (0, program(0, PROGRAM_ORGAN)),
        (0, program(1, PROGRAM_ORGAN)),
        (0, cc(0, 7, 127)),
        (0, cc(1, 7, 40)),
        (T(1), note_on(0, 60)),
        (T(1), note_on(1, 67)),
        (T(2), cc(1, 11, 127)),
        (T(3), cc(1, 11, 90)),
        (T(4), cc(1, 11, 60)),
        (T(4.5), cc(1, 11, 30)),
        (T(5), cc(1, 11, 127)),
        (T(5), note_off(0, 60)),
        (T(5), note_off(1, 67)),
    ]
    return ev, T(6)


def build_bend():
    ev = [
        (0, program(0, PROGRAM_ORGAN)),
        (0, bend(0, 8192)),
        (T(1), note_on(0, 64)),
    ]
    # Up: 16 steps, one every 30 ticks, from beat 2 to beat 3.
    for i in range(1, 17):
        value = 8192 + (8191 * i + 8) // 16
        ev.append((T(2) + 30 * i, bend(0, value)))
    # Down: 16 steps, one every 30 ticks, from beat 3 to beat 4.
    for i in range(1, 17):
        value = 16383 - (16383 * i + 8) // 16
        ev.append((T(3) + 30 * i, bend(0, value)))
    ev.append((T(5), bend(0, 8192)))
    ev.append((T(6), note_off(0, 64)))
    return ev, T(7)


def build_alloff():
    ev = [
        (0, cc(0, 64, 127)),
        (0, program(0, PROGRAM_ORGAN)),
        (T(1), note_on(0, 60)),
        (T(2), cc(0, 123, 0)),
        (T(3), cc(0, 64, 0)),
        (T(5), cc(0, 64, 127)),
        (T(6), note_on(0, 62)),
        (T(7), cc(0, 120, 0)),
        (T(8), cc(0, 64, 0)),
    ]
    return ev, T(10)


def build_steal():
    ev = [
        (0, cc(0, 64, 127)),
        (0, program(0, PROGRAM_ORGAN)),
        (0, program(1, PROGRAM_ORGAN)),
        (0, cc(1, 7, 50)),
        (T(1), note_on(0, 60)),
        (T(1.1), note_on(0, 62)),
        (T(1.2), note_on(0, 64)),
        (T(2), note_off(0, 60)),
        (T(2), note_off(0, 62)),
        (T(2), note_off(0, 64)),
        (T(3), note_on(1, 72)),
        (T(4), note_off(1, 72)),
        (T(5), cc(0, 64, 0)),
    ]
    for beat in (1, 2, 3, 4, 5):
        ev.append((T(beat), note_on(DRUM_CH, 38)))
        ev.append((T(beat) + 60, note_off(DRUM_CH, 38)))
    return ev, T(6)


def build_extras():
    ev = [
        (0, cc(0, 0, 0)),
        (0, cc(0, 32, 0)),
        (0, program(0, PROGRAM_ORGAN)),
        (0, cc(0, 101, 0)),
        (0, cc(0, 100, 0)),
        (0, cc(0, 6, 2)),
        (0, cc(0, 38, 0)),
        (0, cc(0, 71, 64)),
        (0, cc(0, 1, 30)),
        (0, cc(0, 7, 100)),
        (0, cc(0, 10, 64)),
        (0, cc(0, 91, 40)),
        (0, cc(0, 93, 20)),
        (T(1), note_on(0, 60)),
        (T(1.25), chan_at(0, 80)),
        (T(2), poly_at(0, 60, 90)),
        (T(2.5), chan_at(0, 30)),
        (T(2.75), poly_at(0, 60, 50)),
        (T(3), note_off(0, 60)),
        (T(3.5), cc(0, 64, 127)),
        (T(4), note_on(0, 64)),
        (T(4.2), note_off(0, 64)),
        (T(4.5), cc(0, 64, 0)),
    ]
    return ev, T(6)


FIXTURES = [
    ('SUSTAIN.MID', build_sustain),
    ('VOLEXPR.MID', build_volexpr),
    ('BEND.MID', build_bend),
    ('ALLOFF.MID', build_alloff),
    ('STEAL.MID', build_steal),
    ('EXTRAS.MID', build_extras),
]


def main(argv):
    here = os.path.dirname(os.path.abspath(__file__))
    default_dir = os.path.join(here, 'FIXTURES')
    outdir = argv[1] if len(argv) > 1 else default_dir
    os.makedirs(outdir, exist_ok=True)
    for name, builder in FIXTURES:
        events, end_tick = builder()
        path = os.path.join(outdir, name)
        write_smf(path, events, end_tick)
        print('%s %d bytes' % (path, os.path.getsize(path)))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
