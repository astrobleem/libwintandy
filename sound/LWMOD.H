/* App-local providers have shared Win16 module names. An absolute LoadLibrary
   path can still return an already-loaded module from another installation. */
#ifndef LW_PROVIDER_PATH_H
#define LW_PROVIDER_PATH_H
static WORD LWModuleMatches(HMODULE module,LPCSTR wanted)
{
    static char actual[144];
    WORD n;
    n=GetModuleFileName(module,actual,sizeof(actual));
    return n && n<sizeof(actual) && !lstrcmpi(actual,wanted);
}
#endif
