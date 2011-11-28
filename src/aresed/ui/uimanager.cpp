/*
The MIT License

Copyright (c) 2011 by Jorrit Tyberghein

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

#include <crystalspace.h>
#include "../apparesed.h"
#include "uimanager.h"
#include "filereq.h"
#include "newproject.h"
#include "celldialog.h"
#include "pcdialog.h"

/* Fun fact: should occur after csutil/event.h, otherwise, gcc may report
 * missing csMouseEventHelper symbols. */
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

UIManager::UIManager (AppAresEditWX* app, wxWindow* parent) :
  app (app), parent (parent)
{
  filereqDialog = new FileReq (parent, app->GetVFS (), "/saves");
  newprojectDialog = new NewProjectDialog (parent, this, app->GetVFS ());
  cellDialog = new CellDialog (parent, this);
  pcDialog = new PropertyClassDialog (parent, this);
  contextMenuID = ID_FirstContextItem;
}

UIManager::~UIManager ()
{
  delete filereqDialog;
  delete newprojectDialog;
  delete cellDialog;
  delete pcDialog;
}

void UIManager::Message (const char* description, ...)
{
  va_list args;
  va_start (args, description);
  csString msg;
  msg.FormatV (description, args);
  wxMessageBox (wxString::FromUTF8 (msg), wxT("Message"),
      wxICON_INFORMATION, parent);
  va_end (args);
}

void UIManager::Error (const char* description, ...)
{
  va_list args;
  va_start (args, description);
  csString msg;
  msg.FormatV (description, args);
  wxMessageBox (wxString::FromUTF8 (msg), wxT("Error!"),
      wxICON_ERROR, parent);
  va_end (args);
}

