
#ifndef _TWAININTEROP_H
#define _TWAININTEROP_H


#include "twain2.3.h"

#ifdef TWH_CMP_MSC
#define LOADLIBRARY(lib) LoadLibrary(lib) 
#define LOADFUNCTION(lib, func) GetProcAddress(lib, func)
#define UNLOADLIBRARY(lib) FreeLibrary(lib)

#elif  TWH_CMP_GNU
#define LOADLIBRARY(lib) dlopen(lib, RTLD_NOW)
#define LOADFUNCTION(lib, func) dlsym(lib, func)
#define UNLOADLIBRARY(lib) dlclose(lib)
typedef void * HMODULE;

#if !defined(TRUE)
#define FALSE		0
#define TRUE		1
#endif

#else
#error Sorry, we don't recognize this system...
#endif


#ifndef _WINUSER_
struct tagMSG; // Forward or never
typedef tagMSG MSG;
#endif

#endif //_TWAININTEROP_H