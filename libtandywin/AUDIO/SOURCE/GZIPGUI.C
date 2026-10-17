#define WINVER 0x0300
#include <windows.h>
#include "GUNZIP.H"
#include "GZVECT.H"
#include "EXPECT.H"
static HFILE log;static WORD checks,failures,token;static char b[220],name[144];
static const unsigned char *vector;static WORD vectorSize;static BYTE plain[9];
static int readVector(void *u,unsigned long at,unsigned char *out,unsigned short n){WORD i;(void)u;if(at>vectorSize||n>vectorSize-at)return 0;for(i=0;i<n;i++)out[i]=vector[(WORD)at+i];return 1;}
static void check(char *n,DWORD got,DWORD want){checks++;if(got!=want)failures++;wsprintf(b,"CHECK %s got=%lu want=%lu\r\n",(LPSTR)n,got,want);_lwrite(log,b,lstrlen(b));_lclose(log);log=_lopen("C:\\FMTGUI.LOG",OF_WRITE);_llseek(log,0L,2);}
static void waitms(WORD ms){DWORD t=GetTickCount();MSG m;while(GetTickCount()-t<ms){while(PeekMessage(&m,0,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessage(&m);}Yield();}}
static void ack(HWND w){DWORD t=GetTickCount();++token;PostMessage(w,WM_USER+45,token,0L);while((WORD)SendMessage(w,WM_USER+46,0,0L)!=token&&GetTickCount()-t<18000UL)waitms(55);check("queued_ack",SendMessage(w,WM_USER+46,0,0L),token);}
static void button(HWND w,WORD id){HWND x=GetDlgItem(w,id);PostMessage(x,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(8,8));PostMessage(x,WM_LBUTTONUP,0,MAKELONG(8,8));ack(w);}
static void path(HWND w,LPSTR p){PostMessage(GetDlgItem(w,103),WM_SETTEXT,0,(LONG)p);ack(w);button(w,101);}
static void state(HWND w,LPSTR p){GetWindowText(GetDlgItem(w,104),name,sizeof(name));wsprintf(b,"STATE actual=%s expected=%s\r\n",(LPSTR)name,p);_lwrite(log,b,lstrlen(b));check("state",!lstrcmp(name,p),1);}
static WORD dbg(HWND w,WORD i){return (WORD)SendMessage(w,WM_USER+50,i,0L);}
static void eof(HWND w){DWORD t=GetTickCount();while(SendMessage(w,WM_USER+47,0,0L)&&GetTickCount()-t<30000UL)waitms(55);ack(w);state(w,"Done");check("EOF_sound_free",OpenSound(),3);CloseSound();check("EOF_error",dbg(w,6),0);}
static void positive(HWND w,LPSTR p){path(w,p);state(w,"VGM PSG");eof(w);check("VGM_byte_hash",dbg(w,30),VGM_HASH);check("VGM_byte_count",dbg(w,31),VGM_WRITES);check("intentional_noise_writes",dbg(w,23),3);}
int PASCAL WinMain(HINSTANCE i,HINSTANCE p,LPSTR c,int show)
{
 HWND w;WORD child,j,k,n,r,same;const GZVCASE *v;DWORD freeBefore,freeAfter;
 log=_lcreat("C:\\FMTGUI.LOG",0);
 for(j=0;j<GZV_COUNT;j++){
  v=&gzv_cases[j];vector=v->data;vectorSize=v->size;for(k=0;k<9;k++)plain[k]=0xcc;plain[8]=0x5a;
  wsprintf(b,"VECTOR %s\r\n",(LPSTR)v->name);_lwrite(log,b,lstrlen(b));
  r=gz_decode(readVector,0,vectorSize,plain,8,&n);same=1;for(k=0;k<v->used;k++)if(plain[k]!=(BYTE)v->plain[k])same=0;
  check("native_gzip_result",r,v->error);check("native_gzip_used",n,v->used);check("native_gzip_plain",same,1);check("native_gzip_canary",plain[8],0x5a);check("native_gzip_emitted_A",plain[0],65);
 }
 child=WinExec("C:\\MMLAB\\MINIPLAY.EXE",SW_SHOWNORMAL);check("launch",child>=32,1);waitms(800);w=FindWindow("MiniMCIPlayer",0);check("window",w&&IsWindowVisible(w),1);if(!w)goto done;
 freeBefore=GetFreeSpace(0);
 positive(w,"C:\\MMLAB\\GZ31.VGZ");positive(w,"C:\\MMLAB\\GZ32.VGZ");positive(w,"C:\\MMLAB\\SHORT.VGZ");
 path(w,"C:\\MMLAB\\RS30.VGZ");check("reserved30_refused",SendMessage(w,WM_USER+47,0,0L),0);check("reserved30_sound_free",OpenSound(),3);CloseSound();
 path(w,"C:\\MMLAB\\RS31.VGZ");check("reserved31_refused",SendMessage(w,WM_USER+47,0,0L),0);check("reserved31_sound_free",OpenSound(),3);CloseSound();
 freeAfter=GetFreeSpace(0);wsprintf(b,"MEMORY free_before=%lu free_after=%lu\r\n",freeBefore,freeAfter);_lwrite(log,b,lstrlen(b));check("no_heap_loss",freeAfter+4096UL>=freeBefore,1);
 PostMessage(w,WM_CLOSE,0,0L);waitms(600);check("close_destroyed",IsWindow(w),0);check("close_free",OpenSound(),3);CloseSound();
done:wsprintf(b,"FINAL failures=%u checks=%u\r\n",failures,checks);_lwrite(log,b,lstrlen(b));_lclose(log);ExitWindows(0L,0);return failures;
}
