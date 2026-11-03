/* Actual-source host contract tests. GDI is mocked, never native evidence. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "windows.h"
#include "WTCANVAS.H"

static unsigned long checks;
static const char *section;
#define CHECK(x) do { checks++; if (!(x)) { \
    fprintf(stderr, "%s:%d [%s] %s\n", __FILE__, __LINE__, section, #x); \
    exit(1); } } while (0)
#define MAX_HANDLES 64
#define REF_DC 1
#define DEST_DC 2
#define STOCK_BITMAP 1
#define ERROR_COLOR ((DWORD)0xffffffffUL)
enum { NEW_DC, NEW_MONO, NEW_COLOR, SELECT, DELETE_DC, DELETE_BITMAP,
       TEXT_COLOR, BK_COLOR, PATBLT, BITBLT, OP_COUNT };
typedef struct {
    int alive;
    HBITMAP selected;
    DWORD text, background;
} MOCKDC;
typedef struct { int alive, width, height, mono; } MOCKBITMAP;
static MOCKDC dcs[MAX_HANDLES];
static MOCKBITMAP bitmaps[MAX_HANDLES];
static int calls[OP_COUNT], fail_at[OP_COUNT], sequence;
static int last_select_sequence, last_delete_dc_sequence;
static int last_delete_bitmap_sequence, memory_dcs, owned_bitmaps;
static HDC compatible_dc_reference, compatible_bitmap_reference;
static int blt_x, blt_y, blt_w, blt_h, blt_sx, blt_sy;
static HDC blt_source, blt_destination;
static DWORD blt_text, blt_background;

static int fail(int operation)
{
    calls[operation]++;
    sequence++;
    return calls[operation] == fail_at[operation];
}
static void reset_mock(void)
{
    CHECK(memory_dcs == 0 && owned_bitmaps == 0);
    memset(dcs, 0, sizeof(dcs));
    memset(bitmaps, 0, sizeof(bitmaps));
    memset(calls, 0, sizeof(calls));
    memset(fail_at, 0, sizeof(fail_at));
    sequence = last_select_sequence = last_delete_dc_sequence = 0;
    last_delete_bitmap_sequence = 0;
    compatible_dc_reference = compatible_bitmap_reference = 0;
    dcs[REF_DC].alive = dcs[DEST_DC].alive = 1;
    dcs[REF_DC].text = RGB(1, 2, 3);
    dcs[REF_DC].background = RGB(4, 5, 6);
    dcs[DEST_DC].text = RGB(13, 14, 15);
    dcs[DEST_DC].background = RGB(16, 17, 18);
    bitmaps[STOCK_BITMAP].alive = 1;
}
static void dc_valid(HDC dc)
{
    CHECK(dc < MAX_HANDLES && dcs[dc].alive);
}
HDC CreateCompatibleDC(HDC reference)
{
    unsigned n;
    dc_valid(reference);
    compatible_dc_reference = reference;
    if (fail(NEW_DC)) return 0;
    for (n = 3; n < MAX_HANDLES; n++) if (!dcs[n].alive) {
        dcs[n].alive = 1;
        dcs[n].selected = STOCK_BITMAP;
        dcs[n].text = RGB(0, 0, 0);
        dcs[n].background = RGB(255, 255, 255);
        memory_dcs++;
        return (HDC)n;
    }
    CHECK(0);
    return 0;
}
static HBITMAP new_bitmap(int width, int height, int mono)
{
    unsigned n;
    for (n = 2; n < MAX_HANDLES; n++) if (!bitmaps[n].alive) {
        bitmaps[n].alive = 1;
        bitmaps[n].width = width;
        bitmaps[n].height = height;
        bitmaps[n].mono = mono;
        owned_bitmaps++;
        return (HBITMAP)n;
    }
    CHECK(0);
    return 0;
}
HBITMAP CreateCompatibleBitmap(HDC reference, int width, int height)
{
    dc_valid(reference);
    compatible_bitmap_reference = reference;
    if (fail(NEW_COLOR)) return 0;
    return new_bitmap(width, height, 0);
}
HBITMAP CreateBitmap(int width, int height, unsigned planes,
                     unsigned bits, const void *data)
{
    CHECK(planes == 1 && bits == 1 && data == NULL);
    if (fail(NEW_MONO)) return 0;
    return new_bitmap(width, height, 1);
}
HGDIOBJ SelectObject(HDC dc, HGDIOBJ object)
{
    HBITMAP old;
    unsigned n;
    dc_valid(dc);
    CHECK(object < MAX_HANDLES && bitmaps[object].alive);
    if (fail(SELECT)) return 0;
    if (object != STOCK_BITMAP)
        for (n = 3; n < MAX_HANDLES; n++)
            CHECK(n == dc || !dcs[n].alive || dcs[n].selected != object);
    old = dcs[dc].selected;
    dcs[dc].selected = object;
    last_select_sequence = sequence;
    return old;
}
BOOL DeleteDC(HDC dc)
{
    dc_valid(dc);
    CHECK(dc >= 3);
    if (fail(DELETE_DC)) return FALSE;
    last_delete_dc_sequence = sequence;
    dcs[dc].alive = 0;
    dcs[dc].selected = 0;
    memory_dcs--;
    return TRUE;
}
BOOL DeleteObject(HGDIOBJ object)
{
    unsigned n;
    CHECK(object > STOCK_BITMAP && object < MAX_HANDLES);
    CHECK(bitmaps[object].alive);
    for (n = 3; n < MAX_HANDLES; n++)
        CHECK(!dcs[n].alive || dcs[n].selected != object);
    if (fail(DELETE_BITMAP)) return FALSE;
    last_delete_bitmap_sequence = sequence;
    bitmaps[object].alive = 0;
    owned_bitmaps--;
    return TRUE;
}
DWORD SetTextColor(HDC dc, DWORD color)
{
    DWORD old;
    dc_valid(dc);
    if (fail(TEXT_COLOR)) return ERROR_COLOR;
    old = dcs[dc].text;
    dcs[dc].text = color;
    return old;
}
DWORD SetBkColor(HDC dc, DWORD color)
{
    DWORD old;
    dc_valid(dc);
    if (fail(BK_COLOR)) return ERROR_COLOR;
    old = dcs[dc].background;
    dcs[dc].background = color;
    return old;
}
BOOL PatBlt(HDC dc, int x, int y, int width, int height, DWORD rop)
{
    dc_valid(dc);
    CHECK(rop == BLACKNESS && x == 0 && y == 0);
    CHECK(dcs[dc].selected > STOCK_BITMAP);
    CHECK(width == bitmaps[dcs[dc].selected].width);
    CHECK(height == bitmaps[dcs[dc].selected].height);
    return !fail(PATBLT);
}
BOOL BitBlt(HDC dest, int x, int y, int width, int height,
            HDC source, int sx, int sy, DWORD rop)
{
    dc_valid(dest);
    dc_valid(source);
    CHECK(rop == SRCCOPY && width > 0 && height > 0);
    CHECK(sx >= 0 && sy >= 0 && dcs[source].selected > STOCK_BITMAP);
    CHECK(sx + width <= bitmaps[dcs[source].selected].width);
    CHECK(sy + height <= bitmaps[dcs[source].selected].height);
    CHECK(x >= -32768 && y >= -32768);
    CHECK((long)x + width <= 32767L && (long)y + height <= 32767L);
    blt_x = x; blt_y = y; blt_w = width; blt_h = height;
    blt_sx = sx; blt_sy = sy; blt_source = source; blt_destination = dest;
    blt_text = dcs[dest].text; blt_background = dcs[dest].background;
    return !fail(BITBLT);
}

static RECT rect(int left, int top, int right, int bottom)
{
    RECT r;
    r.left = (short)left; r.top = (short)top;
    r.right = (short)right; r.bottom = (short)bottom;
    return r;
}
static void same_rect(const RECT *r, int left, int top, int right, int bottom)
{
    CHECK(r->left == left && r->top == top);
    CHECK(r->right == right && r->bottom == bottom);
}
static void damage_cases(void)
{
    LTWDAMAGE s, other;
    RECT a, b, d;
    section = "damage edge cases";
    CHECK(!LtwDamageInit(NULL, 20, 10));
    LtwDamageReset(NULL); LtwDamageBegin(NULL); LtwDamageCommit(NULL);
    CHECK(!LtwDamageAdd(NULL, NULL));
    CHECK(!LtwDamageGet(NULL, NULL));
    CHECK(!LtwDamageInit(&s, 0, 10));
    CHECK(!LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 0, 0);
    CHECK(!LtwDamageInit(&s, 10, -1));
    CHECK(!LtwDamageInit(&s, 32768, 10));
    CHECK(!LtwDamageInit(&s, 10, 32768));
    CHECK(LtwDamageInit(&s, 20, 10));
    CHECK(LtwDamageInit(&other, 7, 9));
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 20, 10);
    CHECK(!LtwDamageGet(&s, NULL));
    LtwDamageBegin(&s);
    a = rect(1, 2, 5, 6); b = rect(15, 0, 25, 4);
    CHECK(LtwDamageAdd(&s, &a));
    CHECK(LtwDamageAdd(&s, &b));
    CHECK(!LtwDamageAdd(&s, NULL));
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 20, 10);
    LtwDamageCommit(&s);
    LtwDamageBegin(&s);
    a = rect(-10, 8, 2, 20);
    CHECK(LtwDamageAdd(&s, &a));
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 20, 10);
    /* No commit: a failed transfer cannot forget the previously shown frame. */
    LtwDamageBegin(&s);
    a = rect(8, 6, 12, 9); CHECK(LtwDamageAdd(&s, &a));
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 1, 0, 20, 9);
    LtwDamageCommit(&s);
    LtwDamageBegin(&s); /* Object disappears: erase last bounds once. */
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 8, 6, 12, 9);
    LtwDamageCommit(&s);
    CHECK(!LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 0, 0);
    a = rect(20, 0, 30, 4); CHECK(!LtwDamageAdd(&s, &a));
    a = rect(-5, 0, 0, 4); CHECK(!LtwDamageAdd(&s, &a));
    a = rect(0, 10, 4, 20); CHECK(!LtwDamageAdd(&s, &a));
    a = rect(0, -5, 4, 0); CHECK(!LtwDamageAdd(&s, &a));
    a = rect(6, 2, 4, 8); CHECK(!LtwDamageAdd(&s, &a));
    a = rect(2, 8, 4, 6); CHECK(!LtwDamageAdd(&s, &a));
    a = rect(3, 2, 3, 8); CHECK(!LtwDamageAdd(&s, &a));
    a = rect(-32768, -32768, 32767, 32767);
    CHECK(LtwDamageAdd(&s, &a));
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 20, 10);
    LtwDamageCommit(&s);
    LtwDamageBegin(&s); LtwDamageReset(&s);
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 20, 10);
    CHECK(LtwDamageGet(&other, &d)); same_rect(&d, 0, 0, 7, 9);
    CHECK(LtwDamageInit(&s, 3, 2)); /* Resize resets history/full. */
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 3, 2);
    CHECK(LtwDamageInit(&s, 32767, 32767));
    CHECK(LtwDamageGet(&s, &d)); same_rect(&d, 0, 0, 32767, 32767);
}

