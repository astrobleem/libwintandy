/* Moving-marker consumer of WTCANVAS; no Cube geometry or driver access. */
#define WINVER 0x0300
#include <windows.h>
#include "WTCANVAS.H"

static LTWCANVAS canvas;
static LTWDAMAGE damage;
static RECT marker;
static int position, direction = 1;
static BOOL windowDestroyed, retryPaint, paintFailed;

static void model(void)
{
    int size;
    size = canvas.width < 12 ? canvas.width : 12;
    if (size > canvas.height) size = canvas.height;
    if (position > canvas.width - size) position = canvas.width - size;
    if (position < 0) position = 0;
    marker.left = position;
    marker.right = position + size;
    marker.top = (canvas.height - size) / 2;
    marker.bottom = marker.top + size;
    LtwDamageBegin(&damage);
    LtwDamageAdd(&damage, &marker);
}
static BOOL resize_canvas(HWND window)
{
    RECT client;
    HDC dc;
    int width, height;
    BOOL ok;
    if (!LtwCanvasDestroy(&canvas)) return FALSE;
    LtwDamageInit(&damage, 0, 0);
    GetClientRect(window, &client);
    width = client.right < 160 ? client.right : 160;
    height = client.bottom < 96 ? client.bottom : 96;
    if (width <= 0 || height <= 0) return TRUE;
    dc = GetDC(window);
    if (!dc) return FALSE;
    ok = LtwCanvasCreate(&canvas, dc, width, height, TRUE);
    ReleaseDC(window, dc);
    if (!ok) return FALSE;
    LtwDamageInit(&damage, width, height); /* recreation forces full damage */
    model();
    return TRUE;
}
static BOOL draw_model(void)
{
    RECT dirty;
    if (!LtwDamageGet(&damage, &dirty)) return TRUE;
    if (!PatBlt(canvas.dc, dirty.left, dirty.top,
                dirty.right - dirty.left, dirty.bottom - dirty.top,
                BLACKNESS)) return FALSE;
    return PatBlt(canvas.dc, marker.left, marker.top,
                  marker.right - marker.left, marker.bottom - marker.top,
                  WHITENESS);
}
static BOOL background(HWND window, HDC dc)
{
    RECT client, r;
    BOOL ok = TRUE;
    GetClientRect(window, &client);
    /* Fill only outside the bounded canvas. BeginPaint owns expose clipping. */
    r = client;
    r.left = canvas.width;
    if (r.left < r.right &&
        !FillRect(dc, &r, GetStockObject(BLACK_BRUSH))) ok = FALSE;
    r = client;
    r.right = canvas.width;
    r.top = canvas.height;
    if (r.top < r.bottom && r.right > 0)
        if (!FillRect(dc, &r, GetStockObject(BLACK_BRUSH))) ok = FALSE;
    return ok;
}
long FAR PASCAL CanvasProc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    RECT dirty;
    PAINTSTRUCT paint;
    HDC dc;
    BOOL ok;
    switch (message) {
    case WM_CREATE:
        windowDestroyed = FALSE;
        return 0L;
    case WM_SIZE:
        if (wp != SIZEICONIC) {
            ok = resize_canvas(window);
            retryPaint = TRUE;
            SetWindowText(window, ok ? "Bounded mono canvas" :
                                      "Canvas allocation failed - resize to retry");
            InvalidateRect(window, NULL, FALSE);
        }
        return 0L;
    case WM_TIMER:
        if (wp != 1 || windowDestroyed || IsIconic(window)) return 0L;
        if (retryPaint) InvalidateRect(window, NULL, FALSE);
        if (!canvas.width) return 0L;
        position += direction;
        if (position <= 0 || position >= canvas.width -
            (marker.right - marker.left)) direction = -direction;
        model();
        /* Always invalidate the whole uncommitted old/current union. Thus
         * coalesced timers/paints cannot lose damage before Commit below. */
        if (LtwDamageGet(&damage, &dirty))
            InvalidateRect(window, &dirty, FALSE);
        return 0L;
    case WM_ERASEBKGND:
        return 1L;
    case WM_PAINT:
        /* A prior failed draw may have forced full damage without a timer
         * yet. Include it BEFORE BeginPaint, so Commit can never discard
         * damage excluded by an unrelated expose's update region. */
        if (retryPaint) InvalidateRect(window, NULL, FALSE);
        else if (LtwDamageGet(&damage, &dirty))
            InvalidateRect(window, &dirty, FALSE);
        dc = BeginPaint(window, &paint);
        ok = background(window, dc);
        if (canvas.width) {
            if (ok) ok = draw_model();
            /* Expose can exceed animation damage: copy the actual paint
             * bounds from the initialized backing surface, not merely dirty.
             * GDI retains the possibly nonrectangular BeginPaint clip. */
            if (ok) ok = LtwCanvasPresent(&canvas, dc, 0, 0, &paint.rcPaint,
                                         RGB(0, 0, 0), RGB(255, 255, 255));
            if (ok) LtwDamageCommit(&damage);
            else LtwDamageReset(&damage);
        }
        EndPaint(window, &paint);
        retryPaint = !ok;
        if (ok && paintFailed && canvas.width)
            SetWindowText(window, "Bounded mono canvas");
        paintFailed = !ok;
        if (!ok) {
            /* Do not spin a failed WM_PAINT. The next timer invalidates the
             * full canvas and retries; retained old bounds are not committed. */
            SetWindowText(window, "Canvas drawing failed - retrying");
        }
        return 0L;
    case WM_DESTROY:
        windowDestroyed = TRUE;
        /* The task timer outlives this HWND. Retain ownership on failure;
         * WinMain retries on later ticks and only quits after full cleanup. */
        if (LtwCanvasDestroy(&canvas)) PostQuitMessage(0);
        return 0L;
    }
    return DefWindowProc(window, message, wp, lp);
}
int PASCAL WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command, int show)
{
    WNDCLASS wc;
    HWND window;
    MSG message;
    UINT timer;
    int x = CW_USEDEFAULT, y = CW_USEDEFAULT;
    int width = 220, height = 150, screen;
    (void)command;
    if (!previous) {
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = CanvasProc;
        wc.cbClsExtra = wc.cbWndExtra = 0;
        wc.hInstance = instance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;
        wc.lpszMenuName = NULL;
        wc.lpszClassName = "LtwCanvas30";
        if (!RegisterClass(&wc)) return 1;
    }
    /* Default USER placement can leave the outer frame off a compact display.
     * Clamp the initial outer size and use an explicit origin when needed. */
    screen = GetSystemMetrics(SM_CXSCREEN);
    if (screen > 0 && width > screen) { width = screen; x = y = 0; }
    screen = GetSystemMetrics(SM_CYSCREEN);
    if (screen > 0 && height > screen) { height = screen; x = y = 0; }
    /* One task timer drives animation and survives HWND destruction for
     * cleanup retries. Reserve it before allocating any canvas resources. */
    timer = SetTimer(NULL, 0, 100, NULL);
    if (!timer) return 1;
    window = CreateWindow("LtwCanvas30", "Bounded mono canvas",
                          WS_OVERLAPPEDWINDOW, x, y, width, height,
                          NULL, NULL, instance, NULL);
    if (!window) { KillTimer(NULL, timer); return 1; }
    ShowWindow(window, show);
    UpdateWindow(window);
    while (GetMessage(&message, NULL, 0, 0)) {
        if (!message.hwnd && message.message == WM_TIMER &&
            message.wParam == timer) {
            if (windowDestroyed) {
                if (LtwCanvasDestroy(&canvas)) PostQuitMessage(0);
            } else SendMessage(window, WM_TIMER, 1, 0L);
        } else {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }
    }
    KillTimer(NULL, timer);
    return (int)message.wParam;
}
