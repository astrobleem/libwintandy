#ifndef WTCANVAS_TEST_WINDOWS_H
#define WTCANVAS_TEST_WINDOWS_H
/* Host-only GDI model: Win16-sized RECT/handles, 32-bit DWORD. Not an SDK. */
#include <stddef.h>
typedef unsigned short HDC;
typedef unsigned short HBITMAP;
typedef unsigned short HGDIOBJ;
typedef unsigned int DWORD;
typedef int BOOL;
typedef char *LPSTR;
typedef struct { short left, top, right, bottom; } RECT;
#define FAR
#define TRUE 1
#define FALSE 0
#define BLACKNESS ((DWORD)0x00000042UL)
#define SRCCOPY ((DWORD)0x00cc0020UL)
#define RGB(r,g,b) ((DWORD)(r) | ((DWORD)(g) << 8) | ((DWORD)(b) << 16))
HDC CreateCompatibleDC(HDC reference);
HBITMAP CreateCompatibleBitmap(HDC reference, int width, int height);
HBITMAP CreateBitmap(int width, int height, unsigned planes,
                     unsigned bits, const void *data);
HGDIOBJ SelectObject(HDC dc, HGDIOBJ object);
BOOL DeleteDC(HDC dc);
BOOL DeleteObject(HGDIOBJ object);
DWORD SetTextColor(HDC dc, DWORD color);
DWORD SetBkColor(HDC dc, DWORD color);
BOOL PatBlt(HDC dc, int x, int y, int width, int height, DWORD rop);
BOOL BitBlt(HDC dest, int x, int y, int width, int height,
            HDC source, int sx, int sy, DWORD rop);
#endif