#define PW 17
#define PH 13
static unsigned char old_pixels[PH][PW], new_pixels[PH][PW];
static unsigned long rng = 0x13579UL;
static unsigned random_value(void)
{
    /* Fixed 32-bit generator even on LP64 hosts. */
    rng = (rng * 1664525UL + 1013904223UL) & 0xffffffffUL;
    return (unsigned)((rng >> 8) & 0xffffUL);
}
static void damage_property(void)
{
    LTWDAMAGE s;
    RECT r, d, again;
    int frame, n, x, y, count, full = 1, any, left, top, right, bottom;
    section = "2000-frame independent raster-union oracle";
    CHECK(LtwDamageInit(&s, PW, PH));
    memset(old_pixels, 0, sizeof(old_pixels));
    for (frame = 0; frame < 2000; frame++) {
        memset(new_pixels, 0, sizeof(new_pixels));
        LtwDamageBegin(&s);
        count = (int)(random_value() % 6U);
        for (n = 0; n < count; n++) {
            r = rect((int)(random_value() % 57U) - 20,
                     (int)(random_value() % 47U) - 17,
                     (int)(random_value() % 57U) - 20,
                     (int)(random_value() % 47U) - 17);
            LtwDamageAdd(&s, &r);
            for (y = 0; y < PH; y++) for (x = 0; x < PW; x++)
                if (x >= r.left && x < r.right && y >= r.top && y < r.bottom)
                    new_pixels[y][x] = 1;
        }
        if (frame % 37 == 0) { LtwDamageReset(&s); full = 1; }
        any = 0; left = PW; top = PH; right = bottom = 0;
        for (y = 0; y < PH; y++) for (x = 0; x < PW; x++) {
            if (full || old_pixels[y][x] || new_pixels[y][x]) {
                any = 1;
                if (x < left) left = x;
                if (y < top) top = y;
                if (x + 1 > right) right = x + 1;
                if (y + 1 > bottom) bottom = y + 1;
            }
        }
        CHECK(LtwDamageGet(&s, &d) == any);
        if (any) same_rect(&d, left, top, right, bottom);
        else same_rect(&d, 0, 0, 0, 0);
        CHECK(LtwDamageGet(&s, &again) == any);
        same_rect(&again, d.left, d.top, d.right, d.bottom);
        if (frame % 11 != 0) {
            LtwDamageCommit(&s);
            memcpy(old_pixels, new_pixels, sizeof(old_pixels));
            full = 0;
        } /* Otherwise simulate no presentation/commit and preserve history. */
    }
}

