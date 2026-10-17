#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FMTCORE.H"
#include "GUNZIP.H"
static unsigned char input[60000],plain[60000];static unsigned long size;static FXCORE core;static FXBURST b;
static int failures,checks;
#define C(x) do{checks++;if(!(x)){printf("FAIL line%d: %s\n",__LINE__,#x);failures++;}}while(0)
static int readat(void *p,unsigned long at,unsigned char *v,unsigned short n){unsigned char *s=p?p:input;if(at>size||n>size-at)return 0;memcpy(v,s+at,n);return 1;}
static void load(char *p){FILE *f=fopen(p,"rb");if(!f){perror(p);exit(2);}size=fread(input,1,sizeof(input),f);fclose(f);}
int main(void){unsigned short n;int r;unsigned long count,last;char *names[]={"LONG.VGZ","FIXED.VGZ","STORE.VGZ"};unsigned i;
 for(i=0;i<3;i++){load(names[i]);C(gz_decode(readat,0,size,plain,60000,&n)==0);C(n>1000);size=n;C(FXPreflight(&core,readat,plain,size,FX_VGM)==0);C(core.groups==1200);}
 load("BAD.VGZ");C(gz_decode(readat,0,size,plain,60000,&n)==4);
 load("SHORT.VGZ");C(gz_decode(readat,0,size,plain,10,&n)==2);
 load("SHORT.VGZ");size--;C(gz_decode(readat,0,size,plain,60000,&n)!=0);
 load("SHORT.VGM");C(FXPreflight(&core,readat,0,size,FX_VGM)==0);C(core.nextAt==1193181UL);C(FXOpen(&core,readat,0,size,FX_VGM)==0);
 count=last=0;while((r=FXNext(&core,&b))>0){if(count)C(b.at-last>=FX_MIN);last=b.at;count++;}C(r==0);C(count==61);C(last==1193181UL);
 load("LOOP.VGM");C(FXPreflight(&core,readat,0,size,FX_VGM)==0);C(core.looped==1);C(core.nextAt==1193181UL);C(core.groups==60);
 load("FAST.VGM");C(FXPreflight(&core,readat,0,size,FX_VGM)<0);
 load("SHORT.VGM");input[40]=9;input[42]=16;C(FXPreflight(&core,readat,0,size,FX_VGM)<0);C(core.error==2);
 load("THREE.MOD");C(FXPreflight(&core,readat,0,size,FX_MOD)==0);C(core.groups==48);C(core.nextAt>1145000UL&&core.nextAt<1145500UL);
 C(FXOpen(&core,readat,0,size,FX_MOD)==0);C(FXNext(&core,&b)==1);C(b.count==9);C(b.data[0]==(0x80|(432&15)));
 for(i=0;i<b.count;i++)C((b.data[i]&0xf0)!=0xe0); /* no invented noise part */
 load("MAX.VGM");C(FXPreflight(&core,readat,0,size,FX_VGM)==0);C(FXOpen(&core,readat,0,size,FX_VGM)==0);C(FXNext(&core,&b)==1);C(b.count==16);
 input[96]=0x50;input[97]=0xe7;input[98]=0x62;input[99]=0x66;size=100;input[4]=96;C(FXPreflight(&core,readat,0,size,FX_VGM)<0);
 core.error=0;core.done=1;core.terminal=0;core.groups=1;core.lastAt=0;core.nextAt=2147483648UL;C(FXNext(&core,&b)<0);C(core.error==6);
 load("FOUR.MOD");C(FXPreflight(&core,readat,0,size,FX_MOD)<0);C(core.modError.code==MOD_ERR_VOICE_LIMIT);
 load("BAD.MOD");C(FXPreflight(&core,readat,0,size,FX_MOD)<0);C(core.modError.code==MOD_ERR_UNSUPPORTED_EFFECT);
 printf("format producer: %d checks, %d failures\n",checks,failures);return failures!=0;
}
