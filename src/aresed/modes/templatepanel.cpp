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

#include "templatepanel.h"
#include "entitymode.h"
#include "../apparesed.h"
#include "../ui/uimanager.h"
#include "../ui/listctrltools.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "propclass/chars.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(EntityTemplatePanel, wxPanel)
  EVT_CONTEXT_MENU (EntityTemplatePanel :: OnContextMenu)

  EVT_MENU (ID_TplPar_Add, EntityTemplatePanel :: OnTemplateParentAdd)
  EVT_MENU (ID_TplPar_Delete, EntityTemplatePanel :: OnTemplateParentDelete)

  EVT_MENU (ID_Char_Add, EntityTemplatePanel :: OnCharacteristicsAdd)
  EVT_MENU (ID_Char_Edit, EntityTemplatePanel :: OnCharacteristicsEdit)
  EVT_MENU (ID_Char_Delete, EntityTemplatePanel :: OnCharacteristicsDelete)

  EVT_MENU (ID_Class_Add, EntityTemplatePanel :: OnClassAdd)
  EVT_MENU (ID_Class_Delete, EntityTemplatePanel :: OnClassDelete)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

EntityTemplatePanel::EntityTemplatePanel (wxWindow* parent, UIManager* uiManager,
    EntityMode* emode) :
  uiManager (uiManager), emode (emode), tpl (0)
{
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("EntityTemplatePanel"));

  wxListCtrl* list;

  list = XRCCTRL (*this, "templateParentsList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Template", 100);

  list = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);

  list = XRCCTRL (*this, "templateClassList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Class", 100);
}

EntityTemplatePanel::~EntityTemplatePanel ()
{
}

bool EntityTemplatePanel::CheckHitList (const char* listname, bool& hasItem,
    const wxPoint& pos)
{
  wxListCtrl* list = wxStaticCast(FindWindow (
	wxXmlResource::GetXRCID (wxString::FromUTF8 (listname))), wxListCtrl);
  return ListCtrlTools::CheckHitList (list, hasItem, pos);
}

void EntityTemplatePanel::OnContextMenu (wxContextMenuEvent& event)
{
  bool hasItem;
  if (CheckHitList ("templateParentsList", hasItem, event.GetPosition ()))
    OnTemplateRMB (hasItem);
  else if (CheckHitList ("templateCharacteristicsList", hasItem, event.GetPosition ()))
    OnCharacteristicsRMB (hasItem);
  else if (CheckHitList ("templateClassList", hasItem, event.GetPosition ()))
    OnClassesRMB (hasItem);
}

void EntityTemplatePanel::OnTemplateRMB (bool hasItem)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_TplPar_Add, wxT ("&Add"));
  if (hasItem)
  {
    contextMenu.Append(ID_TplPar_Delete, wxT ("&Delete"));
  }
  PopupMenu (&contextMenu);
}

void EntityTemplatePanel::OnCharacteristicsRMB (bool hasItem)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Char_Add, wxT ("&Add"));
  if (hasItem)
  {
    contextMenu.Append(ID_Char_Edit, wxT ("&Edit"));
    contextMenu.Append(ID_Char_Delete, wxT ("&Delete"));
  }
  PopupMenu (&contextMenu);
}

void EntityTemplatePanel::OnClassesRMB (bool hasItem)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Class_Add, wxT ("&Add"));
  if (hasItem)
  {
    contextMenu.Append(ID_Class_Delete, wxT ("&Delete"));
  }
  PopupMenu (&contextMenu);
}

void EntityTemplatePanel::OnTemplateParentAdd (wxCommandEvent& event)
{
  UIDialog* dialog = uiManager->CreateDialog ("Add template");
  dialog->AddRow ();
  dialog->AddLabel ("Template:");
  dialog->AddText ("name");

  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "templateParentsList", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
    iCelEntityTemplate* t = pl->FindEntityTemplate (name);
    if (t)
    {
      ListCtrlTools::AddRow (list, name.GetData (), 0);
      UpdateTemplate ();
    }
    else
      uiManager->Error ("Can't find template with name '%s'!", name.GetData ());
  }
  delete dialog;
}

void EntityTemplatePanel::OnTemplateParentDelete (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "templateParentsList", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  UpdateTemplate ();
}

