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

#ifndef __appares_helpframe_h
#define __appares_helpframe_h

#include <crystalspace.h>

#include "editor/iplugin.h"
#include "editor/iapp.h"
#include "editor/icommand.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

/**
 * The frame for viewing online help.
 */
class HelpFrame : public wxFrame,
  public scfImplementation3<HelpFrame, iEditorPlugin, iComponent,
	iCommandHandler>
{
private:
  iAresEditor* app;
  iObjectRegistry* object_reg;
  static csStringID ID_Show;
  static csStringID ID_About;

  void OnClose (wxCloseEvent& event);

public:
  HelpFrame (iBase* parent);
  virtual ~HelpFrame ();

  virtual bool Initialize (iObjectRegistry* object_reg);

  virtual void SetApplication (iAresEditor* app);
  virtual void SetParent (wxWindow* parent);
  virtual const char* GetPluginName () const { return "Help"; }
  virtual bool Command (csStringID id, const csString& args)
  {
    if (id == ID_Show) { Show (); return true; }
    if (id == ID_About) { About (); return true; }
    return false;
  }
  virtual bool IsCommandValid (csStringID id, const csString& args,
      iSelection* selection, size_t pastesize)
  {
    return true;
  }
  virtual csPtr<iString> GetAlternativeLabel (csStringID id,
      iSelection* selection, size_t pastesize)
  {
    return 0;
  }

  iObjectRegistry* GetObjectRegistry () const { return object_reg; }
  iAresEditor* GetApplication () const { return app; }
  iUIManager* GetUIManager () const { return app->GetUI (); }

  void Show ();
  void About ();

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_helpframe_h

