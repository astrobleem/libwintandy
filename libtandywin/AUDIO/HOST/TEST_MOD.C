#include "modplay.h"
#include <stdio.h>

static MOD_U8 bytes[4096];

static int memory_read(void *user, MOD_U32 offset, MOD_U8 *dst, MOD_U16 count)
{
    MOD_U16 i;
    (void)user;
    if (offset > (MOD_U32)sizeof(bytes) ||
        (MOD_U32)count > (MOD_U32)sizeof(bytes) - offset) return 0;
    for (i = 0; i < count; ++i) dst[i] = bytes[offset + i];
    return 1;
}

static void make_header(const char *signature)
{
    MOD_U16 i;
    for (i = 0; i < (MOD_U16)sizeof(bytes); ++i) bytes[i] = 0;
    bytes[45] = 32;
    bytes[950] = 1;
    bytes[951] = 0;
    bytes[952] = 0;
    bytes[1080] = (MOD_U8)signature[0];
    bytes[1081] = (MOD_U8)signature[1];
    bytes[1082] = (MOD_U8)signature[2];
    bytes[1083] = (MOD_U8)signature[3];
}

static void put_row(MOD_U8 row, MOD_U8 fx, MOD_U8 param)
{
    MOD_U16 off;
    off = (MOD_U16)(MOD_HEADER_BYTES + (MOD_U16)row * 16U);
    bytes[off + 2] = fx;
    bytes[off + 3] = param;
}

static void put_pattern_effect(MOD_U8 pattern, MOD_U8 row,
                               MOD_U8 fx, MOD_U8 param)
{
    MOD_U16 off;
    off = (MOD_U16)(MOD_HEADER_BYTES + (MOD_U16)pattern * 1024U +
                    (MOD_U16)row * 16U);
    bytes[off + 2] = fx;
    bytes[off + 3] = param;
}

static void put_channel_effect(MOD_U8 row, MOD_U8 channel,
                              MOD_U8 fx, MOD_U8 param)
{
    MOD_U16 off;
    off = (MOD_U16)(MOD_HEADER_BYTES + (MOD_U16)row * 16U +
                    (MOD_U16)channel * 4U);
    bytes[off + 2] = fx;
    bytes[off + 3] = param;
}

static int check(int condition, const char *label)
{
    if (!condition) {
        printf("FAIL: %s\n", label);
        return 0;
    }
    return 1;
}

