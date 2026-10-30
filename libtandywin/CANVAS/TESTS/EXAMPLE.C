/* Execute the unchanged example window procedure with host USER/GDI mocks.
 * This proves message/call/state behavior, not actual Windows clipping. */
#include "EXSTUB/windows.h"
#define main LibraryContractMain
#define PatBlt InitOnlyPatBlt
#include "CANVTEST.C"
#undef PatBlt
#undef main
#undef NULL
#define NULL 0
#include "../../../examples/CANVAS30/CANVAS.C"

#define WINDOW 1
static RECT client_rect, update_rect;
static BOOL update_pending, iconic;
static int invalidations, begins, ends, timers, kills, quit;
static int before_begin_invalidations;
static const char *title;
static int screen_width, screen_height, created_x, created_y;
static int created_width, created_height;
static int fill_calls, fail_fill_at;
static int script_mode, script_ticks, retry_failures, create_calls;
#define TASK_TIMER 77

int GetSystemMetrics(int index)
{
    return index == SM_CXSCREEN ? screen_width : screen_height;
}

void GetClientRect(HWND window, RECT *client)
{
    CHECK(window == WINDOW);
    *client = client_rect;
}
HDC GetDC(HWND window)
{
    CHECK(window == WINDOW);
    return REF_DC;
}
void ReleaseDC(HWND window, HDC dc)
{
    CHECK(window == WINDOW && dc == REF_DC);
}
UINT SetTimer(HWND window, UINT id, UINT milliseconds, void *callback)
{
    CHECK(!window && id == 0 && milliseconds == 100 && !callback);
    timers++;
    return script_mode == 4 ? 0 : TASK_TIMER;
}
void KillTimer(HWND window, UINT id)
{
    CHECK(!window && id == TASK_TIMER);
    CHECK(!memory_dcs && !owned_bitmaps);
    kills++;
}
void SetWindowText(HWND window, const char *text)
{
    CHECK(window == WINDOW && text);
    title = text;
}
BOOL InvalidateRect(HWND window, const RECT *r, BOOL erase)
{
    RECT clipped;
    CHECK(window == WINDOW && !erase);
    invalidations++;
    clipped = r ? *r : client_rect;
    if (clipped.left < 0) clipped.left = 0;
    if (clipped.top < 0) clipped.top = 0;
    if (clipped.right > client_rect.right) clipped.right = client_rect.right;
    if (clipped.bottom > client_rect.bottom) clipped.bottom = client_rect.bottom;
    if (clipped.left >= clipped.right || clipped.top >= clipped.bottom) return TRUE;
    if (!update_pending) update_rect = clipped;
    else {
        if (clipped.left < update_rect.left) update_rect.left = clipped.left;
        if (clipped.top < update_rect.top) update_rect.top = clipped.top;
        if (clipped.right > update_rect.right) update_rect.right = clipped.right;
        if (clipped.bottom > update_rect.bottom) update_rect.bottom = clipped.bottom;
    }
    update_pending = TRUE;
    return TRUE;
}
BOOL IsIconic(HWND window)
{
    CHECK(window == WINDOW);
    return iconic;
}
HDC BeginPaint(HWND window, PAINTSTRUCT *paint)
{
    CHECK(window == WINDOW && update_pending);
    paint->rcPaint = update_rect;
    update_pending = FALSE;
    before_begin_invalidations = invalidations;
    begins++;
    return DEST_DC;
}
void EndPaint(HWND window, const PAINTSTRUCT *paint)
{
    CHECK(window == WINDOW && paint);
    ends++;
}
HGDIOBJ GetStockObject(int object)
{
    CHECK(object == BLACK_BRUSH);
    return 42;
}
int FillRect(HDC dc, const RECT *r, HGDIOBJ brush)
{
    CHECK(dc == DEST_DC && brush == 42);
    CHECK(r->left < r->right && r->top < r->bottom);
    fill_calls++;
    if (fill_calls == fail_fill_at) return FALSE;
    return TRUE;
}
BOOL PatBlt(HDC dc, int x, int y, int width, int height, DWORD rop)
{
    dc_valid(dc);
    CHECK(rop == BLACKNESS || rop == WHITENESS);
    CHECK(dcs[dc].selected > STOCK_BITMAP);
    CHECK(x >= 0 && y >= 0 && width > 0 && height > 0);
    CHECK(x + width <= bitmaps[dcs[dc].selected].width);
    CHECK(y + height <= bitmaps[dcs[dc].selected].height);
    return !fail(PATBLT);
}
void PostQuitMessage(int code)
{
    CHECK(code == 0);
    CHECK(!memory_dcs && !owned_bitmaps);
    quit++;
}
long DefWindowProc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    (void)window; (void)message; (void)wp; (void)lp;
    return 19L;
}
HICON LoadIcon(HINSTANCE instance, const char *name)
{
    (void)instance; (void)name;
    return 1;
}
HCURSOR LoadCursor(HINSTANCE instance, const char *name)
{
    (void)instance; (void)name;
    return 1;
}
BOOL RegisterClass(const WNDCLASS *wc)
{
    (void)wc;
    return TRUE;
}
HWND CreateWindow(const char *class_name, const char *window_title, DWORD style,
                  int x, int y, int width, int height, HWND parent,
                  unsigned short menu, HINSTANCE instance, void *parameter)
{
    (void)class_name; (void)window_title; (void)style;
    created_x = x; created_y = y;
    created_width = width; created_height = height;
    (void)parent; (void)menu; (void)instance;
    (void)parameter;
    create_calls++;
    if (script_mode == 5) return 0;
    if (script_mode >= 1 && script_mode <= 3) {
        CHECK(CanvasProc(WINDOW, WM_CREATE, 0, 0) == 0L);
        CHECK(CanvasProc(WINDOW, WM_SIZE, 0, 0) == 0L);
        CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
        fail_at[script_mode == 2 ? DELETE_BITMAP : DELETE_DC] =
            calls[script_mode == 2 ? DELETE_BITMAP : DELETE_DC] + 1;
        CHECK(CanvasProc(WINDOW, WM_DESTROY, 0, 0) == 0L);
        CHECK(!quit && windowDestroyed && !canvas.width);
    }
    return WINDOW;
}
void ShowWindow(HWND window, int show) { (void)window; (void)show; }
void UpdateWindow(HWND window) { (void)window; }
BOOL GetMessage(MSG *message, HWND window, UINT first, UINT last)
{
    (void)window; (void)first; (void)last;
    message->wParam = 0;
    if (script_mode >= 1 && script_mode <= 3 && !quit) {
        CHECK(timers == 1 && kills == 0);
        CHECK(windowDestroyed && owned_bitmaps == 1);
        CHECK(memory_dcs == (script_mode == 2 ? 0 : 1));
        if (script_mode == 3 && retry_failures > 0) {
            fail_at[DELETE_DC] = calls[DELETE_DC] + 1;
            retry_failures--;
        }
        script_ticks++;
        CHECK(script_ticks <= 4);
        message->hwnd = 0; message->message = WM_TIMER;
        message->wParam = TASK_TIMER;
        return TRUE;
    }
    return FALSE;
}
void TranslateMessage(const MSG *message) { (void)message; }
void DispatchMessage(const MSG *message) { (void)message; }
long SendMessage(HWND window, UINT message, WPARAM wp, LPARAM lp)
{ CHECK(window == WINDOW); return CanvasProc(window, message, wp, lp); }