void EntityTemplatePanel::OnCharacteristicsAdd (wxCommandEvent& event)
{
  UIDialog* dialog = uiManager->CreateDialog ("Add characteristic property");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("name");
  dialog->AddRow ();
  dialog->AddLabel ("Value:");
  dialog->AddText ("value");

  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (), 0);
    UpdateTemplate ();
  }
  delete dialog;
}

void EntityTemplatePanel::OnCharacteristicsEdit (wxCommandEvent& event)
{
  UIDialog* dialog = uiManager->CreateDialog ("Edit characteristic property");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("name");
  dialog->AddRow ();
  dialog->AddLabel ("Value:");
  dialog->AddText ("value");

  wxListCtrl* list = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  dialog->SetText ("name", row[0]);
  dialog->SetText ("value", row[1]);

  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::ReplaceRow (list, idx,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (), 0);
    UpdateTemplate ();
  }
  delete dialog;
}

void EntityTemplatePanel::OnCharacteristicsDelete (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  UpdateTemplate ();
}

void EntityTemplatePanel::OnClassAdd (wxCommandEvent& event)
{
  UIDialog* dialog = uiManager->CreateDialog ("Add class");
  dialog->AddRow ();
  dialog->AddLabel ("Class:");
  dialog->AddText ("name");

  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "templateClassList", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (), 0);
    UpdateTemplate ();
  }
  delete dialog;
}

void EntityTemplatePanel::OnClassDelete (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "templateClassList", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  UpdateTemplate ();
}

void EntityTemplatePanel::UpdateTemplate ()
{
  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  tpl->RemoveParents ();
  wxListCtrl* parentsList = XRCCTRL (*this, "templateParentsList", wxListCtrl);
  for (int r = 0 ; r < parentsList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (parentsList, r);
    csString name = row[0];
    iCelEntityTemplate* t = pl->FindEntityTemplate (name);
    if (t)	// Template should normally exist since it was checked before.
      tpl->AddParent (t);
  }

  iTemplateCharacteristics* chars = tpl->GetCharacteristics ();
  chars->ClearAll ();
  wxListCtrl* charsList = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  for (int r = 0 ; r < charsList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (charsList, r);
    csString name = row[0];
    csString value = row[1];
    float f;
    csScanStr (value, "%f", &f);
    chars->SetCharacteristic (name, f);
  }

  tpl->RemoveClasses ();
  wxListCtrl* classList = XRCCTRL (*this, "templateClassList", wxListCtrl);
  for (int r = 0 ; r < classList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (classList, r);
    csString name = row[0];
    csStringID id = pl->FetchStringID (name);
    tpl->AddClass (id);
  }
}

void EntityTemplatePanel::SwitchToTpl (iCelEntityTemplate* tpl)
{
  EntityTemplatePanel::tpl = tpl;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  wxListCtrl* parentsList = XRCCTRL (*this, "templateParentsList", wxListCtrl);
  parentsList->DeleteAllItems ();
  csRef<iCelEntityTemplateIterator> itParents = tpl->GetParents ();
  while (itParents->HasNext ())
  {
    iCelEntityTemplate* parent = itParents->Next ();
    ListCtrlTools::AddRow (parentsList, parent->GetName (), 0);
  }

  wxListCtrl* charList = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  charList->DeleteAllItems ();
  csRef<iCharacteristicsIterator> itChars = tpl->GetCharacteristics ()->GetAllCharacteristics ();
  while (itChars->HasNext ())
  {
    float f;
    csString name = itChars->Next (f);
    csString value; value.Format ("%g", f);
    ListCtrlTools::AddRow (charList, name.GetData (), value.GetData (), 0);
  }

  wxListCtrl* classList = XRCCTRL (*this, "templateClassList", wxListCtrl);
  classList->DeleteAllItems ();
  const csSet<csStringID>& classes = tpl->GetClasses ();
  csSet<csStringID>::GlobalIterator itClass = classes.GetIterator ();
  while (itClass.HasNext ())
  {
    csStringID classID = itClass.Next ();
    csString className = pl->FetchString (classID);
    ListCtrlTools::AddRow (classList, className.GetData (), 0);
  }
}


