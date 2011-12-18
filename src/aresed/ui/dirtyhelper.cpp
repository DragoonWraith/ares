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

#include "uitools.h"
#include "dirtyhelper.h"
#include "../models/rowmodel.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/choicebk.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

//-----------------------------------------------------------------------------

static csString GetNotebookPage (wxNotebook* book)
{
  csString value;
  int pageSel = book->GetSelection ();
  if (pageSel == wxNOT_FOUND) value = "";
  else
  {
    wxString pageTxt = book->GetPageText (pageSel);
    value = (const char*)pageTxt.mb_str (wxConvUTF8);
  }
  return value;
}

static csString GetChoicebookPage (wxChoicebook* book)
{
  csString value;
  int pageSel = book->GetSelection ();
  if (pageSel == wxNOT_FOUND) value = "";
  else
  {
    wxString pageTxt = book->GetPageText (pageSel);
    value = (const char*)pageTxt.mb_str (wxConvUTF8);
  }
  return value;
}

DirtyHelper::~DirtyHelper ()
{
  for (size_t i = 0 ; i < checkBoxes.GetSize () ; i++)
    checkBoxes[i]->Disconnect (wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
  for (size_t i = 0 ; i < textControls.GetSize () ; i++)
    textControls[i]->Disconnect (wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
  for (size_t i = 0 ; i < notebookControls.GetSize () ; i++)
    notebookControls[i]->Disconnect (wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
  for (size_t i = 0 ; i < choicebookControls.GetSize () ; i++)
    choicebookControls[i]->Disconnect (wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
}

void DirtyHelper::SetDirty (bool dirty)
{
  if (!dirty)
  {
    for (size_t i = 0 ; i < checkBoxes.GetSize () ; i++)
      checkBoxStates.Put (i, checkBoxes[i]->GetValue ());
    for (size_t i = 0 ; i < textControls.GetSize () ; i++)
      textControlStates.Put (i, (const char*)textControls[i]->GetValue ().mb_str (wxConvUTF8));
    for (size_t i = 0 ; i < notebookControls.GetSize () ; i++)
      notebookControlStates.Put (i, GetNotebookPage (notebookControls[i]));
    for (size_t i = 0 ; i < choicebookControls.GetSize () ; i++)
      choicebookControlStates.Put (i, GetChoicebookPage (choicebookControls[i]));
  }
  DirtyHelper::dirty = dirty;
}

bool DirtyHelper::CheckDirty ()
{
  for (size_t i = 0 ; i < checkBoxes.GetSize () ; i++)
    if (checkBoxStates[i] != checkBoxes[i]->GetValue ()) return true;
  for (size_t i = 0 ; i < textControls.GetSize () ; i++)
  {
    csString value = (const char*)textControls[i]->GetValue ().mb_str (wxConvUTF8);
    if (value != textControlStates[i])
      return true;
  }
  for (size_t i = 0 ; i < notebookControls.GetSize () ; i++)
  {
    csString value = GetNotebookPage (notebookControls[i]);
    if (value != notebookControlStates[i])
      return true;
  }
  for (size_t i = 0 ; i < choicebookControls.GetSize () ; i++)
  {
    csString value = GetChoicebookPage (choicebookControls[i]);
    if (value != choicebookControlStates[i])
      return true;
  }
  return false;
}

void DirtyHelper::OnComponentChanged (wxCommandEvent& event)
{
  bool newDirty = CheckDirty ();
  if (newDirty != dirty)
  {
    dirty = newDirty;
    for (size_t i = 0 ; i < listeners.GetSize () ; i++)
      listeners[i]->DirtyChanged (dirty);
  }
}

void DirtyHelper::AddDirtyListener (DirtyListener* listener)
{
  listeners.Push (listener);
}

void DirtyHelper::RemoveDirtyListener (DirtyListener* listener)
{
  listeners.Delete (listener);
}

bool DirtyHelper::RegisterComponent (wxWindow* parent, const char* name)
{
  wxString wxname = wxString::FromUTF8 (name);
  wxWindow* child = parent->FindWindow (wxname);
  if (!child)
  {
    printf ("Can't find component '%s'!\n", name);
    fflush (stdout);
    return false;
  }
  if (child->IsKindOf (CLASSINFO (wxTextCtrl)))
  {
    wxTextCtrl* text = wxStaticCast (child, wxTextCtrl);
    textControls.Push (text);
    text->Connect (wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
    textControlStates.Push ((const char*)text->GetValue ().mb_str (wxConvUTF8));
  }
  else if (child->IsKindOf (CLASSINFO (wxCheckBox)))
  {
    wxCheckBox* cb = wxStaticCast (child, wxCheckBox);
    checkBoxes.Push (cb);
    cb->Connect (wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
    checkBoxStates.Push (cb->GetValue ());
  }
  else if (child->IsKindOf (CLASSINFO (wxChoicebook)))
  {
    wxChoicebook* cb = wxStaticCast (child, wxChoicebook);
    choicebookControls.Push (cb);
    cb->Connect (wxEVT_COMMAND_CHOICEBOOK_PAGE_CHANGED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
    choicebookControlStates.Push (GetChoicebookPage (cb));
  }
  else if (child->IsKindOf (CLASSINFO (wxNotebook)))
  {
    wxNotebook* cb = wxStaticCast (child, wxNotebook);
    notebookControls.Push (cb);
    cb->Connect (wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, wxCommandEventHandler (DirtyHelper :: OnComponentChanged), 0, this);
    notebookControlStates.Push (GetNotebookPage (cb));
  }
  else
  {
    printf ("Unknown type for component '%s'!\n", name);
    fflush (stdout);
    return false;
  }
  return true;
}

bool DirtyHelper::RegisterComponents (wxWindow* parent, ...)
{
  va_list args;
  va_start (args, parent);
  const char* c = va_arg (args, char*);
  while (c != (const char*)0)
  {
    if (!RegisterComponent (parent, c))
      return false;
    c = va_arg (args, char*);
  }
  va_end (args);
  return true;
}