int main(void)
{
    MOD_Module module;
    MOD_Player player;
    MOD_TickFrame frame;
    MOD_Error error;
    MOD_U16 period;
    int ok;
    int i;

    ok = 1;
    ok &= check(sizeof(MOD_U8) == 1 && sizeof(MOD_U16) == 2 &&
                sizeof(MOD_U32) == 4,
                "public integer widths are exactly 8/16/32 bits");
    make_header("M.K.");
    bytes[MOD_HEADER_BYTES] = 0x01;
    bytes[MOD_HEADER_BYTES + 1] = 0xAC;
    bytes[MOD_HEADER_BYTES + 2] = 0x1F;
    bytes[MOD_HEADER_BYTES + 3] = 3;
    put_row(1, 10, 2);
    put_row(2, 14, 0x60);
    put_row(3, 14, 0x62);
    ok &= check(mod_open(&module, memory_read, 0, MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "MK header opens");
    if (ok) {
        ok &= check(module.patch_id[0] == 0 && module.patch_kind[0] == MOD_PATCH_SAMPLE_PLACEHOLDER,
                    "instrument 1 explicitly maps to placeholder patch 0");
        mod_player_init(&player, &module);
        for (i = 0; i < 3; ++i) {
            ok &= check(mod_player_tick(&player, &frame, &error) == 1,
                        "speed effect produces tick");
            if (i == 0) {
                ok &= check(frame.period[0] == 428 && frame.instrument[0] == 1 &&
                            frame.patch_id[0] == 0 && frame.speed == 3 &&
                            frame.bpm == 125 && frame.pit_cycles_denominator == 750,
                            "tick preserves source note, instrument, speed and rational tempo");
            }
        }
        ok &= check(mod_player_tick(&player, &frame, &error) == 1 &&
                    frame.row == 1 && frame.tick == 0,
                    "row advance follows selected speed");
        ok &= check(mod_player_tick(&player, &frame, &error) == 1 &&
                    frame.row == 1 && frame.attenuation[0] == 8,
                    "volume slide updates attenuation on later tick");
        ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 1,
                    "second volume slide tick");
        ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 2,
                    "row transition after volume slide");
        for (i = 0; i < 2; ++i)
            ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 2,
                        "pattern loop marker row ticks");
        ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 3,
                    "pattern loop end row");
        for (i = 0; i < 2; ++i)
            ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 3,
                        "pattern loop end row ticks");
        ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 2,
                    "pattern loop jumps to saved row");
    }

    make_header("4CHN");
    ok &= check(mod_open(&module, memory_read, 0, MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "4CHN signature opens");
    make_header("8CHN");
    ok &= check(mod_open(&module, memory_read, 0, MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 0 &&
                error.code == MOD_ERR_SIGNATURE,
                "unsupported channel signature has explicit error");
    make_header("M.K.");
    bytes[42] = 0;
    bytes[43] = 1;
    ok &= check(mod_open(&module, memory_read, 0, MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 0 &&
                error.code == MOD_ERR_TRUNCATED,
                "declared sample payload is length checked");
    make_header("M.K.");
    bytes[MOD_HEADER_BYTES + 2] = 0x09;
    ok &= check(mod_open(&module, memory_read, 0, MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "unsupported effect fixture header opens");
    mod_player_init(&player, &module);
    ok &= check(mod_player_tick(&player, &frame, &error) == -1 &&
                error.code == MOD_ERR_UNSUPPORTED_EFFECT && error.effect == 9 &&
                error.order == 0 && error.row == 0 && error.channel == 0,
                "unsupported sample-offset effect reports source location and command");
    ok &= check(mod_preflight(&module, 65535UL, &error) == 0 &&
                error.code == MOD_ERR_UNSUPPORTED_EFFECT && error.effect == 9 &&
                error.order == 0 && error.row == 0 && error.channel == 0,
                "preflight rejects unsupported effect before playback ownership");

    make_header("M.K.");
    bytes[950] = 2;
    bytes[953] = 1;
    put_pattern_effect(0, 0, 11, 1);
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + 2UL * MOD_PATTERN_BYTES, &error) == 1,
                "two-order jump fixture opens");
    mod_player_init(&player, &module);
    for (i = 0; i < 6; ++i)
        ok &= check(mod_player_tick(&player, &frame, &error) == 1,
                    "Bxx row completes all six ticks");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 &&
                frame.order == 1 && frame.row == 0,
                "Bxx advances to selected order");

    make_header("M.K.");
    put_pattern_effect(0, 0, 13, 2);
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "pattern break fixture opens");
    mod_player_init(&player, &module);
    for (i = 0; i < 6; ++i)
        ok &= check(mod_player_tick(&player, &frame, &error) == 1,
                    "Dxx row completes all six ticks");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 &&
                frame.order == 0 && frame.row == 2,
                "Dxx breaks to requested row in the next order/restart");

    make_header("M.K.");
    put_channel_effect(0, 0, 14, 0x60);
    put_channel_effect(0, 1, 15, 1);
    put_channel_effect(1, 0, 14, 0x62);
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "finite explicit E60/E62 fixture opens");
    mod_player_init(&player, &module);
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 0,
                "pattern-loop fixture starts at E60 row");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 1,
                "pattern-loop fixture reaches E62 row");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 0,
                "first loop returns to E60 without resetting the count");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 1,
                "second E62 repeat");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 0,
                "second loop returns to E60");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 1,
                "completed loop leaves E62");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.row == 2,
                "E60/E62 loop terminates after the requested repeats");
    ok &= check(mod_preflight(&module, 1000UL, &error) == 1,
                "finite explicit pattern loop passes bounded preflight");

    make_header("M.K.");
    bytes[45] = 64;
    bytes[MOD_HEADER_BYTES] = 0x01;
    bytes[MOD_HEADER_BYTES + 1] = 0xAC;
    bytes[MOD_HEADER_BYTES + 2] = 0x10;
    bytes[MOD_HEADER_BYTES + 3] = 0x12;
    put_channel_effect(0, 1, 15, 3);
    put_channel_effect(1, 0, 1, 2);
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "arpeggio and slide fixture opens");
    mod_player_init(&player, &module);
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.period[0] == 428,
                "arpeggio begins on base note");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.period[0] == 404,
                "arpeggio applies first semitone");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.period[0] == 381,
                "arpeggio applies second semitone");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 &&
                frame.row == 1 && frame.period[0] == 428,
                "arpeggio returns to base at next row");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.period[0] == 426,
                "period slide is applied after row tick zero");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 && frame.period[0] == 424,
                "period slide continues on following tick");
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 &&
                frame.row == 2 && frame.period[0] == 424,
                "period slide result persists after effect row");

    ok &= check(mod_period_to_psg(428, &period) == 1 && period == 432,
                "selected PAL period conversion returns the expected tone divisor");
    ok &= check(mod_period_to_psg(4095, &period) == 0,
                "out-of-range MOD period is not silently clamped");

    make_header("M.K.");
    for (i = 0; i < 4; ++i) {
        MOD_U16 off;
        off = (MOD_U16)(MOD_HEADER_BYTES + (MOD_U16)i * 4U);
        bytes[off] = 0x01;
        bytes[off + 1] = 0xAC;
        bytes[off + 2] = 0x10;
    }
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "four-lane fixture opens");
    ok &= check(mod_preflight(&module, 1000UL, &error) == 0 &&
                error.code == MOD_ERR_VOICE_LIMIT && error.order == 0 &&
                error.row == 0 && error.tick == 0,
                "preflight rejects more than three simultaneously audible lanes");

    make_header("M.K.");
    for (i = 0; i < 4; ++i) {
        MOD_U16 off;
        off = (MOD_U16)(MOD_HEADER_BYTES + (MOD_U16)i * 4U);
        bytes[off] = 0x01;
        bytes[off + 1] = 0xAC;
        bytes[off + 2] = (MOD_U8)(i == 3 ? 0x1C : 0x10);
        bytes[off + 3] = (MOD_U8)(i == 3 ? 1 : 0);
    }
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "muted-lane fixture opens");
    mod_player_init(&player, &module);
    ok &= check(mod_player_tick(&player, &frame, &error) == 1 &&
                frame.attenuation[3] == 15 &&
                (frame.enabled_mask & 0x08U) == 0,
                "active lane quantized to PSG attenuation 15 is not audible");
    ok &= check(mod_preflight(&module, 1000UL, &error) == 1,
                "preflight counts audible lanes rather than active notes");

    make_header("M.K.");
    put_channel_effect(0, 0, 11, 0);
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "unbounded order-jump fixture opens");
    ok &= check(mod_preflight(&module, 20UL, &error) == 0 &&
                error.code == MOD_ERR_PREFLIGHT_LIMIT,
                "bounded preflight refuses an internal order-jump loop");

    make_header("M.K.");
    bytes[44] = 1;
    bytes[MOD_HEADER_BYTES] = 0x01;
    bytes[MOD_HEADER_BYTES + 1] = 0xAC;
    bytes[MOD_HEADER_BYTES + 2] = 0x10;
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "finetune fixture opens structurally");
    ok &= check(mod_preflight(&module, 1000UL, &error) == 0 &&
                error.code == MOD_ERR_FINETUNE,
                "preflight refuses used nonzero finetune");

    make_header("M.K.");
    put_channel_effect(0, 0, 13, 0x6A);
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "invalid break fixture opens structurally");
    ok &= check(mod_preflight(&module, 1000UL, &error) == 0 &&
                error.code == MOD_ERR_BAD_BREAK && error.parameter == 0x6A,
                "preflight rejects malformed packed-BCD break");

    make_header("M.K.");
    bytes[MOD_HEADER_BYTES] = 0x20;
    bytes[MOD_HEADER_BYTES + 1] = 0;
    bytes[MOD_HEADER_BYTES + 2] = 0;
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "invalid instrument fixture opens structurally");
    ok &= check(mod_preflight(&module, 1000UL, &error) == 0 &&
                error.code == MOD_ERR_INSTRUMENT,
                "preflight rejects instrument number above 31");

    make_header("M.K.");
    bytes[45] = 64;
    bytes[MOD_HEADER_BYTES] = 0x0F;
    bytes[MOD_HEADER_BYTES + 1] = 0xFF;
    bytes[MOD_HEADER_BYTES + 2] = 0x10;
    ok &= check(mod_open(&module, memory_read, 0,
                         MOD_HEADER_BYTES + MOD_PATTERN_BYTES, &error) == 1,
                "out-of-range note fixture opens structurally");
    ok &= check(mod_preflight(&module, 1000UL, &error) == 0 &&
                error.code == MOD_ERR_TONE_RANGE,
                "preflight rejects note outside PSG divisor range");
    if (ok) printf("MOD parser tests passed\n");
    return ok ? 0 : 1;
}
