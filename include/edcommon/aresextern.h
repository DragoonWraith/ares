/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

#ifndef __ARESEXTERN_H__
#define __ARESEXTERN_H__

#include "aresconfig.h"
#include "celplatform.h"

#if defined(CS_PLATFORM_WIN32)
  #if defined(ARES_BUILD_SHARED_LIBS)
    #define ARES_EXPORT_SYM CS_EXPORT_SYM_DLL
    #define ARES_IMPORT_SYM CS_IMPORT_SYM_DLL
  #else
    #define ARES_EXPORT_SYM
    #define ARES_IMPORT_SYM
  #endif // ARES_BUILD_SHARED_LIBS
#else
  #if defined(ARES_BUILD_SHARED_LIBS)
    #define ARES_EXPORT_SYM CS_VISIBILITY_DEFAULT
  #else
    #define ARES_EXPORT_SYM
  #endif
  #define ARES_IMPORT_SYM
#endif

#ifdef ARES_EDCOMMON_LIB
  #define ARES_EDCOMMON_EXPORT ARES_EXPORT_SYM
#else
  #define ARES_EDCOMMON_EXPORT ARES_IMPORT_SYM
#endif

#endif // __ARESEXTERN_H__

