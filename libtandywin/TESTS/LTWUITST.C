#include <stdio.h>
#include <string.h>
#include "windows.h"
#include "../LWTANDY.H"
struct MockWindow { int alive; char text[512]; int setCalls; int getCalls; int redrawOff; int redrawOn; int invalidations; int eraseInvalidations; };
static int failures,checks;
#define CHECK(expr) do { checks++; if(!(expr)) { \
 fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#expr); \
 failures++; } } while(0)
long SendMessage(HWND window,unsigned message,unsigned long wParam,long lParam) {
 (void)lParam;
 if(message==WM_SETREDRAW) { if(wParam)window->redrawOn++; else window->redrawOff++; }
 return 0L;
}
int IsWindow(HWND window) { return window&&window->alive; }
int InvalidateRect(HWND window,void *rect,int erase) {
 (void)rect; window->invalidations++; if(erase)window->eraseInvalidations++; return TRUE;
}
int GetWindowTextLength(HWND window) { return (int)strlen(window->text); }
int GetWindowText(HWND window,char *text,int capacity) {
 size_t length=strlen(window->text); window->getCalls++;
 if(length>=(size_t)capacity)length=(size_t)capacity-1;
 memcpy(text,window->text,length); text[length]='\0'; return (int)length;
}
void SetWindowText(HWND window,LPCSTR text) {
 window->setCalls++; strncpy(window->text,text,sizeof(window->text)-1);
 window->text[sizeof(window->text)-1]='\0';
}
int lstrcmp(LPCSTR left,LPCSTR right) { return strcmp(left,right); }
static void resetWindow(struct MockWindow *window,const char *text) {
 memset(window,0,sizeof(*window)); window->alive=1;
 if(text)strncpy(window->text,text,sizeof(window->text)-1);
}
static void testSetText(void) {
 struct MockWindow window; char longText[180]; int i;
 resetWindow(&window,"same");
 CHECK(LtwSetText(&window,"same")==FALSE);
 CHECK(window.getCalls==1&&window.setCalls==0);
 CHECK(LtwSetText(&window,"changed")==TRUE);
 CHECK(window.setCalls==1&&strcmp(window.text,"changed")==0);
 CHECK(LtwSetText(NULL,"ignored")==FALSE);
 for(i=0;i<(int)sizeof(longText)-1;i++)longText[i]='x';
 longText[sizeof(longText)-1]='\0'; strcpy(window.text,longText); window.getCalls=0; window.setCalls=0;
 CHECK(LtwSetText(&window,longText)==TRUE);
 CHECK(window.getCalls==0&&window.setCalls==1);
 CHECK(strlen(window.text)==sizeof(longText)-1);
}
static void testTextBoundaryAndExternalChange(void) {
 struct MockWindow window; char text[145]; int i;
 for(i=0;i<144;i++)text[i]='b';
 text[143]='\0'; resetWindow(&window,text);
 CHECK(LtwSetText(&window,text)==FALSE);
 CHECK(window.getCalls==1&&window.setCalls==0);
 text[143]='b'; text[144]='\0'; resetWindow(&window,text);
 CHECK(LtwSetText(&window,text)==TRUE);
 CHECK(window.getCalls==0&&window.setCalls==1);
 strcpy(window.text,"external USER change");
 CHECK(LtwSetText(&window,"restored")==TRUE);
 CHECK(window.setCalls==2&&strcmp(window.text,"restored")==0);
}
static void testNestedBatch(void) {
 struct MockWindow window; LTWLISTPAINT state={0,0}; resetWindow(&window,"");
 LtwListBegin(&state,&window);
 CHECK(window.redrawOff==1&&window.redrawOn==0&&window.invalidations==0);
 LtwListBegin(&state,&window); CHECK(window.redrawOff==1);
 LtwListEnd(&state,&window); CHECK(window.redrawOn==0&&window.invalidations==0);
 LtwListEnd(&state,&window);
 CHECK(window.redrawOn==1&&window.invalidations==1&&window.eraseInvalidations==1);
 CHECK(state.depth==0&&state.list==NULL);
 LtwListEnd(&state,&window);
 CHECK(window.redrawOn==1&&window.invalidations==1);
}
static void testDestroyedOrReplacedHandle(void) {
 struct MockWindow original,replacement; LTWLISTPAINT state={0,0};
 resetWindow(&original,""); resetWindow(&replacement,"");
 LtwListBegin(&state,&original); original.alive=0; LtwListEnd(&state,&original);
 CHECK(original.redrawOn==0&&original.invalidations==0);
 LtwListBegin(&state,&original); LtwListEnd(&state,&replacement);
 CHECK(original.redrawOn==0&&original.invalidations==0);
 CHECK(replacement.redrawOn==0&&replacement.invalidations==0);
 CHECK(state.depth==0&&state.list==NULL);
}
int main(void) {
 testSetText(); testTextBoundaryAndExternalChange(); testNestedBatch(); testDestroyedOrReplacedHandle();
 if(failures) { fprintf(stderr,"%d check(s) failed\n",failures); return 1; }
 printf("LTWUI host stub checks passed (%d checks)\n",checks); return 0;
}
