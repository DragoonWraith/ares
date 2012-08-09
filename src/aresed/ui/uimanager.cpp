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
#include "edcommon/listctrltools.h"
#include "edcommon/model.h"

/* Fun fact: should occur after csutil/event.h, otherwise, gcc may report
 * missing csMouseEventHelper symbols. */
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/textctrl.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

//------------------------------------------------------------------------------

UIDialog::UIDialog (wxWindow* parent, const char* title, int width, int height) :
  scfImplementationType (this),
  wxDialog (parent, -1, wxString::FromUTF8 (title)), view (this)
{
  mainPanel = new wxPanel (this, -1, wxDefaultPosition, wxSize (width, height));
  wxBoxSizer* mainSizer = new wxBoxSizer (wxVERTICAL);
  SetSizer (mainSizer);
  sizer = new wxBoxSizer (wxVERTICAL);
  mainPanel->SetSizer (sizer);
  lastRowSizer = 0;
  okCancelAdded = false;
  mainPanel->SetMinSize (wxSize (width, height));
  mainSizer->Add (mainPanel, 1, wxALL, 5);
}

UIDialog::~UIDialog ()
{
  for (size_t i = 0 ; i < buttons.GetSize () ; i++)
    buttons[i]->Disconnect (wxEVT_COMMAND_BUTTON_CLICKED,
	wxCommandEventHandler (UIDialog::OnButtonClicked), 0, this);
}

void UIDialog::AddOkCancel ()
{
  if (okCancelAdded) return;
  okCancelAdded = true;
  AddRow ();
  AddSpace ();
  AddButton ("Ok");
  AddButton ("Cancel");
}

void UIDialog::AddRow (int proportion)
{
  lastRowSizer = new wxBoxSizer (wxHORIZONTAL);
  sizer->Add (lastRowSizer, proportion, wxEXPAND, 5);
}

