/* Foreground admission fixture only: no native Windows/timer evidence. */
#ifndef MCI_TEST_WINDOWS_H
#define MCI_TEST_WINDOWS_H
#define FAR
#define PASCAL
#define _loadds
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;
typedef unsigned int HWND;
typedef unsigned int HTASK;
typedef unsigned int HMODULE;
typedef int HFILE;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef struct {
 unsigned style; LONG (*lpfnWndProc)(HWND,WORD,WORD,LONG);
 int cbClsExtra,cbWndExtra; HMODULE hInstance;
 unsigned hIcon,hCursor,hbrBackground;
 LPCSTR lpszMenuName,lpszClassName;
} WNDCLASS;
#define WM_TIMER 0x113
#define WM_CLOSE 0x10
#define WM_ENDSESSION 0x16
#define OF_READ 0
#define OF_SHARE_DENY_WRITE 0x20
#define GetCurrentTask() 1U
#define IsWindow(w) ((w)!=0)
#define GetWindowTask(w) 1U
#define PostMessage(w,m,a,b) 1
#define GetTickCount() 0U
#define DefWindowProc(w,m,a,b) 0L
#define GetModuleHandle(n) 32U
#define GetModuleFileName(h,n,s) 1U
#define LoadLibrary(n) 32U
#define FreeLibrary(h) 1U
#define RegisterClass(c) 1U
#define UnregisterClass(c,h) 1U
#define CreateWindow(c,n,s,x,y,w,h,p,m,i,a) 1U
#define SetTimer(w,i,t,p) 1U
#define DestroyWindow(w) 1U
#define KillTimer(w,i) 1U
HFILE _lopen(LPCSTR,unsigned);
LONG _llseek(HFILE,LONG,int);
WORD _lread(HFILE,void *,WORD);
int _lclose(HFILE);
#endif
