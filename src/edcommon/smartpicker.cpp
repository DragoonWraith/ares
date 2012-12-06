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

#include <crystalspace.h>

#include "edcommon/smartpicker.h"
#include "edcommon/uitools.h"
#include "edcommon/model.h"
#include "editor/iuimanager.h"
#include "editor/iapp.h"
#include "editor/i3dview.h"
#include "editor/imodelrepository.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/listbox.h>
#include <wx/choicebk.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

//-----------------------------------------------------------------------------

void SmartPickerLogic::Cleanup ()
{
  csHash<wxTextCtrl*,csPtrKey<wxButton> >::GlobalIterator it = buttonToText.GetIterator ();
  while (it.HasNext ())
  {
    csPtrKey<wxButton> button;
    it.Next (button);
    button->Disconnect (wxEVT_COMMAND_BUTTON_CLICKED,
	wxCommandEventHandler (SmartPickerLogic::OnSearchEntityButton), 0, this);
  }
  buttonToText.Empty ();
}

void SmartPickerLogic::AddLabel (wxWindow* parent, wxBoxSizer* rowSizer, const char* txt)
{
  wxStaticText* label = new wxStaticText (parent, wxID_ANY, wxString::FromUTF8 (txt),
      wxDefaultPosition, wxDefaultSize, 0);
  label->Wrap (-1);
  rowSizer->Add (label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
}

wxTextCtrl* SmartPickerLogic::AddText (wxWindow* parent, wxBoxSizer* rowSizer)
{
  wxTextCtrl* text = new wxTextCtrl (parent, wxID_ANY, wxEmptyString,
      wxDefaultPosition, wxDefaultSize, 0);
  rowSizer->Add (text, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
  return text;
}

wxButton* SmartPickerLogic::AddButton (wxWindow* parent, wxBoxSizer* rowSizer, const char* str)
{
  wxButton* button = new wxButton (parent, wxID_ANY, wxString::FromUTF8 (str),
      wxDefaultPosition, wxDefaultSize, 0);
  rowSizer->Add (button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
  button->Connect (wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler (
	SmartPickerLogic::OnSearchEntityButton), 0, this);
  return button;
}

wxTextCtrl* SmartPickerLogic::AddEntityPicker (wxWindow* parent, wxBoxSizer* rowSizer,
      const char* label)
{
  if (label) AddLabel (parent, rowSizer, label);
  wxTextCtrl* text = AddText (parent, rowSizer);
  wxButton* button = AddButton (parent, rowSizer, "...");
  buttonToText.Put (button, text);
  return text;
}

void SmartPickerLogic::OnSearchEntityButton (wxCommandEvent& event)
{
  wxButton* button = static_cast<wxButton*> (event.GetEventObject ());
  wxTextCtrl* text = buttonToText.Get (button, (wxTextCtrl*)0);
  if (text)
  {
    using namespace Ares;
    csRef<Value> objects = uiManager->GetApplication ()->Get3DView ()->
      GetModelRepository ()->GetObjectsWithEntityValue ();
    Value* entity = uiManager->AskDialog ("Select an entity", objects, "Entity,Template,Dynfact,Logic",
	DYNOBJ_COL_ENTITY, DYNOBJ_COL_TEMPLATE, DYNOBJ_COL_FACTORY, DYNOBJ_COL_LOGIC);
    if (entity)
    {
      csString name = entity->GetStringArrayValue ()->Get (DYNOBJ_COL_ENTITY);
      UITools::SetValue (text, name);
    }
  }
}

