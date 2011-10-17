/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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

#define CS_IMPLEMENT_PLATFORM_APPLICATION

#include "aresed.h"
#include "apparesed.h"
#include <csutil/sysfunc.h> // Provides csPrintf()

#include "csutil/event.h"

/* Fun fact: should occur after csutil/event.h, otherwise, gcc may report
 * missing csMouseEventHelper symbols. */
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

CS_IMPLEMENT_APPLICATION

#if 0
int main(int argc, char** argv)
{
  csPrintf ("ares version 0.1 by Jorrit Tyberghein.\n");

  /* Runs the application.  
   *
   * csApplicationRunner<> cares about creating an application instance 
   * which will perform initialization and event handling for the entire game. 
   *
   * The underlying csApplicationFramework also performs some core 
   * initialization.  It will set up the configuration manager, event queue, 
   * object registry, and much more.  The object registry is very important, 
   * and it is stored in your main application class (again, by 
   * csApplicationFramework). 
   */
  return csApplicationRunner<AppAresEdit>::Run (argc, argv);
}

#else

#if defined(CS_PLATFORM_WIN32)

#ifndef SW_SHOWNORMAL
#define SW_SHOWNORMAL 1
#endif

/*
  WX provides WinMain(), but not main(), which is required for console apps.
 */
int main (int argc, const char* const argv[])
{
  return WinMain (GetModuleHandle (0), 0, GetCommandLineA (), SW_SHOWNORMAL);
}

#endif


class AppPump : public wxTimer
{
public:
  AppAresEdit* s;
  AppPump() { };
  virtual ~AppPump () { }
  virtual void Notify()
  {
    s->PushFrame ();
  }
};

// Define a new application type
class MyApp: public wxApp
{
public:
  iObjectRegistry* object_reg;

  virtual bool OnInit (void);
  virtual int OnExit (void);
};

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit (void)
{
  wxInitAllImageHandlers ();

  AppAresEdit* app = new AppAresEdit ();

#if defined(wxUSE_UNICODE) && wxUSE_UNICODE
  char** csargv;
  csargv = (char**)cs_malloc(sizeof(char*) * argc);
  for(int i = 0; i < argc; i++) 
  {
    csargv[i] = strdup (wxString(argv[i]).mb_str().data());
  }
  if (!app->AresInitialize (argc, csargv)) return false;
#else
  if (!app->AresInitialize (argc, argv)) return false;
#endif
  if (!app->Application ()) return false;

  AppPump* p = new AppPump ();
  p->s = app;
  p->Start (20);

  return true;
}

int MyApp::OnExit ()
{
  csInitializer::DestroyApplication (object_reg);
  return 0;
}

#endif