int main(void)
{
    RECT expose;
    int initial_invalidations, old_calls, i;
    section = "actual example failed-paint partial-expose recovery";
    reset_mock();
    client_rect = rect(0, 0, 40, 30);
    CHECK(CanvasProc(WINDOW, WM_CREATE, 0, 0) == 0L && timers == 0);
    CHECK(CanvasProc(WINDOW, WM_SIZE, 0, 0) == 0L);
    CHECK(update_pending && canvas.width == 40 && canvas.height == 30);
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(!damage.full && damage.havePrevious && damage.previous.left == 0);
    CHECK(blt_w == 40 && blt_h == 30 && begins == ends);
    CHECK(CanvasProc(WINDOW, WM_TIMER, 1, 0) == 0L);
    CHECK(position == 1 && update_pending);
    fail_at[BITBLT] = calls[BITBLT] + 1;
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(damage.full && damage.previous.left == 0 && !update_pending);
    CHECK(title && strstr(title, "failed"));
    /* External exposure happens BEFORE the next timer can invalidate full. */
    expose = rect(35, 25, 40, 30);
    CHECK(InvalidateRect(WINDOW, &expose, FALSE));
    initial_invalidations = invalidations;
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(before_begin_invalidations == initial_invalidations + 1);
    CHECK(blt_x == 0 && blt_y == 0 && blt_w == 40 && blt_h == 30);
    CHECK(!damage.full && damage.previous.left == 1 && !update_pending);
    CHECK(begins == ends);

    section = "actual example coalesced timers and outside exposure";
    for (i = 0; i < 5; i++) CHECK(CanvasProc(WINDOW, WM_TIMER, 1, 0) == 0L);
    CHECK(position == 6 && damage.previous.left == 1 && update_pending);
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(damage.previous.left == 6);
    CHECK(blt_x == 1 && blt_w == 17);
    expose = rect(35, 25, 40, 30);
    CHECK(InvalidateRect(WINDOW, &expose, FALSE));
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(blt_x <= 6 && blt_x + blt_w == 40);

    section = "actual example resize, minimize, zero size, destroy";
    iconic = TRUE;
    old_calls = calls[BITBLT];
    CHECK(CanvasProc(WINDOW, WM_TIMER, 1, 0) == 0L);
    CHECK(position == 6 && calls[BITBLT] == old_calls);
    CHECK(CanvasProc(WINDOW, WM_SIZE, SIZEICONIC, 0) == 0L);
    CHECK(canvas.width == 40);
    iconic = FALSE;
    client_rect = rect(0, 0, 8, 6);
    CHECK(CanvasProc(WINDOW, WM_SIZE, 0, 0) == 0L);
    CHECK(canvas.width == 8 && canvas.height == 6 && damage.full);
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(blt_x == 0 && blt_y == 0 && blt_w == 8 && blt_h == 6);
    client_rect = rect(0, 0, 0, 0);
    CHECK(CanvasProc(WINDOW, WM_SIZE, 0, 0) == 0L);
    CHECK(!canvas.width && !canvas.height && !memory_dcs && !owned_bitmaps);
    client_rect = rect(0, 0, 190, 120);
    CHECK(CanvasProc(WINDOW, WM_SIZE, 0, 0) == 0L);
    CHECK(canvas.width == 160 && canvas.height == 96);
    fail_fill_at = fill_calls + 1;
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(retryPaint && paintFailed && damage.full && !update_pending);
    CHECK(CanvasProc(WINDOW, WM_TIMER, 1, 0) == 0L);
    CHECK(update_pending && update_rect.right == 190 && update_rect.bottom == 120);
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(!retryPaint && !paintFailed && !damage.full);
    CHECK(title && !strstr(title, "failed"));
    /* Bottom-strip failure with a marker-only expose before the next tick. */
    fail_fill_at = fill_calls + 2;
    CHECK(InvalidateRect(WINDOW, NULL, FALSE));
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(retryPaint && damage.full);
    expose = rect(5, 5, 7, 7);
    CHECK(InvalidateRect(WINDOW, &expose, FALSE));
    CHECK(CanvasProc(WINDOW, WM_PAINT, 0, 0) == 0L);
    CHECK(update_rect.right == 190 && update_rect.bottom == 120);
    CHECK(!retryPaint && !damage.full);
    CHECK(blt_w == 160 && blt_h == 96);
    CHECK(CanvasProc(WINDOW, WM_DESTROY, 0, 0) == 0L);
    CHECK(kills == 0 && quit == 1);
    clean_canvas(&canvas);
    section = "actual WinMain initial outer-frame fit";
    screen_width = 160; screen_height = 200;
    CHECK(WinMain(1, 0, "", 5) == 0);
    CHECK(created_width == 160 && created_height == 150);
    CHECK(created_x == 0 && created_y == 0);
    screen_width = 320; screen_height = 200;
    CHECK(WinMain(1, 0, "", 5) == 0);
    CHECK(created_width == 220 && created_height == 150);
    CHECK(created_x == CW_USEDEFAULT && created_y == CW_USEDEFAULT);
    section = "actual WinMain retries retained cleanup before quit";
    for (i = 1; i <= 3; i++) {
        reset_mock(); quit = kills = timers = script_ticks = 0;
        script_mode = i; retry_failures = i == 3 ? 2 : 0;
        client_rect = rect(0, 0, 40, 30);
        CHECK(WinMain(1, 0, "", 5) == 0);
        CHECK(script_ticks == (i == 3 ? 3 : 1));
        CHECK(quit == 1 && kills == 1 && timers == 1);
        clean_canvas(&canvas);
    }
    section = "timer reservation and window creation failure";
    script_mode = 4; create_calls = timers = kills = quit = 0;
    CHECK(WinMain(1, 0, "", 5) == 1);
    CHECK(!create_calls && timers == 1 && !kills && !quit);
    script_mode = 5; create_calls = timers = kills = quit = 0;
    CHECK(WinMain(1, 0, "", 5) == 1);
    CHECK(create_calls == 1 && timers == 1 && kills == 1 && !quit);
    script_mode = 0;
    screen_width = 160; screen_height = 100;
    CHECK(WinMain(1, 0, "", 5) == 0);
    CHECK(created_width == 160 && created_height == 100);
    CHECK(created_x == 0 && created_y == 0);
    screen_width = screen_height = 0;
    CHECK(WinMain(1, 0, "", 5) == 0);
    CHECK(created_width == 220 && created_height == 150);
    CHECK(created_x == CW_USEDEFAULT && created_y == CW_USEDEFAULT);
    printf("PASS: %lu example assertions; failed-paint/expose, coalesced timers, resize/minimize/zero-size/destroy.\n", checks);
    puts("Actual CANVAS.C window procedure with host USER/GDI mocks; not native visual QA.");
    return 0;
}
