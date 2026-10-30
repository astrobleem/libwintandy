/* Optional Windows 3.0 / 8088 canvas helpers. Include WINDOWS.H first. */
#ifndef WTCANVAS_H
#define WTCANVAS_H

/* Right/bottom are exclusive. Coordinates are local to an unchanged canvas.
 * Add ALL current objects each frame, including pen/effect padding. */
typedef struct {
    int width, height;
    RECT previous, current;
    BOOL havePrevious, haveCurrent, full;
} LTWDAMAGE;

BOOL LtwDamageInit(LTWDAMAGE FAR *state, int width, int height);
void LtwDamageReset(LTWDAMAGE FAR *state); /* next damage is full */
void LtwDamageBegin(LTWDAMAGE FAR *state); /* begin current object union */
BOOL LtwDamageAdd(LTWDAMAGE FAR *state, const RECT FAR *object);
BOOL LtwDamageGet(const LTWDAMAGE FAR *state, RECT FAR *dirty);
/* Commit only after successful clear + draw + presentation of ALL damage.
 * After any partial/failed drawing, Reset before trying the next frame. */
void LtwDamageCommit(LTWDAMAGE FAR *state);

/* Zero initialize. Owns one DC + bitmap; never copy a live state. The caller
 * borrows dc to draw, restoring pens/brushes and keeping its bitmap selected,
 * MM_TEXT mapping, zero origin, no clip region and default mono colors.
 * No timers, windows, VRAM, global state or implicit screen ownership. */
#define LTW_CANVAS_MAX_PIXELS 32760L
#define LTW_CANVAS_MAX_MONO_BYTES 32760L
typedef struct {
    HDC dc;
    HBITMAP bitmap, oldBitmap;
    int width, height;
    BOOL mono;
} LTWCANVAS;

/* Explicit small dimensions; initially black. Color uses reference, NOT the
 * freshly-created (monochrome) memory DC. Refuse a nonempty state. */
BOOL LtwCanvasCreate(LTWCANVAS FAR *state, HDC reference,
                     int width, int height, BOOL mono);
/* Restores original bitmap, deletes DC before bitmap; retry if FALSE. */
BOOL LtwCanvasDestroy(LTWCANVAS FAR *state);
/* NULL rect means full canvas. Empty/clipped-away rect succeeds as a no-op.
 * Caller owns MM_TEXT destination DC and its lifetime/clip. Mono expansion
 * maps 0 to zeroColor and 1 to oneColor, then restores both destination colors.
 * Color canvases ignore these colors. Failure never commits damage. */
BOOL LtwCanvasPresent(const LTWCANVAS FAR *state, HDC destination,
                      int x, int y, const RECT FAR *rect,
                      DWORD zeroColor, DWORD oneColor);
#endif
