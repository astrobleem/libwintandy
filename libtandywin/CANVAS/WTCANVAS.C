#define WINVER 0x0300
#include <windows.h>
#include "WTCANVAS.H"

static void empty_rect(RECT FAR *r)
{
    r->left = r->top = r->right = r->bottom = 0;
}
static BOOL dimensions(int width, int height)
{
    return width > 0 && height > 0 && width <= 32767 && height <= 32767;
}
static BOOL clip_rect(const RECT FAR *in, int width, int height,
                      RECT FAR *out)
{
    RECT r;
    empty_rect(&r);
    if (in && dimensions(width, height)) {
        r = *in;
        if (r.left < 0) r.left = 0;
        if (r.top < 0) r.top = 0;
        if (r.right > width) r.right = (short)width;
        if (r.bottom > height) r.bottom = (short)height;
        if (r.left >= r.right || r.top >= r.bottom) empty_rect(&r);
    }
    *out = r;
    return r.right > r.left && r.bottom > r.top;
}
static void union_rect(RECT FAR *a, const RECT FAR *b)
{
    if (b->left < a->left) a->left = b->left;
    if (b->top < a->top) a->top = b->top;
    if (b->right > a->right) a->right = b->right;
    if (b->bottom > a->bottom) a->bottom = b->bottom;
}
BOOL LtwDamageInit(LTWDAMAGE FAR *s, int width, int height)
{
    if (!s) return FALSE;
    s->width = s->height = 0;
    empty_rect(&s->previous);
    empty_rect(&s->current);
    s->havePrevious = s->haveCurrent = s->full = FALSE;
    if (!dimensions(width, height)) return FALSE;
    s->width = width;
    s->height = height;
    s->full = TRUE;
    return TRUE;
}
void LtwDamageReset(LTWDAMAGE FAR *s)
{
    if (s) s->full = TRUE;
}
void LtwDamageBegin(LTWDAMAGE FAR *s)
{
    if (!s) return;
    s->haveCurrent = FALSE;
    empty_rect(&s->current);
}
BOOL LtwDamageAdd(LTWDAMAGE FAR *s, const RECT FAR *object)
{
    RECT r;
    if (!s || !clip_rect(object, s->width, s->height, &r)) return FALSE;
    if (s->haveCurrent) union_rect(&s->current, &r);
    else s->current = r;
    s->haveCurrent = TRUE;
    return TRUE;
}
BOOL LtwDamageGet(const LTWDAMAGE FAR *s, RECT FAR *dirty)
{
    if (!dirty) return FALSE;
    empty_rect(dirty);
    if (!s || !dimensions(s->width, s->height)) return FALSE;
    if (s->full) {
        dirty->right = (short)s->width;
        dirty->bottom = (short)s->height;
        return TRUE;
    }
    if (s->havePrevious) *dirty = s->previous;
    if (s->haveCurrent) {
        if (s->havePrevious) union_rect(dirty, &s->current);
        else *dirty = s->current;
    }
    return s->havePrevious || s->haveCurrent;
}
void LtwDamageCommit(LTWDAMAGE FAR *s)
{
    if (!s) return;
    s->previous = s->current;
    s->havePrevious = s->haveCurrent;
    s->full = FALSE;
}

BOOL LtwCanvasDestroy(LTWCANVAS FAR *s)
{
    if (!s) return FALSE;
    /* A partly destroyed canvas must never be presented. */
    s->width = s->height = 0;
    s->mono = FALSE;
    if (s->dc) {
        if (s->oldBitmap) SelectObject(s->dc, s->oldBitmap);
        /* DeleteDC detaches even if restoring the selection failed. Never
         * delete a bitmap while a live DC may still have it selected. */
        if (!DeleteDC(s->dc)) return FALSE;
        s->dc = 0;
        s->oldBitmap = 0;
    }
    if (s->bitmap) {
        if (!DeleteObject(s->bitmap)) return FALSE;
        s->bitmap = 0;
    }
    s->oldBitmap = 0;
    s->width = s->height = 0;
    s->mono = FALSE;
    return TRUE;
}
BOOL LtwCanvasCreate(LTWCANVAS FAR *s, HDC reference,
                     int width, int height, BOOL mono)
{
    if (!s || s->dc || s->bitmap || s->oldBitmap || !reference ||
        !dimensions(width, height) ||
        (long)width * height > LTW_CANVAS_MAX_PIXELS ||
        ((long)width + 15L) / 16L * 2L * height >
            LTW_CANVAS_MAX_MONO_BYTES) return FALSE;
    s->width = s->height = 0;
    s->mono = FALSE;
    s->dc = CreateCompatibleDC(reference);
    if (!s->dc) return FALSE;
    s->bitmap = mono ? CreateBitmap(width, height, 1, 1, (LPSTR)0) :
                      CreateCompatibleBitmap(reference, width, height);
    if (!s->bitmap) goto fail;
    s->oldBitmap = SelectObject(s->dc, s->bitmap);
    if (!s->oldBitmap) goto fail;
    if (SetTextColor(s->dc, RGB(0, 0, 0)) == 0xffffffffUL ||
        SetBkColor(s->dc, RGB(255, 255, 255)) == 0xffffffffUL ||
        !PatBlt(s->dc, 0, 0, width, height, BLACKNESS)) goto fail;
    s->width = width;
    s->height = height;
    s->mono = mono != FALSE;
    return TRUE;
fail:
    /* Failed cleanup leaves handles owned by s, so Destroy can be retried. */
    LtwCanvasDestroy(s);
    return FALSE;
}
BOOL LtwCanvasPresent(const LTWCANVAS FAR *s, HDC destination,
                      int x, int y, const RECT FAR *rect,
                      DWORD zeroColor, DWORD oneColor)
{
    RECT r;
    DWORD oldText, oldBk;
    BOOL ok;
    if (!s || !s->dc || !s->bitmap || !s->oldBitmap || !destination ||
        !dimensions(s->width, s->height)) return FALSE;
    if (rect) {
        if (!clip_rect(rect, s->width, s->height, &r)) return TRUE;
    } else {
        r.left = r.top = 0;
        r.right = (short)s->width;
        r.bottom = (short)s->height;
    }
    /* Check before 16-bit addition. Exclusive right/bottom must fit too. */
    if ((long)x + r.left < -32768L || (long)y + r.top < -32768L ||
        (long)x + r.right > 32767L || (long)y + r.bottom > 32767L)
        return FALSE;
    oldText = oldBk = 0;
    if (s->mono) {
        oldText = SetTextColor(destination, zeroColor);
        if (oldText == 0xffffffffUL) return FALSE;
        oldBk = SetBkColor(destination, oneColor);
        if (oldBk == 0xffffffffUL) {
            SetTextColor(destination, oldText);
            return FALSE;
        }
    }
    ok = BitBlt(destination, x + r.left, y + r.top,
                r.right - r.left, r.bottom - r.top,
                s->dc, r.left, r.top, SRCCOPY);
    if (s->mono) {
        if (SetBkColor(destination, oldBk) == 0xffffffffUL) ok = FALSE;
        if (SetTextColor(destination, oldText) == 0xffffffffUL) ok = FALSE;
    }
    return ok;
}