static void blank_canvas(LTWCANVAS *s)
{
    memset(s, 0, sizeof(*s));
}
static void clean_canvas(const LTWCANVAS *s)
{
    CHECK(!s->dc && !s->bitmap && !s->oldBitmap);
    CHECK(!s->width && !s->height && !s->mono);
    CHECK(memory_dcs == 0 && owned_bitmaps == 0);
}
static void surface_lifetime(void)
{
    LTWCANVAS s, second;
    HDC dc;
    HBITMAP bitmap;
    int i;
    section = "surface lifetime and bounds";
    reset_mock(); blank_canvas(&s); blank_canvas(&second);
    CHECK(!LtwCanvasDestroy(NULL));
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    CHECK(!LtwCanvasCreate(NULL, REF_DC, 20, 10, TRUE));
    CHECK(!LtwCanvasCreate(&s, 0, 20, 10, TRUE));
    CHECK(!LtwCanvasCreate(&s, REF_DC, 0, 10, TRUE));
    CHECK(!LtwCanvasCreate(&s, REF_DC, 10, -1, TRUE));
    CHECK(!LtwCanvasCreate(&s, REF_DC, 32768, 1, TRUE));
    CHECK(!LtwCanvasCreate(&s, REF_DC, 1, 32768, TRUE));
    CHECK(!LtwCanvasCreate(&s, REF_DC, 200, 200, TRUE));
    CHECK(!LtwCanvasCreate(&s, REF_DC, 1, 20000, TRUE));
    CHECK(calls[NEW_DC] == 0); clean_canvas(&s);
    CHECK(LtwCanvasCreate(&s, REF_DC, 224, 128, TRUE));
    CHECK(s.width == 224 && s.height == 128 && s.mono);
    CHECK(s.oldBitmap == STOCK_BITMAP && dcs[s.dc].selected == s.bitmap);
    CHECK(compatible_dc_reference == REF_DC);
    CHECK(calls[NEW_MONO] == 1 && calls[NEW_COLOR] == 0 && calls[PATBLT] == 1);
    CHECK(dcs[s.dc].text == RGB(0, 0, 0));
    CHECK(dcs[s.dc].background == RGB(255, 255, 255));
    CHECK(!LtwCanvasCreate(&s, REF_DC, 20, 10, FALSE));
    CHECK(calls[NEW_DC] == 1);
    CHECK(LtwCanvasCreate(&second, REF_DC, 30, 25, FALSE));
    CHECK(!second.mono && compatible_bitmap_reference == REF_DC);
    CHECK(calls[NEW_COLOR] == 1);
    dc = s.dc; bitmap = s.bitmap;
    CHECK(LtwCanvasDestroy(&s));
    CHECK(!dcs[dc].alive && !bitmaps[bitmap].alive);
    CHECK(last_select_sequence < last_delete_dc_sequence);
    CHECK(last_delete_dc_sequence < last_delete_bitmap_sequence);
    CHECK(memory_dcs == 1 && owned_bitmaps == 1);
    CHECK(LtwCanvasDestroy(&second)); clean_canvas(&second);
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    for (i = 0; i < 200; i++) {
        CHECK(LtwCanvasCreate(&s, REF_DC, 31, 17, i % 2));
        CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    }
    CHECK(LtwCanvasCreate(&s, REF_DC, 32760, 1, TRUE));
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
}
static void allocation_failures(void)
{
    LTWCANVAS s;
    int operation;
    section = "allocation and initialization failures";
    for (operation = NEW_DC; operation <= PATBLT; operation++) {
        if (operation == DELETE_DC || operation == DELETE_BITMAP) continue;
        reset_mock(); blank_canvas(&s);
        fail_at[operation] = 1;
        CHECK(!LtwCanvasCreate(&s, REF_DC, 40, 30, operation != NEW_COLOR));
        clean_canvas(&s);
        CHECK(LtwCanvasDestroy(&s));
    }
    section = "cleanup failures retain ownership for retry";
    reset_mock(); blank_canvas(&s);
    fail_at[NEW_MONO] = 1; fail_at[DELETE_DC] = 1;
    CHECK(!LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    CHECK(s.dc && !s.bitmap && memory_dcs == 1);
    CHECK(!LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    reset_mock(); blank_canvas(&s);
    fail_at[SELECT] = 1; fail_at[DELETE_BITMAP] = 1;
    CHECK(!LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    CHECK(!s.dc && s.bitmap && owned_bitmaps == 1);
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    reset_mock(); blank_canvas(&s);
    CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    fail_at[SELECT] = calls[SELECT] + 1;
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    reset_mock(); blank_canvas(&s);
    CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    fail_at[DELETE_DC] = calls[DELETE_DC] + 1;
    CHECK(!LtwCanvasDestroy(&s));
    CHECK(s.dc && s.bitmap && s.oldBitmap && calls[DELETE_BITMAP] == 0);
    CHECK(!s.width && !s.height && !s.mono);
    CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 0, NULL, 0, 0));
    CHECK(calls[BITBLT] == 0);
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    reset_mock(); blank_canvas(&s);
    CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    fail_at[DELETE_BITMAP] = calls[DELETE_BITMAP] + 1;
    CHECK(!LtwCanvasDestroy(&s));
    CHECK(!s.dc && s.bitmap && !s.oldBitmap);
    CHECK(!s.width && !s.height && !s.mono);
    CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 0, NULL, 0, 0));
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    reset_mock(); blank_canvas(&s);
    CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    fail_at[SELECT] = calls[SELECT] + 1; fail_at[DELETE_DC] = 1;
    CHECK(!LtwCanvasDestroy(&s));
    CHECK(dcs[s.dc].selected == s.bitmap && calls[DELETE_BITMAP] == 0);
    CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 0, NULL, 0, 0));
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
}

