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
	wxCommandEventHandler (SmartPickerLogic::OnSearchButton), 0, this);
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

wxButton* SmartPickerLogic::AddButton (wxWindow* parent, wxBoxSizer* rowSizer, const char* str,
    bool exact)
{
  wxButton* button = new wxButton (parent, wxID_ANY, wxString::FromUTF8 (str),
      wxDefaultPosition, wxDefaultSize, exact ? wxBU_EXACTFIT : 0);
  rowSizer->Add (button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
  return button;
}

bool SmartPickerLogic::SetupPicker (SmartPickerType type, wxWindow* parent, const char* entityText,
    const char* searchButton)
{
  wxString wxentity = wxString::FromUTF8 (entityText);
  wxWindow* wentity = parent->FindWindow (wxentity);
  if (!wentity) { csPrintf ("Cannot find text control '%s'!\n", entityText); return false; }
  wxTextCtrl* text = wxStaticCast (wentity, wxTextCtrl);
  if (!text) return false;

  wxString wxbutton = wxString::FromUTF8 (searchButton);
  wxWindow* wbutton = parent->FindWindow (wxbutton);
  if (!wbutton) { csPrintf ("Cannot find button '%s'!\n", searchButton); return false; }
  wxButton* button = wxStaticCast (wbutton, wxButton);
  if (!button) return false;

  SetupPicker (type, text, button);
  return true;
}

void SmartPickerLogic::SetupPicker (SmartPickerType type, wxTextCtrl* entityText,
    wxButton* searchButton)
{
  searchButton->Connect (wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler (
	SmartPickerLogic::OnSearchButton), 0, this);
  buttonToText.Put (searchButton, entityText);
  buttonToType.Put (searchButton, type);
}

wxTextCtrl* SmartPickerLogic::AddPicker (SmartPickerType type, wxWindow* parent, wxBoxSizer* rowSizer,
      const char* label)
{
  if (label) AddLabel (parent, rowSizer, label);
  wxTextCtrl* text = AddText (parent, rowSizer);
  wxButton* button = AddButton (parent, rowSizer, "...", true);
  SetupPicker (type, text, button);
  return text;
}

void SmartPickerLogic::OnSearchButton (wxCommandEvent& event)
{
  wxButton* button = static_cast<wxButton*> (event.GetEventObject ());
  wxTextCtrl* text = buttonToText.Get (button, (wxTextCtrl*)0);
  if (text)
  {
    SmartPickerType type = buttonToType.Get (button, SPT_NONE);
    using namespace Ares;
    csRef<Value> objects;
    Value* chosen;
    int col;

    iModelRepository* rep = uiManager->GetApplication ()->Get3DView ()->GetModelRepository ();
    switch (type)
    {
      case SPT_ENTITY:
        objects = rep->GetObjectsWithEntityValue ();
        chosen = uiManager->AskDialog ("Select an entity", 500, objects, "Entity,Template,Dynfact,Logic",
	    DYNOBJ_COL_ENTITY, DYNOBJ_COL_TEMPLATE, DYNOBJ_COL_FACTORY, DYNOBJ_COL_LOGIC);
	col = DYNOBJ_COL_ENTITY;
	break;

      case SPT_QUEST:
        objects = rep->GetQuestsValue ();
	chosen = uiManager->AskDialog ("Select a quest", 400, objects, "Name,M", QUEST_COL_NAME,
	    QUEST_COL_MODIFIED);
	col = QUEST_COL_NAME;
	break;

      case SPT_TEMPLATE:
	objects = rep->GetTemplatesValue ();
	chosen = uiManager->AskDialog ("Select a template", 400, objects, "Template,M",
	    TEMPLATE_COL_NAME, TEMPLATE_COL_MODIFIED);
	col = TEMPLATE_COL_NAME;
	break;

      case SPT_NONE:
	CS_ASSERT (false);
    }
    if (chosen)
    {
      csString name = chosen->GetStringArrayValue ()->Get (col);
      UITools::SetValue (text, name, true);
    }
  }
}

