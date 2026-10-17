#ifndef LTWUI_TEST_WINDOWS_H
#define LTWUI_TEST_WINDOWS_H
#include <stddef.h>
/* Minimal host-only USER declarations for exercising the real LTWUI.C. */
typedef struct MockWindow *HWND;
typedef int BOOL;
typedef const char *LPCSTR;
#define TRUE 1
#define FALSE 0
#define WM_SETREDRAW 0x000B
long SendMessage(HWND window,unsigned message,unsigned long wParam,long lParam);
int IsWindow(HWND window);
int InvalidateRect(HWND window,void *rect,int erase);
int GetWindowTextLength(HWND window);
int GetWindowText(HWND window,char *text,int capacity);
void SetWindowText(HWND window,LPCSTR text);
int lstrcmp(LPCSTR left,LPCSTR right);
#endif