static void presentation(void)
{
    LTWCANVAS s;
    RECT r;
    DWORD old_text, old_bk;
    int text_calls, bk_calls, blt_calls;
    section = "presentation geometry, clipping and explicit mono colors";
    reset_mock(); blank_canvas(&s);
    CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 0, NULL, 0, 0));
    CHECK(!LtwCanvasPresent(NULL, DEST_DC, 0, 0, NULL, 0, 0));
    CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    old_text = dcs[DEST_DC].text; old_bk = dcs[DEST_DC].background;
    CHECK(!LtwCanvasPresent(&s, 0, 0, 0, NULL, 0, 0));
    r = rect(-8, 4, 60, 12);
    CHECK(LtwCanvasPresent(&s, DEST_DC, 100, 200, &r, RGB(9, 8, 7), RGB(6, 5, 4)));
    CHECK(blt_x == 100 && blt_y == 204 && blt_w == 40 && blt_h == 8);
    CHECK(blt_sx == 0 && blt_sy == 4 && blt_source == s.dc && blt_destination == DEST_DC);
    CHECK(blt_text == RGB(9, 8, 7) && blt_background == RGB(6, 5, 4));
    CHECK(dcs[DEST_DC].text == old_text && dcs[DEST_DC].background == old_bk);
    r = rect(3, 7, 11, 19);
    CHECK(LtwCanvasPresent(&s, DEST_DC, -20, -20, &r, 0, 0xffffffU));
    CHECK(blt_x == -17 && blt_y == -13 && blt_w == 8 && blt_h == 12);
    CHECK(blt_sx == 3 && blt_sy == 7);
    CHECK(LtwCanvasPresent(&s, DEST_DC, -32768, -32768, NULL, 0, 0xffffffU));
    CHECK(blt_x == -32768 && blt_y == -32768 && blt_w == 40 && blt_h == 30);
    CHECK(LtwCanvasPresent(&s, DEST_DC, 32727, 32737, NULL, 0, 0xffffffU));
    blt_calls = calls[BITBLT]; text_calls = calls[TEXT_COLOR]; bk_calls = calls[BK_COLOR];
    CHECK(!LtwCanvasPresent(&s, DEST_DC, 32728, 0, NULL, 0, 0));
    CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 32738, NULL, 0, 0));
    r = rect(40, 0, 50, 10);
    CHECK(LtwCanvasPresent(&s, DEST_DC, 0, 0, &r, 0, 0));
    r = rect(0, 3, 0, 8); CHECK(LtwCanvasPresent(&s, DEST_DC, 0, 0, &r, 0, 0));
    r = rect(9, 9, 2, 2); CHECK(LtwCanvasPresent(&s, DEST_DC, 0, 0, &r, 0, 0));
    CHECK(calls[BITBLT] == blt_calls && calls[TEXT_COLOR] == text_calls && calls[BK_COLOR] == bk_calls);
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, FALSE));
    text_calls = calls[TEXT_COLOR]; bk_calls = calls[BK_COLOR];
    CHECK(LtwCanvasPresent(&s, DEST_DC, 10, 20, NULL, 0, 0));
    CHECK(calls[TEXT_COLOR] == text_calls && calls[BK_COLOR] == bk_calls);
    CHECK(blt_text == old_text && blt_background == old_bk);
    CHECK(blt_x == 10 && blt_y == 20 && blt_w == 40 && blt_h == 30);
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
}
static void presentation_failures(void)
{
    LTWCANVAS s;
    LTWDAMAGE damage;
    RECT r, d;
    DWORD old_text, old_bk;
    int operation, text_calls, bk_calls;
    section = "presentation failure restoration and uncommitted damage";
    for (operation = TEXT_COLOR; operation <= BITBLT; operation++) {
        if (operation == PATBLT) continue;
        reset_mock(); blank_canvas(&s);
        CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
        CHECK(LtwDamageInit(&damage, 40, 30));
        r = rect(2, 3, 8, 9); CHECK(LtwDamageAdd(&damage, &r));
        LtwDamageCommit(&damage); LtwDamageBegin(&damage);
        r = rect(20, 21, 28, 29); CHECK(LtwDamageAdd(&damage, &r));
        CHECK(LtwDamageGet(&damage, &d));
        old_text = dcs[DEST_DC].text; old_bk = dcs[DEST_DC].background;
        fail_at[operation] = calls[operation] + 1;
        CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 0, &d, 0, 0xffffffU));
        CHECK(dcs[DEST_DC].text == old_text && dcs[DEST_DC].background == old_bk);
        CHECK(damage.havePrevious && damage.previous.left == 2);
        CHECK(LtwDamageGet(&damage, &d)); same_rect(&d, 2, 3, 28, 29);
        if (operation != BITBLT) CHECK(calls[BITBLT] == 0);
        LtwDamageReset(&damage);
        CHECK(LtwDamageGet(&damage, &d)); same_rect(&d, 0, 0, 40, 30);
        CHECK(LtwCanvasPresent(&s, DEST_DC, 0, 0, &d, 0, 0xffffffU));
        LtwDamageCommit(&damage);
        CHECK(damage.previous.left == 20 && !damage.full);
        CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    }
    section = "failed setup attempts rollback even when rollback fails";
    reset_mock(); blank_canvas(&s);
    CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
    text_calls = calls[TEXT_COLOR]; bk_calls = calls[BK_COLOR];
    old_bk = dcs[DEST_DC].background;
    fail_at[BK_COLOR] = calls[BK_COLOR] + 1;
    fail_at[TEXT_COLOR] = calls[TEXT_COLOR] + 2;
    CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 0, NULL, 0, 0xffffffU));
    CHECK(calls[TEXT_COLOR] == text_calls + 2 && calls[BK_COLOR] == bk_calls + 1);
    CHECK(calls[BITBLT] == 0 && dcs[DEST_DC].background == old_bk);
    CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    section = "restoration failure is reported and other color still restored";
    for (operation = TEXT_COLOR; operation <= BK_COLOR; operation++) {
        reset_mock(); blank_canvas(&s);
        CHECK(LtwCanvasCreate(&s, REF_DC, 40, 30, TRUE));
        old_text = dcs[DEST_DC].text; old_bk = dcs[DEST_DC].background;
        text_calls = calls[TEXT_COLOR]; bk_calls = calls[BK_COLOR];
        fail_at[operation] = calls[operation] + 2;
        CHECK(!LtwCanvasPresent(&s, DEST_DC, 0, 0, NULL, 0, 0xffffffU));
        CHECK(calls[TEXT_COLOR] == text_calls + 2 && calls[BK_COLOR] == bk_calls + 2);
        CHECK(calls[BITBLT] == 1);
        if (operation == TEXT_COLOR) CHECK(dcs[DEST_DC].background == old_bk);
        else CHECK(dcs[DEST_DC].text == old_text);
        CHECK(LtwCanvasDestroy(&s)); clean_canvas(&s);
    }
}
int main(void)
{
    section = "host type preflight";
    CHECK(sizeof(short) == 2 && SHRT_MIN == -32768 && SHRT_MAX == 32767);
    CHECK(sizeof(DWORD) == 4 && UINT_MAX == 0xffffffffU);
    CHECK(sizeof(RECT) == 8 && sizeof(HDC) == 2);
    damage_cases(); damage_property(); surface_lifetime();
    allocation_failures(); presentation(); presentation_failures();
    printf("PASS: %lu assertions; 2000 damage frames; 200 lifecycle cycles.\n", checks);
    puts("Actual WTCANVAS.C with host GDI mocks only; no native ABI/visual/timing claim.");
    return 0;
}
