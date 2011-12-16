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

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/listbox.h>
#include <wx/choicebk.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

//-----------------------------------------------------------------------------

void UITools::ClearControl (wxWindow* parent, const char* name)
{
  wxString wxname = wxString::FromUTF8 (name);
  wxWindow* child = parent->FindWindow (wxname);
  if (!child)
  {
    printf ("Error clearing control '%s'!\n", name);
    fflush (stdout);
  }
  else
  {
    if (child->IsKindOf (CLASSINFO (wxTextCtrl)))
    {
      wxTextCtrl* text = wxStaticCast (child, wxTextCtrl);
      text->SetValue (wxT (""));
    }
    else if (child->IsKindOf (CLASSINFO (wxStaticText)))
    {
      wxStaticText* text = wxStaticCast (child, wxStaticText);
      text->SetLabel (wxT (""));
    }
    else if (child->IsKindOf (CLASSINFO (wxCheckBox)))
    {
      wxCheckBox* cb = wxStaticCast (child, wxCheckBox);
      cb->SetValue (false);
    }
    else
    {
      printf ("Can't clear control '%s', unknown type!\n", name);
      fflush (stdout);
    }
  }
}

void UITools::ClearControls (wxWindow* parent, ...)
{
  va_list args;
  va_start (args, parent);
  const char* c = va_arg (args, char*);
  while (c != (const char*)0)
  {
    ClearControl (parent, c);
    c = va_arg (args, char*);
  }
  va_end (args);
}

void UITools::SetValue (wxWindow* parent, const char* name, float value)
{
  csString v;
  v.Format ("%g", value);
  SetValue (parent, name, v);
}

void UITools::SetValue (wxWindow* parent, const char* name, int value)
{
  csString v;
  v.Format ("%d", value);
  SetValue (parent, name, v);
}

void UITools::SetValue (wxWindow* parent, const char* name, const char* value)
{
  wxString wxname = wxString::FromUTF8 (name);
  wxString wxvalue = wxString::FromUTF8 (value);
  wxWindow* child = parent->FindWindow (wxname);
  if (!child)
  {
    printf ("Error setting value for '%s'!\n", name);
    fflush (stdout);
  }
  else
  {
    if (child->IsKindOf (CLASSINFO (wxTextCtrl)))
    {
      wxTextCtrl* text = wxStaticCast (child, wxTextCtrl);
      text->SetValue (wxvalue);
    }
    else if (child->IsKindOf (CLASSINFO (wxStaticText)))
    {
      wxStaticText* text = wxStaticCast (child, wxStaticText);
      text->SetLabel (wxvalue);
    }
    else if (child->IsKindOf (CLASSINFO (wxButton)))
    {
      wxButton* cb = wxStaticCast (child, wxButton);
      cb->SetLabel (wxvalue);
    }
    else
    {
      printf ("Can't set value for '%s', unknown type!\n", name);
      fflush (stdout);
    }
  }
}

bool UITools::SwitchPage (wxWindow* parent, const char* name, const char* pageName)
{
  wxString wxname = wxString::FromUTF8 (name);
  wxString wxpage = wxString::FromUTF8 (pageName);
  wxWindow* child = parent->FindWindow (wxname);
  if (!child)
  {
    printf ("Can't find choicebook or notebook '%s'!\n", name);
    fflush (stdout);
    return false;
  }
  if (child->IsKindOf (CLASSINFO (wxChoicebook)))
  {
    wxChoicebook* book = wxStaticCast (child, wxChoicebook);
    for (size_t i = 0 ; i < book->GetPageCount () ; i++)
    {
      wxString wxp = book->GetPageText (i);
      if (wxp == wxpage)
      {
	book->ChangeSelection (i);
	return true;
      }
    }
    return false;
  }
  else if (child->IsKindOf (CLASSINFO (wxNotebook)))
  {
    wxNotebook* book = wxStaticCast (child, wxNotebook);
    for (size_t i = 0 ; i < book->GetPageCount () ; i++)
    {
      wxString wxp = book->GetPageText (i);
      if (wxp == wxpage)
      {
	book->ChangeSelection (i);
	return true;
      }
    }
    return false;
  }
  else
  {
    printf ("Unknown type for '%s'! Expected choicebook or notebook!\n", name);
    fflush (stdout);
    return false;
  }
}

