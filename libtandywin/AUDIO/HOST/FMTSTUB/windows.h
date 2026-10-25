#ifndef FMTSTUB_WINDOWS_H
#define FMTSTUB_WINDOWS_H
/* Host stand-in for the few Win16 calls FMTAPP.C makes; see TEST_FMTAPP.C. */
#include <stdint.h>
#include <string.h>
#define FAR
#define PASCAL
#define _loadds
#define WINVER 0x0300
typedef uint16_t WORD;typedef unsigned long DWORD;typedef long LONG;typedef uint8_t BYTE; /* Win16 long is 32 bits; so is Windows-host long */
typedef int HFILE;typedef uintptr_t HMODULE;typedef uintptr_t HTASK;typedef uintptr_t HGLOBAL;
typedef char *LPSTR;typedef void *LPVOID;
typedef struct {char name[260];} OFSTRUCT;
#define HFILE_ERROR (-1)
#define OF_READ 0
#define OF_READWRITE 2
#define OF_SHARE_DENY_WRITE 0x20
#define OF_DELETE 0x200
#define GMEM_MOVEABLE 2
#define lstrcpy strcpy
#define lstrcat strcat
HFILE _lopen(const char *,int);HFILE _lcreat(const char *,int);LONG _llseek(HFILE,LONG,int);
WORD _lread(HFILE,void *,WORD);WORD _lwrite(HFILE,const void *,WORD);HFILE _lclose(HFILE);
HFILE OpenFile(const char *,OFSTRUCT *,WORD);int GetTempFileName(BYTE,const char *,WORD,char *);
HGLOBAL GlobalAlloc(WORD,DWORD);void *GlobalLock(HGLOBAL);int GlobalUnlock(HGLOBAL);HGLOBAL GlobalFree(HGLOBAL);
HTASK GetCurrentTask(void);HMODULE LoadLibrary(const char *);void FreeLibrary(HMODULE);void *GetProcAddress(HMODULE,LPSTR);
#endif
