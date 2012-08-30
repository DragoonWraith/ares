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
#include "celtool/initapp.h"

/* Fun fact: should occur after csutil/event.h, otherwise, gcc may report
 * missing csMouseEventHelper symbols. */
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

#ifdef CS_STATIC_LINKED
#include "common_static/link_static_plugins.h"

ARES_LINK_COMMON_STATIC_PLUGIN
#endif

CS_IMPLEMENT_APPLICATION

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
  AppPump() { };
  virtual ~AppPump () { }
  virtual void Notify()
  {
    wxWakeUpIdle ();
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

extern AppAresEditWX* aresed;

bool MyApp::OnInit (void)
{
  wxInitAllImageHandlers ();

  //AppAresEditWX* app = new AppAresEditWX ();

#if defined(wxUSE_UNICODE) && wxUSE_UNICODE
  char** csargv;
  csargv = (char**)cs_malloc(sizeof(char*) * argc);
  for(int i = 0; i < argc; i++) 
  {
    csargv[i] = strdup (wxString(argv[i]).mb_str().data());
  }
  //if (!aresed->AresInitialize (argc, csargv)) return false;
  object_reg = csInitializer::CreateEnvironment (argc, csargv);
#else
  //if (!aresed->AresInitialize (argc, argv)) return false;
  object_reg = csInitializer::CreateEnvironment (argc, argv);
#endif
  //if (!aresed->Application ()) return false;

  if (!celInitializer::SetupConfigManager (object_reg,
      "/appdata/AppAresEdit.cfg", "ares"))
  {
    printf ("Failed to setup config manager!\n");
    fflush (stdout);
    return false;
  }

  csConfigAccess config (object_reg);
  int fbWidth = config->GetInt ("Video.ScreenWidth", 1000);
  int fbHeight = config->GetInt ("Video.ScreenHeight", 600);

  csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser> (object_reg);
  if (cmdline->GetOption ("mode"))
  {
    sscanf (cmdline->GetOption ("mode"), "%dx%d", &fbWidth, &fbHeight);
  }

  aresed = new AppAresEditWX (object_reg, fbWidth, fbHeight);
  aresed->Initialize ();

  AppPump* p = new AppPump ();
  p->Start (5);

  return true;
}

int MyApp::OnExit ()
{
  csInitializer::DestroyApplication (object_reg);
  return 0;
}