void UIDialog::AddLabel (const char* str)
{
  CS_ASSERT (lastRowSizer != 0);
  wxStaticText* label = new wxStaticText (mainPanel, wxID_ANY, wxString::FromUTF8 (str),
      wxDefaultPosition, wxDefaultSize, 0);
  label->Wrap (-1);
  lastRowSizer->Add (label, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
}

void UIDialog::AddText (const char* name)
{
  CS_ASSERT (lastRowSizer != 0);
  wxTextCtrl* text = new wxTextCtrl (mainPanel, wxID_ANY, wxEmptyString,
      wxDefaultPosition, wxDefaultSize, 0);
  lastRowSizer->Add (text, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
  textFields.Put (name, text);
}

void UIDialog::AddMultiText (const char* name)
{
  CS_ASSERT (lastRowSizer != 0);
  wxTextCtrl* text = new wxTextCtrl (mainPanel, wxID_ANY, wxEmptyString,
      wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
  text->SetMinSize (wxSize (-1,50));
  lastRowSizer->Add (text, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
  textFields.Put (name, text);
}

void UIDialog::AddCombo (const char* name, const csStringArray& choiceArray)
{
  CS_ASSERT (lastRowSizer != 0);
  csDirtyAccessArray<wxString> choices;
  for (size_t i = 0 ; i < choiceArray.GetSize () ; i++)
    choices.Push (wxString::FromUTF8 (choiceArray.Get (i)));
  wxComboBox* combo = new wxComboBox (mainPanel, wxID_ANY, wxT (""),
      wxDefaultPosition, wxDefaultSize,
      choices.GetSize (), choices.GetArray (), 0);
  //combo->SetSelection (0);
  lastRowSizer->Add (combo, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
  comboFields.Put (name, combo);
}

void UIDialog::AddCombo (const char* name, ...)
{
  va_list args;
  va_start (args, name);
  csStringArray choiceArray;
  const char* c = va_arg (args, char*);
  while (c != (const char*)0)
  {
    choiceArray.Push (c);
    c = va_arg (args, char*);
  }
  va_end (args);
  AddCombo (name, choiceArray);
}

void UIDialog::AddChoice (const char* name, const csStringArray& choiceArray)
{
  CS_ASSERT (lastRowSizer != 0);
  csDirtyAccessArray<wxString> choices;
  for (size_t i = 0 ; i < choiceArray.GetSize () ; i++)
    choices.Push (wxString::FromUTF8 (choiceArray.Get (i)));
  wxChoice* choice = new wxChoice (mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
      choices.GetSize (), choices.GetArray (), 0);
  choice->SetSelection (0);
  lastRowSizer->Add (choice, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
  choiceFields.Put (name, choice);
}

void UIDialog::AddChoice (const char* name, ...)
{
  va_list args;
  va_start (args, name);
  csStringArray choiceArray;
  const char* c = va_arg (args, char*);
  while (c != (const char*)0)
  {
    choiceArray.Push (c);
    c = va_arg (args, char*);
  }
  va_end (args);
  AddChoice (name, choiceArray);
}

using namespace Ares;

void UIDialog::AddList (const char* name, Value* collectionValue, size_t valueColumn,
    const char* heading, const char* names)
{
  CS_ASSERT (collectionValue->GetType () == VALUE_COLLECTION);
  CS_ASSERT (lastRowSizer != 0);
  wxListCtrl* list = new wxListCtrl (mainPanel, wxID_ANY, wxDefaultPosition,
      wxDefaultSize, wxLC_REPORT);
  list->SetMinSize (wxSize (-1,150));
  lastRowSizer->Add (list, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
  ValueListInfo info;
  info.list = list;
  info.col = valueColumn;
  info.collectionValue = collectionValue;
  valueListFields.Put (name, info);
  view.DefineHeading (list, heading, names);
  view.Bind (collectionValue, list);
}

void UIDialog::AddListIndexed (const char* name, Value* collectionValue, size_t valueColumn,
    const char* heading, ...)
{
  CS_ASSERT (collectionValue->GetType () == VALUE_COLLECTION);
  CS_ASSERT (lastRowSizer != 0);
  wxListCtrl* list = new wxListCtrl (mainPanel, wxID_ANY, wxDefaultPosition,
      wxDefaultSize, wxLC_REPORT);
  list->SetMinSize (wxSize (-1,150));
  lastRowSizer->Add (list, 1, wxEXPAND | wxALL | wxALIGN_CENTER_VERTICAL, 5);
  ValueListInfo info;
  info.list = list;
  info.col = valueColumn;
  info.collectionValue = collectionValue;
  valueListFields.Put (name, info);

  va_list args;
  va_start (args, heading);
  view.DefineHeadingIndexed (list, heading, args);
  va_end (args);

  view.Bind (collectionValue, list);
}

void UIDialog::AddButton (const char* str)
{
  CS_ASSERT (lastRowSizer != 0);
  wxButton* button = new wxButton (mainPanel, wxID_ANY, wxString::FromUTF8 (str),
      wxDefaultPosition, wxDefaultSize, 0);
  lastRowSizer->Add (button, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
  button->Connect (wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler (
	UIDialog::OnButtonClicked), 0, this);
  buttons.Push (button);
}

void UIDialog::SetValue (const char* name, const char* value)
{
  wxTextCtrl* text = textFields.Get (name, 0);
  if (text)
  {
    text->SetValue (wxString::FromUTF8 (value));
    return;
  }
  wxChoice* choice = choiceFields.Get (name, 0);
  if (choice)
  {
    choice->SetStringSelection (wxString::FromUTF8 (value));
    return;
  }
  wxComboBox* combo = comboFields.Get (name, 0);
  if (combo)
  {
    combo->SetValue (wxString::FromUTF8 (value));
    return;
  }
  SetList (name, value);
}

void UIDialog::SetText (const char* name, const char* value)
{
  wxTextCtrl* text = textFields.Get (name, 0);
  if (!text) return;
  text->SetValue (wxString::FromUTF8 (value));
}

void UIDialog::SetChoice (const char* name, const char* value)
{
  wxChoice* choice = choiceFields.Get (name, 0);
  if (!choice) return;
  choice->SetStringSelection (wxString::FromUTF8 (value));
}

void UIDialog::SetCombo (const char* name, const char* value)
{
  wxComboBox* combo = comboFields.Get (name, 0);
  if (!combo) return;
  combo->SetValue (wxString::FromUTF8 (value));
}

void UIDialog::SetList (const char* name, const char* value)
{
  if (!valueListFields.Contains (name)) return;
  ValueListInfo info = valueListFields.Get (name, ValueListInfo());
  ListCtrlTools::ClearSelection (info.list);
  if (info.col == csArrayItemNotFound) return;
  size_t rowidx = 0;
  csString sValue = value;
  csRef<ValueIterator> it = info.collectionValue->GetIterator ();
  while (it->HasNext ())
  {
    Value* compositeValue = it->NextChild ();
    Value* child = compositeValue->GetChild (rowidx);
    if (sValue == child->GetStringValue ())
    {
      ListCtrlTools::SelectRow (info.list, rowidx);
      break;
    }
    rowidx++;
  }
}

void UIDialog::SetFieldContents (const DialogResult& result)
{
  DialogResult copy = result;
  Clear ();
  // Make a copy for safety.
  DialogResult::GlobalIterator it = copy.GetIterator ();
  while (it.HasNext ())
  {
    csString name;
    csString value = it.Next (name);
printf ("SetFieldContents %s %s\n", name.GetData (), value.GetData ()); fflush (stdout);
    SetValue (name, value);
  }
}

void UIDialog::Clear ()
{
  csHash<wxTextCtrl*,csString>::GlobalIterator itText = textFields.GetIterator ();
  while (itText.HasNext ())
  {
    csString name;
    wxTextCtrl* text = itText.Next (name);
    text->SetValue (wxT (""));
  }
  csHash<wxComboBox*,csString>::GlobalIterator itCo = comboFields.GetIterator ();
  while (itCo.HasNext ())
  {
    csString name;
    wxComboBox* combo = itCo.Next (name);
    combo->SetValue (wxT (""));
  }
  csHash<wxChoice*,csString>::GlobalIterator itCh = choiceFields.GetIterator ();
  while (itCh.HasNext ())
  {
    csString name;
    wxChoice* choice = itCh.Next (name);
    choice->SetSelection (0);
  }
  csHash<ValueListInfo,csString>::GlobalIterator itvLst = valueListFields.GetIterator ();
  while (itvLst.HasNext ())
  {
    csString name;
    const ValueListInfo& info = itvLst.Next (name);
    ListCtrlTools::ClearSelection (info.list);
  }
}

void UIDialog::OnButtonClicked (wxCommandEvent& event)
{
  wxButton* button = static_cast<wxButton*> (event.GetEventObject ());
  csString buttonLabel = (const char*)button->GetLabel ().mb_str (wxConvUTF8);

  fieldContents.DeleteAll ();
  csHash<wxTextCtrl*,csString>::GlobalIterator itText = textFields.GetIterator ();
  while (itText.HasNext ())
  {
    csString name;
    wxTextCtrl* text = itText.Next (name);
    csString value = (const char*)text->GetValue ().mb_str (wxConvUTF8);
    fieldContents.Put (name, value);
  }
  csHash<wxComboBox*,csString>::GlobalIterator itCo = comboFields.GetIterator ();
  while (itCo.HasNext ())
  {
    csString name;
    wxComboBox* combo = itCo.Next (name);
    csString value = (const char*)combo->GetValue ().mb_str (wxConvUTF8);
    fieldContents.Put (name, value);
  }
  csHash<wxChoice*,csString>::GlobalIterator itCh = choiceFields.GetIterator ();
  while (itCh.HasNext ())
  {
    csString name;
    wxChoice* choice = itCh.Next (name);
    csString value = (const char*)choice->GetStringSelection ().mb_str (wxConvUTF8);
    fieldContents.Put (name, value);
  }
  csHash<ValueListInfo,csString>::GlobalIterator itvLst = valueListFields.GetIterator ();
  while (itvLst.HasNext ())
  {
    csString name;
    const ValueListInfo& info = itvLst.Next (name);
    long rowidx = ListCtrlTools::GetFirstSelectedRow (info.list);
    csString value;
    if (rowidx != -1)
    {
      Value* compositeValue = info.collectionValue->GetChild (rowidx);
      if (info.col != csArrayItemNotFound)
      {
	if (compositeValue->GetType () == VALUE_COMPOSITE)
	{
          Value* child = compositeValue->GetChild (info.col);
          value = view.ValueToString (child);
	}
	else if (compositeValue->GetType () == VALUE_STRINGARRAY)
	{
	  const csStringArray* array = compositeValue->GetStringArrayValue ();
	  if (array)
	    value = array->Get (info.col);
	}
	else
	{
	  value = view.ValueToString (compositeValue);
	}
      }
      else
      {
	value = view.ValueToString (compositeValue);
      }
    }
    fieldContents.Put (name, value);
  }

  if (buttonLabel == "Ok")
  {
    EndModal (1);
    return;
  }
  else if (buttonLabel == "Cancel")
  {
    EndModal (0);
    return;
  }

  if (callback) callback->ButtonPressed (this, buttonLabel);
}

void UIDialog::AddSpace ()
{
  CS_ASSERT (lastRowSizer != 0);
  lastRowSizer->Add (0, 0, 1, wxEXPAND, 5);
}

int UIDialog::Show (iUIDialogCallback* cb)
{
  callback = cb;
  AddOkCancel ();
  mainPanel->Layout ();
  mainPanel->Fit ();
  Layout ();
  Fit ();
  //sizer->Fit (this);
  Centre (wxBOTH);

  csHash<ValueListInfo,csString>::GlobalIterator itvLst = valueListFields.GetIterator ();
  while (itvLst.HasNext ())
  {
    csString name;
    const ValueListInfo& info = itvLst.Next (name);
    info.collectionValue->Refresh ();
    for (int col = 0 ; col < info.list->GetItemCount () ; col++)
      info.list->SetColumnWidth (col, wxLIST_AUTOSIZE);
  }

  return ShowModal ();
}


//------------------------------------------------------------------------------

UIManager::UIManager (AppAresEditWX* app, wxWindow* parent) :
  scfImplementationType (this),
  app (app), parent (parent)
{
  filereqDialog = new FileReq (parent, app->GetVFS (), "/saves");
  newprojectDialog = new NewProjectDialog (parent, this, app->GetVFS ());
  cellDialog = new CellDialog (parent, this);
  contextMenuID = ID_FirstContextItem;
}

UIManager::~UIManager ()
{
  delete filereqDialog;
  delete newprojectDialog;
  delete cellDialog;
}

csRef<iString> UIManager::AskDialog (const char* description, const char* label)
{
  csRef<iString> result;
  result.AttachNew (new scfString (""));
  csRef<iUIDialog> dialog = CreateDialog (description);
  dialog->AddRow ();
  dialog->AddLabel (label);
  dialog->AddText ("name");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    result->Replace (fields.Get ("name", ""));
  }
  return result;
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

bool UIManager::Error (const char* description, ...)
{
  va_list args;
  va_start (args, description);
  csString msg;
  msg.FormatV (description, args);
  wxMessageBox (wxString::FromUTF8 (msg), wxT("Error!"),
      wxICON_ERROR, parent);
  va_end (args);
  return false;
}

bool UIManager::Ask (const char* description, ...)
{
  va_list args;
  va_start (args, description);
  csString msg;
  msg.FormatV (description, args);
  int answer = wxMessageBox (wxString::FromUTF8 (msg), wxT("Confirm"),
      wxYES_NO | wxICON_QUESTION, parent);
  va_end (args);
  return answer == wxYES;
}

csPtr<iUIDialog> UIManager::CreateDialog (const char* title)
{
  UIDialog* dialog = new UIDialog (parent, title);
  return static_cast<iUIDialog*> (dialog);
}

csPtr<iUIDialog> UIManager::CreateDialog (wxWindow* par, const char* title)
{
  UIDialog* dialog = new UIDialog (par, title);
  return static_cast<iUIDialog*> (dialog);
}

