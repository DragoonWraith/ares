/*
The MIT License

Copyright (c) 2012 by Frank Richter

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

#include "cssysdef.h"

#include "common_static/link_static_plugins.h"

#ifdef ARES_BUILD_SHARED_LIBS
CS_DECLARE_DEFAULT_STATIC_VARIABLE_REGISTRATION
CS_DEFINE_STATIC_VARIABLE_REGISTRATION (csStaticVarCleanup_csutil);
#endif

struct _common_static_static_use_CRYSTAL { _common_static_static_use_CRYSTAL (); };
struct _common_static_static_use_CEL { _common_static_static_use_CEL (); };
struct _common_static_static_use { _common_static_static_use (); };

void AresLinkCommonStaticPlugins ()
{
  /* Bind against symbols from generated static_use file to make
   * static plugins registration work; makes that the respective objects
   * are considered for static initialization. */
  _common_static_static_use_CRYSTAL ();
  _common_static_static_use_CEL ();
  _common_static_static_use ();
}
