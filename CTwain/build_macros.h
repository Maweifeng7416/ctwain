// The MIT License (MIT)
// Copyright (c) 2015 Yin-Chun Wang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished 
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
// OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef BUILD_MACROS_H_
#define BUILD_MACROS_H_


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

#endif //BUILD_MACROS_H_