#ifndef CANVAS_EXAMPLE_TEST_WINDOWS_H
#define CANVAS_EXAMPLE_TEST_WINDOWS_H
#include "../windows.h"
#undef NULL
#define NULL 0
#define PASCAL
typedef unsigned short HWND;
typedef unsigned short HINSTANCE;
typedef unsigned short HICON;
typedef unsigned short HCURSOR;
typedef unsigned int UINT;
typedef unsigned short WPARAM;
typedef long LPARAM;
typedef struct { RECT rcPaint; } PAINTSTRUCT;
typedef struct { HWND hwnd; UINT message; WPARAM wParam; } MSG;
typedef struct {
    UINT style;
    long (*lpfnWndProc)(HWND, UINT, WPARAM, LPARAM);
    int cbClsExtra, cbWndExtra;
    HINSTANCE hInstance;
    HICON hIcon;
    HCURSOR hCursor;
    HGDIOBJ hbrBackground;
    const char *lpszMenuName, *lpszClassName;
} WNDCLASS;
#define WM_CREATE 1
#define WM_SIZE 5
#define WM_TIMER 275
#define WM_ERASEBKGND 20
#define WM_PAINT 15
#define WM_DESTROY 2
#define SIZEICONIC 1
#define WHITENESS ((DWORD)0x00ff0062UL)
#define BLACK_BRUSH 4
#define CS_HREDRAW 2
#define CS_VREDRAW 1
#define IDI_APPLICATION ((const char *)32512)
#define IDC_ARROW ((const char *)32512)
#define WS_OVERLAPPEDWINDOW 0x00cf0000UL
#define CW_USEDEFAULT (-32768)
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
int GetSystemMetrics(int index);
void GetClientRect(HWND window, RECT *client);
HDC GetDC(HWND window);
void ReleaseDC(HWND window, HDC dc);
UINT SetTimer(HWND window, UINT id, UINT milliseconds, void *callback);
void KillTimer(HWND window, UINT id);
void SetWindowText(HWND window, const char *text);
BOOL InvalidateRect(HWND window, const RECT *r, BOOL erase);
BOOL IsIconic(HWND window);
HDC BeginPaint(HWND window, PAINTSTRUCT *paint);
void EndPaint(HWND window, const PAINTSTRUCT *paint);
HGDIOBJ GetStockObject(int object);
int FillRect(HDC dc, const RECT *r, HGDIOBJ brush);
void PostQuitMessage(int code);
long DefWindowProc(HWND window, UINT message, WPARAM wp, LPARAM lp);
HICON LoadIcon(HINSTANCE instance, const char *name);
HCURSOR LoadCursor(HINSTANCE instance, const char *name);
BOOL RegisterClass(const WNDCLASS *wc);
HWND CreateWindow(const char *class_name, const char *title, DWORD style,
                  int x, int y, int width, int height, HWND parent,
                  unsigned short menu, HINSTANCE instance, void *parameter);
void ShowWindow(HWND window, int show);
void UpdateWindow(HWND window);
BOOL GetMessage(MSG *message, HWND window, UINT first, UINT last);
void TranslateMessage(const MSG *message);
void DispatchMessage(const MSG *message);
long SendMessage(HWND window, UINT message, WPARAM wp, LPARAM lp);
#endif
