#define FULL_ONLY
#define WINVER 0x0300
#include <windows.h>
#include "EXPECT.H"
static HFILE log;static WORD checks,failures,token;static char b[220],name[144];
static void check(char *n,DWORD got,DWORD want){checks++;if(got!=want)failures++;wsprintf(b,"CHECK %s got=%lu want=%lu\r\n",(LPSTR)n,got,want);_lwrite(log,b,lstrlen(b));_lclose(log);log=_lopen("C:\\FMTGUI.LOG",OF_WRITE);_llseek(log,0L,2);}
static void waitms(WORD ms){DWORD t=GetTickCount();MSG m;while(GetTickCount()-t<ms){while(PeekMessage(&m,0,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessage(&m);}Yield();}}
static void ack(HWND w){DWORD t=GetTickCount();++token;PostMessage(w,WM_USER+45,token,0L);while((WORD)SendMessage(w,WM_USER+46,0,0L)!=token&&GetTickCount()-t<18000UL)waitms(55);check("queued_ack",SendMessage(w,WM_USER+46,0,0L),token);}
static void button(HWND w,WORD id){HWND x=GetDlgItem(w,id);PostMessage(x,WM_LBUTTONDOWN,MK_LBUTTON,MAKELONG(8,8));PostMessage(x,WM_LBUTTONUP,0,MAKELONG(8,8));ack(w);}
static void path(HWND w,LPSTR p){PostMessage(GetDlgItem(w,103),WM_SETTEXT,0,(LONG)p);ack(w);button(w,101);}
static void state(HWND w,LPSTR p){GetWindowText(GetDlgItem(w,104),name,sizeof(name));wsprintf(b,"STATE actual=%s expected=%s\r\n",(LPSTR)name,p);_lwrite(log,b,lstrlen(b));check("state",!lstrcmp(name,p),1);}
static WORD dbg(HWND w,WORD i){return (WORD)SendMessage(w,WM_USER+50,i,0L);}
static void eof(HWND w){DWORD t=GetTickCount();while(SendMessage(w,WM_USER+47,0,0L)&&GetTickCount()-t<30000UL)waitms(55);ack(w);state(w,"Done");check("EOF_sound_free",OpenSound(),3);CloseSound();check("EOF_error",dbg(w,6),0);}
int PASCAL WinMain(HINSTANCE i,HINSTANCE p,LPSTR c,int show)
{
 HWND w;WORD child,before,clockBefore;DWORD freeBefore,freeAfter;
 log=_lcreat("C:\\FMTGUI.LOG",0);child=WinExec("C:\\MMLAB\\MINIPLAY.EXE",SW_SHOWNORMAL);check("launch",child>=32,1);waitms(800);w=FindWindow("MiniMCIPlayer",0);check("window",w&&IsWindowVisible(w),1);if(!w)goto done;
 freeBefore=GetFreeSpace(0);
#ifdef FULL_ONLY
 path(w,"C:\\MMLAB\\LONG.VGM");state(w,"VGM PSG");
 MoveWindow(w,0,0,156,192,TRUE);waitms(100);MoveWindow(w,0,0,292,192,TRUE);waitms(100);
 eof(w);check("long_VGM_hash",dbg(w,30),LONG_HASH);check("long_VGM_writes",dbg(w,31),LONG_WRITES);
 path(w,"C:\\MMLAB\\LONG.VGZ");state(w,"VGM PSG");eof(w);check("long_VGZ_hash",dbg(w,30),LONG_HASH);check("long_VGZ_writes",dbg(w,31),LONG_WRITES);
 path(w,"C:\\MMLAB\\LOOP.VGM");eof(w);check("loop_hash",dbg(w,30),LOOP_HASH);check("loop_writes",dbg(w,31),LOOP_WRITES);check("loop_noise",dbg(w,23),6);
 path(w,"C:\\MMLAB\\MAX.VGM");eof(w);check("maximum_burst_hash",dbg(w,30),MAX_HASH);check("maximum_burst_writes",dbg(w,31),16);
#else
#ifdef INTERACT_ONLY
 path(w,"C:\\MMLAB\\LONG.VGM");state(w,"VGM PSG");waitms(300);check("VGM_active",SendMessage(w,WM_USER+47,0,0L),1);check("burst_mode",dbg(w,22),1);check("exclusive",OpenSound()<0,1);CloseSound();
 check("raw_writes",dbg(w,31)>11,1);button(w,109);state(w,"Paused");check("paused",dbg(w,27),1);before=dbg(w,31);clockBefore=dbg(w,32);waitms(400);check("pause_writes_frozen",dbg(w,31),before);check("pause_clock_frozen",dbg(w,32),clockBefore);
 button(w,109);state(w,"VGM PSG");waitms(300);check("resumed",dbg(w,31)>before,1);check("resume_error",dbg(w,6),0);
 path(w,"C:\\MMLAB\\LONG.VGZ");state(w,"VGM PSG");waitms(300);check("VGZ_active",SendMessage(w,WM_USER+47,0,0L),1);button(w,102);state(w,"Stopped");check("stop_free",OpenSound(),3);CloseSound();
#else
 path(w,"C:\\MMLAB\\SHORT.VGM");eof(w);check("VGM_byte_hash",dbg(w,30),VGM_HASH);check("VGM_byte_count",dbg(w,31),VGM_WRITES);check("intentional_noise_writes",dbg(w,23),3);
 path(w,"C:\\MMLAB\\SHORT.VGZ");eof(w);check("VGZ_same_byte_hash",dbg(w,30),VGM_HASH);check("VGZ_same_byte_count",dbg(w,31),VGM_WRITES);
 path(w,"C:\\MMLAB\\THREE.MOD");state(w,"MOD PSG");eof(w);check("MOD_no_noise_retrigger",dbg(w,23),0);check("MOD_groups",dbg(w,29),48);
 path(w,"C:\\MMLAB\\FOUR.MOD");check("four_lane_refused",SendMessage(w,WM_USER+47,0,0L),0);check("four_lane_sound_free",OpenSound(),3);CloseSound();
 path(w,"C:\\MMLAB\\FAST.VGM");check("fast_refused",SendMessage(w,WM_USER+47,0,0L),0);check("fast_sound_free",OpenSound(),3);CloseSound();
 path(w,"C:\\MMLAB\\BAD.VGZ");check("bad_crc_refused",SendMessage(w,WM_USER+47,0,0L),0);check("bad_crc_sound_free",OpenSound(),3);CloseSound();
#endif
#endif
 wsprintf(b,"REFILL admission=%u minimum_sampled_queued=%u refused_at=%u:%u clock=%u:%u queued=%u source=%u groups=%u\r\n",dbg(w,33),dbg(w,34),dbg(w,36),dbg(w,35),dbg(w,38),dbg(w,37),dbg(w,39),dbg(w,40),dbg(w,41));_lwrite(log,b,lstrlen(b));
 freeAfter=GetFreeSpace(0);wsprintf(b,"MEMORY free_before=%lu free_after=%lu\r\n",freeBefore,freeAfter);_lwrite(log,b,lstrlen(b));check("no_heap_loss",freeAfter+4096UL>=freeBefore,1);
#ifdef INTERACT_ONLY
 path(w,"C:\\MMLAB\\LONG.VGM");waitms(200);
#endif
 PostMessage(w,WM_CLOSE,0,0L);waitms(600);check("close_destroyed",IsWindow(w),0);check("close_free",OpenSound(),3);CloseSound();
done:wsprintf(b,"FINAL failures=%u checks=%u\r\n",failures,checks);_lwrite(log,b,lstrlen(b));_lclose(log);ExitWindows(0L,0);return failures;
}
