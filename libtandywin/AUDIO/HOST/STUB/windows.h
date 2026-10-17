#ifndef FAKE_WINDOWS_H
#define FAKE_WINDOWS_H
#include <stdint.h>
#include <string.h>
#define FAR
#define PASCAL
#define _loadds
#define _cdecl
#define WF_PMODE 1
typedef uint16_t WORD;typedef uint32_t DWORD;typedef int32_t LONG;typedef uint8_t BYTE;
typedef uintptr_t HMODULE;typedef uintptr_t HTASK;typedef char *LPSTR;typedef void (*FARPROC)(void);
#define HIWORD(v) ((WORD)(((DWORD)(v))>>16))
HTASK GetCurrentTask(void);HMODULE GetModuleHandle(const char *);WORD GetModuleFileName(HMODULE,char *,WORD);HMODULE LoadLibrary(const char *);WORD FreeLibrary(HMODULE);FARPROC GetProcAddress(HMODULE,LPSTR);
WORD GetWinFlags(void);int OpenSound(void);void CloseSound(void);void StopSound(void);void Yield(void);
#endif
