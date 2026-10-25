/* gz_decode_stream (32 KiB ring + sink) against gz_decode and plain bytes.
   Usage: test_gzring REF.BIN GZ... (each GZ must inflate to REF.BIN). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GUNZIP.H"
#include "GZVECT.H"
static int failures,checks;
#define C(x) do{checks++;if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);failures++;}}while(0)
static unsigned char *in;static unsigned long insize;
static int rd(void *u,GZ32 at,GZ8 *o,GZ16 n){(void)u;if(at>insize||n>insize-at)return 0;memcpy(o,in+at,n);return 1;}
static unsigned char *out;static unsigned long outlen,outmax,failAfter;static unsigned chunks,maxchunk;
static int sink(void *u,const GZ8 *p,GZ16 n){(void)u;if(failAfter&&outlen+n>failAfter)return 0;
 if(outlen+n>outmax)return 0;memcpy(out+outlen,p,n);outlen+=n;chunks++;if(n>maxchunk)maxchunk=n;return 1;}
static GZ8 ring[GZ_WINDOW];
static int stream(unsigned long limit,GZ32 *total){outlen=chunks=maxchunk=0;return gz_decode_stream(rd,0,(GZ32)insize,ring,sink,0,limit,total);}
static unsigned char *slurp(const char *p,unsigned long *n){FILE *f=fopen(p,"rb");unsigned char *b;if(!f){perror(p);exit(2);}
 fseek(f,0,SEEK_END);*n=ftell(f);fseek(f,0,SEEK_SET);b=malloc(*n+16);fread(b,1,*n,f);fclose(f);return b;}
int main(int argc,char **argv){int k;unsigned i;GZ32 total;int r;unsigned char *ref;unsigned long refn;
 out=malloc(1UL<<24);outmax=1UL<<24;
 /* Vectors: ring mode (limit 8) must give the linear decoder's verdict. */
 for(i=0;i<GZV_COUNT;i++){const GZVCASE *v=&gzv_cases[i];static unsigned char d[2048];memcpy(d,v->data,v->size);in=d;insize=v->size;
  r=stream(8,&total);C(r==v->error);if(!r){C(total==v->used);C(outlen==v->used);C(!memcmp(out,v->plain,v->used));}
  r=stream(60000UL,&total);{GZ16 n;static GZ8 lin[60000U];int lr=gz_decode(rd,0,(GZ32)insize,lin,60000,&n);
   C(r==lr);if(!r){C(total==n&&outlen==n&&!memcmp(out,lin,n));}}}
 if(argc<3){printf("vectors only: checks=%d failures=%d\n",checks,failures);return failures!=0;}
 ref=slurp(argv[1],&refn);
 for(k=2;k<argc;k++){in=slurp(argv[k],&insize);
  r=stream(1UL<<24,&total);
  printf("%s: %lu -> %lu bytes, %u sink calls (max %u), ring %u\n",argv[k],insize,(unsigned long)total,chunks,maxchunk,GZ_WINDOW);
  C(r==0);C(total==refn&&outlen==refn&&!memcmp(out,ref,refn));C(maxchunk<=GZ_CHUNK);
  if(k==argc-1){unsigned char *bad=malloc(insize+16);unsigned long sz=insize;
   /* Corruption: every one must be refused. */
   memcpy(bad,in,sz);in=bad;insize=sz-1;C(stream(1UL<<24,&total)!=0);          /* truncated 1 */
   insize=sz-9;C(stream(1UL<<24,&total)!=0);                                      /* truncated into trailer */
   insize=sz/2;C(stream(1UL<<24,&total)!=0);                                      /* truncated mid-stream */
   bad[sz-8]^=1;insize=sz;r=stream(1UL<<24,&total);C(r==4);bad[sz-8]^=1;         /* CRC */
   bad[sz-4]^=1;r=stream(1UL<<24,&total);C(r==4);bad[sz-4]^=1;                    /* ISIZE */
   bad[sz]=0;insize=sz+1;C(stream(1UL<<24,&total)!=0);insize=sz;                  /* trailing byte */
   bad[sz/2]^=0x55;r=stream(1UL<<24,&total);C(r!=0);bad[sz/2]^=0x55;              /* flipped data */
   r=stream(1000UL,&total);C(r==2);                                                /* output cap */
   failAfter=9000;r=stream(1UL<<24,&total);C(r==5);failAfter=0;                    /* sink (disk) failure */
   r=stream(1UL<<24,&total);C(r==0&&outlen==refn);                                 /* intact again */
  }
  free(in);}
 printf("%s checks=%d failures=%d\n",failures?"FAIL":"PASS",checks,failures);return failures!=0;}
