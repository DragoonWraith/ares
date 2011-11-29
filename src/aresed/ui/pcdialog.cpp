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

#include "pcdialog.h"
#include "uimanager.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "../apparesed.h"
#include "listctrltools.h"
#include "../inspect.h"

//--------------------------------------------------------------------------

static bool ToBool (csString& value)
{
  csString lvalue = value.Downcase ();
  return lvalue == "1" || lvalue == "true" || lvalue == "yes" || lvalue == "on";
}

static long ToLong (const char* value)
{
  long l;
  csScanStr (value, "%d", &l);
  return l;
}

static float ToFloat (const char* value)
{
  float l;
  csScanStr (value, "%f", &l);
  return l;
}

static csVector2 ToVector2 (const char* value)
{
  csVector2 v;
  csScanStr (value, "%f,%f", &v.x, &v.y);
  return v;
}

static csVector3 ToVector3 (const char* value)
{
  csVector3 v;
  csScanStr (value, "%f,%f,%f", &v.x, &v.y, &v.z);
  return v;
}

static csColor ToColor (const char* value)
{
  csColor v;
  csScanStr (value, "%f,%f,%f", &v.red, &v.green, &v.blue);
  return v;
}

static const char* TypeToString (celDataType type)
{
  switch (type)
  {
    case CEL_DATA_NONE: return "none";
    case CEL_DATA_BOOL: return "bool";
    case CEL_DATA_LONG: return "long";
    case CEL_DATA_FLOAT: return "float";
    case CEL_DATA_VECTOR2: return "vector2";
    case CEL_DATA_VECTOR3: return "vector3";
    case CEL_DATA_STRING: return "string";
    case CEL_DATA_COLOR: return "color";
    default: return "?";
  }
}

static celDataType StringToType (const csString& type)
{
  if (type == "bool") return CEL_DATA_BOOL;
  if (type == "long") return CEL_DATA_LONG;
  if (type == "float") return CEL_DATA_FLOAT;
  if (type == "string") return CEL_DATA_STRING;
  if (type == "vector2") return CEL_DATA_VECTOR2;
  if (type == "vector3") return CEL_DATA_VECTOR3;
  if (type == "color") return CEL_DATA_COLOR;
  return CEL_DATA_NONE;
}

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PropertyClassDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), PropertyClassDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), PropertyClassDialog :: OnCancelButton)

  EVT_LIST_ITEM_RIGHT_CLICK (XRCID("propertyListCtrl"), PropertyClassDialog :: OnPropertyRMB)
  EVT_MENU (ID_Prop_Add, PropertyClassDialog :: OnPropertyAdd)
  EVT_MENU (ID_Prop_Edit, PropertyClassDialog :: OnPropertyEdit)
  EVT_MENU (ID_Prop_Delete, PropertyClassDialog :: OnPropertyDel)

  EVT_LIST_ITEM_RIGHT_CLICK (XRCID("wireMessageListCtrl"), PropertyClassDialog :: OnWireMessageRMB)
  EVT_MENU (ID_WireMsg_Add, PropertyClassDialog :: OnWireMessageAdd)
  EVT_MENU (ID_WireMsg_Edit, PropertyClassDialog :: OnWireMessageEdit)
  EVT_MENU (ID_WireMsg_Delete, PropertyClassDialog :: OnWireMessageDel)
  EVT_LIST_ITEM_SELECTED (XRCID("wireMessageListCtrl"), PropertyClassDialog :: OnWireMessageSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("wireMessageListCtrl"), PropertyClassDialog :: OnWireMessageDeselected)

  EVT_LIST_ITEM_RIGHT_CLICK (XRCID("wireParameterListCtrl"), PropertyClassDialog :: OnWireParameterRMB)
  EVT_MENU (ID_WirePar_Add, PropertyClassDialog :: OnWireParameterAdd)
  EVT_MENU (ID_WirePar_Edit, PropertyClassDialog :: OnWireParameterEdit)
  EVT_MENU (ID_WirePar_Delete, PropertyClassDialog :: OnWireParameterDel)

  EVT_LIST_ITEM_RIGHT_CLICK (XRCID("spawnTemplateListCtrl"), PropertyClassDialog :: OnSpawnTemplateRMB)
  EVT_MENU (ID_Spawn_Add, PropertyClassDialog :: OnSpawnTemplateAdd)
  EVT_MENU (ID_Spawn_Edit, PropertyClassDialog :: OnSpawnTemplateEdit)
  EVT_MENU (ID_Spawn_Delete, PropertyClassDialog :: OnSpawnTemplateDel)

  EVT_LIST_ITEM_RIGHT_CLICK (XRCID("questParameterListCtrl"), PropertyClassDialog :: OnQuestParameterRMB)
  EVT_MENU (ID_Quest_Add, PropertyClassDialog :: OnQuestParameterAdd)
  EVT_MENU (ID_Quest_Edit, PropertyClassDialog :: OnQuestParameterEdit)
  EVT_MENU (ID_Quest_Delete, PropertyClassDialog :: OnQuestParameterDel)


  EVT_LIST_ITEM_RIGHT_CLICK (XRCID("inventoryTemplateListCtrl"), PropertyClassDialog :: OnInventoryTemplateRMB)
  EVT_MENU (ID_Inv_Add, PropertyClassDialog :: OnInventoryTemplateAdd)
  EVT_MENU (ID_Inv_Edit, PropertyClassDialog :: OnInventoryTemplateEdit)
  EVT_MENU (ID_Inv_Delete, PropertyClassDialog :: OnInventoryTemplateDel)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void PropertyClassDialog::OnOkButton (wxCommandEvent& event)
{
  if (!UpdatePC ()) return;
  callback->OkPressed (pctpl);
  pctpl = 0;
  callback = 0;
  EndModal (TRUE);
}

void PropertyClassDialog::OnCancelButton (wxCommandEvent& event)
{
  pctpl = 0;
  callback = 0;
  EndModal (TRUE);
}

void PropertyClassDialog::Show (PCEditCallback* cb)
{
  callback = cb;
  ShowModal ();
}

static size_t FindNotebookPage (wxChoicebook* book, const char* name)
{
  wxString iname = wxString::FromUTF8 (name);
  for (size_t i = 0 ; i < book->GetPageCount () ; i++)
  {
    wxString pageName = book->GetPageText (i);
    if (pageName == iname) return i;
  }
  return csArrayItemNotFound;
}

// -----------------------------------------------------------------------

void PropertyClassDialog::AddRowFromInput (const char* listComp,
    const char* nameComp, const char* valueComp, const char* typeComp)
{
  wxTextCtrl* nameText = (wxTextCtrl*)FindWindowByName (
      wxString::FromUTF8 (nameComp));
  wxTextCtrl* valueText = 0;
  if (valueComp) valueText = (wxTextCtrl*)FindWindowByName (
      wxString::FromUTF8 (valueComp));
  wxChoice* typeChoice = 0;
  if (typeComp) typeChoice = (wxChoice*)FindWindowByName (
      wxString::FromUTF8 (typeComp));
  wxListCtrl* list = (wxListCtrl*)FindWindowByName (
      wxString::FromUTF8 (listComp));

  csString name = (const char*)nameText->GetValue ().mb_str (wxConvUTF8);
  csString value;
  if (valueComp)
    value = (const char*)valueText->GetValue ().mb_str (wxConvUTF8);
  csString type;
  if (typeComp)
    type = (const char*)typeChoice->GetStringSelection ().mb_str (wxConvUTF8);

  long idx = ListCtrlTools::FindRow (list, 0, name.GetData ());
  if (idx == -1)
  {
    if (typeComp)
      ListCtrlTools::AddRow (list, name.GetData (), value.GetData (), type.GetData (), 0);
    else if (valueComp)
      ListCtrlTools::AddRow (list, name.GetData (), value.GetData (), 0);
    else
      ListCtrlTools::AddRow (list, name.GetData (), 0);
  }
  else
  {
    if (typeComp)
      ListCtrlTools::ReplaceRow (list, idx, name.GetData (), value.GetData (), type.GetData (), 0);
    else if (valueComp)
      ListCtrlTools::ReplaceRow (list, idx, name.GetData (), value.GetData (), 0);
    else
      ListCtrlTools::ReplaceRow (list, idx, name.GetData (), 0);
  }
}

void PropertyClassDialog::FillFieldsFromRow (const char* listComp,
    const char* nameComp, const char* valueComp, const char* typeComp,
    const char* delComp,
    long selIndex)
{
  wxButton* delButton = (wxButton*)FindWindowByName (
      wxString::FromUTF8 (delComp));
  delButton->Enable ();

  wxTextCtrl* nameText = (wxTextCtrl*)FindWindowByName (
      wxString::FromUTF8 (nameComp));
  wxTextCtrl* valueText = 0;
  if (valueComp) valueText = (wxTextCtrl*)FindWindowByName (
      wxString::FromUTF8 (valueComp));
  wxChoice* typeChoice = 0;
  if (typeComp) typeChoice = (wxChoice*)FindWindowByName (
      wxString::FromUTF8 (typeComp));
  wxListCtrl* list = (wxListCtrl*)FindWindowByName (
      wxString::FromUTF8 (listComp));

  csStringArray row = ListCtrlTools::ReadRow (list, selIndex);
  nameText->SetValue (wxString::FromUTF8 (row[0]));
  if (valueComp)
    valueText->SetValue (wxString::FromUTF8 (row[1]));
  if (typeComp)
    typeChoice->SetStringSelection (wxString::FromUTF8 (row[2]));
}

// -----------------------------------------------------------------------

bool PropertyClassDialog::UpdateCurrentWireParams ()
{
  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  wxTextCtrl* msgText = XRCCTRL (*this, "wireMessageText", wxTextCtrl);

  csString msg = (const char*)msgText->GetValue ().mb_str (wxConvUTF8);
  ParHash d;
  ParHash& params = wireParams.GetOrCreate (msg, d);
  params.DeleteAll ();
  for (int r = 0 ; r < parList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (parList, r);
    csString name = row[0];
    csString value = row[1];
    csString type = row[2];
    csRef<iParameter> par = pm->GetParameter (value, StringToType (type));
    if (!par) return false;
    params.Put (pl->FetchStringID (name), par);
  }
  return true;
}

void PropertyClassDialog::OnWireParameterRMB (wxListEvent& event)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_WirePar_Add, wxT ("&Add"));
  contextMenu.Append(ID_WirePar_Edit, wxT ("&Edit"));
  contextMenu.Append(ID_WirePar_Delete, wxT ("&Delete"));
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassDialog::GetWireParDialog ()
{
  if (!wireParDialog)
  {
    wireParDialog = uiManager->CreateDialog ("Edit parameter");
    wireParDialog->AddRow ();
    wireParDialog->AddLabel ("Name:");
    wireParDialog->AddText ("name");
    wireParDialog->AddChoice ("type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", 0);
    wireParDialog->AddRow ();
    wireParDialog->AddMultiText ("value");
  }
  return wireParDialog;
}

void PropertyClassDialog::OnWireParameterEdit (wxCommandEvent& event)
{
  UIDialog* dialog = GetWireParDialog ();

  dialog->Clear ();
  wxListCtrl* list = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  dialog->SetText ("name", row[0]);
  dialog->SetText ("value", row[1]);
  dialog->SetChoice ("type", row[2]);

  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::ReplaceRow (list, idx,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (),
	fields.Get ("type", "").GetData (), 0);
    UpdateCurrentWireParams ();
  }
}

void PropertyClassDialog::OnWireParameterAdd (wxCommandEvent& event)
{
  UIDialog* dialog = GetWireParDialog ();
  dialog->Clear ();
  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (),
	fields.Get ("type", "").GetData (), 0);
    UpdateCurrentWireParams ();
  }
}

void PropertyClassDialog::OnWireParameterDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
  UpdateCurrentWireParams ();
}

void PropertyClassDialog::OnWireMessageRMB (wxListEvent& event)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_WireMsg_Add, wxT ("&Add"));
  contextMenu.Append(ID_WireMsg_Edit, wxT ("&Edit"));
  contextMenu.Append(ID_WireMsg_Delete, wxT ("&Delete"));
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassDialog::GetWireMsgDialog ()
{
  if (!wireMsgDialog)
  {
    wireMsgDialog = uiManager->CreateDialog ("Edit message");
    wireMsgDialog->AddRow ();
    wireMsgDialog->AddLabel ("Message:");
    wireMsgDialog->AddText ("name");
    wireMsgDialog->AddRow ();
    wireMsgDialog->AddLabel ("Entity:");
    wireMsgDialog->AddText ("entity");
    wireMsgDialog->AddButton ("...");	// @@@ Not implemented yet.
  }
  return wireMsgDialog;
}

void PropertyClassDialog::OnWireMessageEdit (wxCommandEvent& event)
{
  UIDialog* dialog = GetWireMsgDialog ();

  dialog->Clear ();
  wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  dialog->SetText ("name", row[0]);
  dialog->SetText ("entity", row[1]);

  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::ReplaceRow (list, idx,
	fields.Get ("name", "").GetData (),
	fields.Get ("entity", "").GetData (), 0);
    wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
    parList->DeleteAllItems ();
  }
}

void PropertyClassDialog::OnWireMessageAdd (wxCommandEvent& event)
{
  UIDialog* dialog = GetWireMsgDialog ();
  dialog->Clear ();
  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (),
	fields.Get ("entity", "").GetData (), 0);
    wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
    parList->DeleteAllItems ();
  }
}

void PropertyClassDialog::OnWireMessageDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
}

void PropertyClassDialog::OnWireMessageSelected (wxListEvent& event)
{
  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();

  long idx = event.GetIndex ();
  wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  csString msg = row[0];

  const ParHash& params = wireParams.Get (msg, ParHash ());
  ParHash::ConstGlobalIterator it = params.GetIterator ();
  while (it.HasNext ())
  {
    csStringID parid;
    csRef<iParameter> par = it.Next (parid);

    csString name = pl->FetchString (parid);
    if (name == "msgid") continue;	// Ignore this one.
    if (name == "entity") continue;	// Ignore this one.
    csString val = par->GetOriginalExpression ();
    csString type = TypeToString (par->GetPossibleType ());
    ListCtrlTools::AddRow (parList, name.GetData (), val.GetData (), type.GetData (), 0);
  }
}

void PropertyClassDialog::OnWireMessageDeselected (wxListEvent& event)
{
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
}

bool PropertyClassDialog::UpdateWire ()
{
  pctpl->SetName ("pclogic.wire");
  pctpl->RemoveAllProperties ();

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxListCtrl* outputList = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  //wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  wxTextCtrl* inputMaskText = XRCCTRL (*this, "wireInputMaskText", wxTextCtrl);

  {
    ParHash params;
    csString mask = (const char*)inputMaskText->GetValue ().mb_str (wxConvUTF8);
    csRef<iParameter> par = pm->GetParameter (mask, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (pl->FetchStringID ("mask"), par);
    pctpl->PerformAction (pl->FetchStringID ("AddInput"), params);
  }

  for (int r = 0 ; r < outputList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (outputList, r);
    csString message = row[0];
    csString entity = row[1];
    ParHash params;

    csRef<iParameter> par;
    par = pm->GetParameter (message, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (pl->FetchStringID ("msgid"), par);
    par = pm->GetParameter (entity, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (pl->FetchStringID ("entity"), par);

    const ParHash& wparams = wireParams.Get (message, ParHash ());
    ParHash::ConstGlobalIterator it = wparams.GetIterator ();
    while (it.HasNext ())
    {
      csStringID parid;
      csRef<iParameter> par = it.Next (parid);
      params.Put (parid, par);
    }

    pctpl->PerformAction (pl->FetchStringID ("AddOutput"), params);
  }
  return true;
}

void PropertyClassDialog::FillWire ()
{
  wxListCtrl* outputList = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  outputList->DeleteAllItems ();
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
  wxTextCtrl* inputMaskText = XRCCTRL (*this, "wireInputMaskText", wxTextCtrl);
  inputMaskText->SetValue (wxT (""));

  if (!pctpl || csString ("pclogic.wire") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  csString inputMask = InspectTools::GetActionParameterValueString (pl, pctpl,
      "AddInput", "mask");
  inputMaskText->SetValue (wxString::FromUTF8 (inputMask));

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csStringID msgID = pl->FetchStringID ("msgid");
    csStringID entityID = pl->FetchStringID ("entity");
    if (name == "AddOutput")
    {
      csString msgName;
      csString entName;
      ParHash paramsHash;
      while (params->HasNext ())
      {
	csStringID parid;
	iParameter* par = params->Next (parid);
	if (parid == msgID) msgName = par->GetOriginalExpression ();
	else if (parid == entityID) entName = par->GetOriginalExpression ();
	paramsHash.Put (parid, par);
      }
      wireParams.Put (msgName, paramsHash);
      ListCtrlTools::AddRow (outputList, msgName.GetData (), entName.GetData (), 0);
    }
  }
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnSpawnTemplateRMB (wxListEvent& event)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Spawn_Add, wxT ("&Add"));
  contextMenu.Append(ID_Spawn_Edit, wxT ("&Edit"));
  contextMenu.Append(ID_Spawn_Delete, wxT ("&Delete"));
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassDialog::GetSpawnTemplateDialog ()
{
  if (!spawnTempDialog)
  {
    spawnTempDialog = uiManager->CreateDialog ("Edit template");
    spawnTempDialog->AddRow ();
    spawnTempDialog->AddLabel ("Template:");
    spawnTempDialog->AddText ("name");
  }
  return spawnTempDialog;
}

void PropertyClassDialog::OnSpawnTemplateEdit (wxCommandEvent& event)
{
  UIDialog* dialog = GetSpawnTemplateDialog ();

  dialog->Clear ();
  wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  dialog->SetText ("name", row[0]);

  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::ReplaceRow (list, idx,
	fields.Get ("name", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnSpawnTemplateAdd (wxCommandEvent& event)
{
  UIDialog* dialog = GetSpawnTemplateDialog ();
  dialog->Clear ();
  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnSpawnTemplateDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
}

bool PropertyClassDialog::UpdateSpawn ()
{
  pctpl->SetName ("pclogic.spawn");
  pctpl->RemoveAllProperties ();

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  wxCheckBox* repeatCB = XRCCTRL (*this, "spawnRepeatCheckBox", wxCheckBox);
  wxCheckBox* randomCB = XRCCTRL (*this, "spawnRandomCheckBox", wxCheckBox);
  wxCheckBox* spawnuniqueCB = XRCCTRL (*this, "spawnUniqueCheckBox", wxCheckBox);
  wxCheckBox* namecounterCB = XRCCTRL (*this, "spawnNameCounterCheckBox", wxCheckBox);
  wxTextCtrl* minDelayText = XRCCTRL (*this, "spawnMinDelayText", wxTextCtrl);
  wxTextCtrl* maxDelayText = XRCCTRL (*this, "spawnMaxDelayText", wxTextCtrl);
  wxTextCtrl* inhibitText = XRCCTRL (*this, "spawnInhibitText", wxTextCtrl);

  csStringID actionID = pl->FetchStringID ("AddEntityTemplateType");
  csStringID nameID = pl->FetchStringID ("template");
  for (int r = 0 ; r < list->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (list, r);
    csString name = row[0];
    ParHash params;
    csRef<iParameter> par = pm->GetParameter (name, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (nameID, par);
    pctpl->PerformAction (actionID, params);
  }

  {
    csString mindelay = (const char*)minDelayText->GetValue ().mb_str (wxConvUTF8);
    csString maxdelay = (const char*)maxDelayText->GetValue ().mb_str (wxConvUTF8);
    ParHash params;
    csRef<iParameter> par;

    par = pm->GetParameter (repeatCB->IsChecked () ? "true" : "false", CEL_DATA_BOOL);
    if (!par) return false;
    params.Put (pl->FetchStringID ("repeat"), par);

    par = pm->GetParameter (randomCB->IsChecked () ? "true" : "false", CEL_DATA_BOOL);
    if (!par) return false;
    params.Put (pl->FetchStringID ("random"), par);

    par = pm->GetParameter (mindelay, CEL_DATA_LONG);
    if (!par) return false;
    params.Put (pl->FetchStringID ("mindelay"), par);

    par = pm->GetParameter (maxdelay, CEL_DATA_LONG);
    if (!par) return false;
    params.Put (pl->FetchStringID ("maxdelay"), par);

    pctpl->PerformAction (pl->FetchStringID ("SetTiming"), params);
  }

  {
    csString inhibit = (const char*)inhibitText->GetValue ().mb_str (wxConvUTF8);
    ParHash params;
    csRef<iParameter> par = pm->GetParameter (inhibit, CEL_DATA_LONG);
    if (!par) return false;
    params.Put (pl->FetchStringID ("count"), par);

    pctpl->PerformAction (pl->FetchStringID ("Inhibit"), params);
  }

  pctpl->SetProperty (pl->FetchStringID ("spawnunique"), spawnuniqueCB->IsChecked ());
  pctpl->SetProperty (pl->FetchStringID ("namecounter"), namecounterCB->IsChecked ());

  // @@@ TODO AddSpawnPosition
  return true;
}

void PropertyClassDialog::FillSpawn ()
{
  wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  list->DeleteAllItems ();
  wxCheckBox* repeatCB = XRCCTRL (*this, "spawnRepeatCheckBox", wxCheckBox);
  repeatCB->SetValue (false);
  wxCheckBox* randomCB = XRCCTRL (*this, "spawnRandomCheckBox", wxCheckBox);
  randomCB->SetValue (false);
  wxCheckBox* spawnuniqueCB = XRCCTRL (*this, "spawnUniqueCheckBox", wxCheckBox);
  spawnuniqueCB->SetValue (false);
  wxCheckBox* namecounterCB = XRCCTRL (*this, "spawnNameCounterCheckBox", wxCheckBox);
  namecounterCB->SetValue (false);
  wxTextCtrl* minDelayText = XRCCTRL (*this, "spawnMinDelayText", wxTextCtrl);
  minDelayText->SetValue (wxT (""));
  wxTextCtrl* maxDelayText = XRCCTRL (*this, "spawnMaxDelayText", wxTextCtrl);
  maxDelayText->SetValue (wxT (""));
  wxTextCtrl* inhibitText = XRCCTRL (*this, "spawnInhibitText", wxTextCtrl);
  inhibitText->SetValue (wxT (""));

  if (!pctpl || csString ("pclogic.spawn") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  bool repeat = InspectTools::GetActionParameterValueBool (pl, pctpl,
      "SetTiming", "repeat");
  repeatCB->SetValue (repeat);
  bool random = InspectTools::GetActionParameterValueBool (pl, pctpl,
      "SetTiming", "random");
  randomCB->SetValue (random);
  bool valid;
  long mindelay = InspectTools::GetActionParameterValueLong (pl, pctpl,
      "SetTiming", "mindelay", &valid);
  if (valid)
  {
    csString s; s.Format ("%ld", mindelay);
    minDelayText->SetValue (wxString::FromUTF8 (s));
  }
  long maxdelay = InspectTools::GetActionParameterValueLong (pl, pctpl,
      "SetTiming", "maxdelay", &valid);
  if (valid)
  {
    csString s; s.Format ("%ld", maxdelay);
    maxDelayText->SetValue (wxString::FromUTF8 (s));
  }

  bool unique = InspectTools::GetPropertyValueBool (pl, pctpl, "spawnunique");
  spawnuniqueCB->SetValue (unique);
  bool namecounter = InspectTools::GetPropertyValueBool (pl, pctpl, "namecounter");
  namecounterCB->SetValue (namecounter);

  long inhibit = InspectTools::GetActionParameterValueLong (pl, pctpl,
      "Inhibit", "count", &valid);
  if (valid)
  {
    csString s; s.Format ("%ld", inhibit);
    inhibitText->SetValue (wxString::FromUTF8 (s));
  }

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csStringID tplID = pl->FetchStringID ("template");
    if (name == "AddEntityTemplateType")
    {
      csString tplName;
      while (params->HasNext ())
      {
	csStringID parid;
	iParameter* par = params->Next (parid);
	if (parid == tplID) tplName = par->GetOriginalExpression ();
      }
      if (!tplName.IsEmpty ())
        ListCtrlTools::AddRow (list, tplName.GetData (), 0);
    }
  }
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnQuestParameterRMB (wxListEvent& event)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Quest_Add, wxT ("&Add"));
  contextMenu.Append(ID_Quest_Edit, wxT ("&Edit"));
  contextMenu.Append(ID_Quest_Delete, wxT ("&Delete"));
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassDialog::GetQuestDialog ()
{
  if (!questParDialog)
  {
    questParDialog = uiManager->CreateDialog ("Edit parameter");
    questParDialog->AddRow ();
    questParDialog->AddLabel ("Name:");
    questParDialog->AddText ("name");
    questParDialog->AddChoice ("type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", 0);
    questParDialog->AddRow ();
    questParDialog->AddMultiText ("value");
  }
  return questParDialog;
}

void PropertyClassDialog::OnQuestParameterEdit (wxCommandEvent& event)
{
  UIDialog* dialog = GetQuestDialog ();

  dialog->Clear ();
  wxListCtrl* list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  dialog->SetText ("name", row[0]);
  dialog->SetText ("value", row[1]);
  dialog->SetChoice ("type", row[2]);

  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::ReplaceRow (list, idx,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (),
	fields.Get ("type", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnQuestParameterAdd (wxCommandEvent& event)
{
  UIDialog* dialog = GetQuestDialog ();
  dialog->Clear ();
  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (),
	fields.Get ("type", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnQuestParameterDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
}

bool PropertyClassDialog::UpdateQuest ()
{
  pctpl->SetName ("pclogic.quest");
  pctpl->RemoveAllProperties ();

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxListCtrl* parList = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  wxListBox* stateList = XRCCTRL (*this, "questStateBox", wxListBox);
  wxTextCtrl* questText = XRCCTRL (*this, "questText", wxTextCtrl);

  csString questName = (const char*)questText->GetValue ().mb_str (wxConvUTF8);
  if (questName.IsEmpty ())
  {
    uiManager->Error ("Empty quest name is not allowed!");
    return false;
  }

  ParHash params;
  csRef<iParameter> par = pm->GetParameter (questName, CEL_DATA_STRING);
  if (!par) return false;
  params.Put (pl->FetchStringID ("name"), par);
  for (int r = 0 ; r < parList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (parList, r);
    csString name = row[0];
    csString value = row[1];
    csString type = row[2];
    csStringID nameID = pl->FetchStringID (name);
    csRef<iParameter> par = pm->GetParameter (value, StringToType (type));
    if (!par) return false;
    params.Put (nameID, par);
  }
  pctpl->PerformAction (pl->FetchStringID ("NewQuest"), params);

  csString state = (const char*)stateList->GetStringSelection ().mb_str (wxConvUTF8);
  if (!state.IsEmpty ())
    pctpl->SetProperty (pl->FetchStringID ("state"), state.GetData ());
  return true;
}

void PropertyClassDialog::FillQuest ()
{
  wxListBox* stateList = XRCCTRL (*this, "questStateBox", wxListBox);
  stateList->Clear ();
  wxListCtrl* parList = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
  wxTextCtrl* text = XRCCTRL (*this, "questText", wxTextCtrl);
  text->SetValue (wxT (""));

  if (!pctpl || csString ("pclogic.quest") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csString questName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
  if (questName.IsEmpty ()) return;

  text->SetValue (wxString::FromUTF8 (questName));

  csRef<iQuestManager> quest_mgr = csQueryRegistryOrLoad<iQuestManager> (
    uiManager->GetApp ()->GetObjectRegistry (),
    "cel.manager.quests");
  iQuestFactory* questFact = quest_mgr->GetQuestFactory (questName);
  if (!questFact) return;

  csString defaultState = InspectTools::GetPropertyValueString (pl, pctpl, "state");

  // Fill all states and mark the default state.
  wxArrayString states;
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  size_t selIdx = csArrayItemNotFound;
  size_t i = 0;
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    states.Add (wxString::FromUTF8 (stateFact->GetName ()));
    if (defaultState == stateFact->GetName ()) selIdx = i;
    i++;
  }
  stateList->InsertItems (states, 0);
  if (selIdx != csArrayItemNotFound)
    stateList->SetSelection (selIdx);

  // Fill all parameters for the quest.
  size_t nqIdx = pctpl->FindProperty (pl->FetchStringID ("NewQuest"));
  if (nqIdx != csArrayItemNotFound)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> parit = pctpl->GetProperty (nqIdx, id, data);
    while (parit->HasNext ())
    {
      csStringID parid;
      iParameter* par = parit->Next (parid);
      csString name = pl->FetchString (parid);
      if (name == "name") continue;	// Ignore this one.
      csString val = par->GetOriginalExpression ();
      csString type = TypeToString (par->GetPossibleType ());
      ListCtrlTools::AddRow (parList, name.GetData (), val.GetData (), type.GetData (), 0);
    }
  }
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnInventoryTemplateRMB (wxListEvent& event)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Inv_Add, wxT ("&Add"));
  contextMenu.Append(ID_Inv_Edit, wxT ("&Edit"));
  contextMenu.Append(ID_Inv_Delete, wxT ("&Delete"));
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassDialog::GetInventoryTemplateDialog ()
{
  if (!invTempDialog)
  {
    invTempDialog = uiManager->CreateDialog ("Edit template/amount");
    invTempDialog->AddRow ();
    invTempDialog->AddLabel ("Template:");
    invTempDialog->AddText ("name");
    invTempDialog->AddText ("amount");
  }
  return invTempDialog;
}

void PropertyClassDialog::OnInventoryTemplateEdit (wxCommandEvent& event)
{
  UIDialog* dialog = GetInventoryTemplateDialog ();

  dialog->Clear ();
  wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  dialog->SetText ("name", row[0]);
  dialog->SetText ("amount", row[1]);

  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::ReplaceRow (list, idx,
	fields.Get ("name", "").GetData (),
	fields.Get ("amount", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnInventoryTemplateAdd (wxCommandEvent& event)
{
  UIDialog* dialog = GetInventoryTemplateDialog ();
  dialog->Clear ();
  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (),
	fields.Get ("amount", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnInventoryTemplateDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
}

bool PropertyClassDialog::UpdateInventory ()
{
  pctpl->SetName ("pctools.inventory");
  pctpl->RemoveAllProperties ();

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  csStringID actionID = pl->FetchStringID ("AddTemplate");
  csStringID nameID = pl->FetchStringID ("name");
  csStringID amountID = pl->FetchStringID ("amount");
  for (int r = 0 ; r < list->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (list, r);
    csString name = row[0];
    csString amount = row[1];
    ParHash params;

    csRef<iParameter> par;
    par = pm->GetParameter (name, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (nameID, par);

    par = pm->GetParameter (amount, CEL_DATA_LONG);
    if (!par) return false;
    params.Put (amountID, par);

    pctpl->PerformAction (actionID, params);
  }

  wxTextCtrl* lootText = XRCCTRL (*this, "inventoryLootTextCtrl", wxTextCtrl);
  csString loot = (const char*)lootText->GetValue ().mb_str (wxConvUTF8);
  if (!loot.IsEmpty ())
  {
    ParHash params;
    csRef<iParameter> par = pm->GetParameter (loot, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (nameID, par);
    pctpl->PerformAction (pl->FetchStringID ("SetLootGenerator"), params);
  }
  return true;
}

void PropertyClassDialog::FillInventory ()
{
  wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  list->DeleteAllItems ();
  wxTextCtrl* lootText = XRCCTRL (*this, "inventoryLootTextCtrl", wxTextCtrl);
  lootText->SetValue (wxT (""));

  if (!pctpl || csString ("pctools.inventory") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csStringID nameID = pl->FetchStringID ("name");
    csStringID amountID = pl->FetchStringID ("amount");
    if (name == "AddTemplate")
    {
      csString parName;
      csString parAmount;
      while (params->HasNext ())
      {
	csStringID parid;
	iParameter* par = params->Next (parid);
	if (parid == nameID) parName = par->GetOriginalExpression ();
	else if (parid == amountID) parAmount = par->GetOriginalExpression ();
      }
      ListCtrlTools::AddRow (list, parName.GetData (), parAmount.GetData (), 0);
    }
  }

  csString lootName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "SetLootGenerator", "name");
  lootText->SetValue (wxString::FromUTF8 (lootName));
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnPropertyRMB (wxListEvent& event)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Prop_Add, wxT ("&Add"));
  contextMenu.Append(ID_Prop_Edit, wxT ("&Edit"));
  contextMenu.Append(ID_Prop_Delete, wxT ("&Delete"));
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassDialog::GetPropertyDialog ()
{
  if (!propDialog)
  {
    propDialog = uiManager->CreateDialog ("Edit property");
    propDialog->AddRow ();
    propDialog->AddLabel ("Name:");
    propDialog->AddText ("name");
    propDialog->AddChoice ("type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", 0);
    propDialog->AddRow ();
    propDialog->AddMultiText ("value");
  }
  return propDialog;
}

void PropertyClassDialog::OnPropertyEdit (wxCommandEvent& event)
{
  UIDialog* dialog = GetPropertyDialog ();

  dialog->Clear ();
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  dialog->SetText ("name", row[0]);
  dialog->SetText ("value", row[1]);
  dialog->SetChoice ("type", row[2]);

  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::ReplaceRow (list, idx,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (),
	fields.Get ("type", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnPropertyAdd (wxCommandEvent& event)
{
  UIDialog* dialog = GetPropertyDialog ();
  dialog->Clear ();
  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (),
	fields.Get ("value", "").GetData (),
	fields.Get ("type", "").GetData (), 0);
  }
}

void PropertyClassDialog::OnPropertyDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
}

bool PropertyClassDialog::UpdateProperties ()
{
  pctpl->SetName ("pctools.properties");
  pctpl->RemoveAllProperties ();

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  for (int r = 0 ; r < list->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (list, r);
    csString name = row[0];
    csString value = row[1];
    csString type = row[2];
    csStringID nameID = pl->FetchStringID (name);
    if (type == "bool") pctpl->SetProperty (nameID, ToBool (value));
    else if (type == "long") pctpl->SetProperty (nameID, ToLong (value));
    else if (type == "float") pctpl->SetProperty (nameID, ToFloat (value));
    else if (type == "string") pctpl->SetProperty (nameID, value.GetData ());
    else if (type == "vector2") pctpl->SetProperty (nameID, ToVector2 (value));
    else if (type == "vector3") pctpl->SetProperty (nameID, ToVector3 (value));
    else if (type == "color") pctpl->SetProperty (nameID, ToColor (value));
    else
    {
      uiManager->Error ("Unknown type '%s'\n", type.GetData ());
      return false;
    }
  }
  return true;
}

void PropertyClassDialog::FillProperties ()
{
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  list->DeleteAllItems ();

  if (!pctpl || csString ("pctools.properties") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    // @@@ Should do something with actions.
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csString value, type;
    celParameterTools::ToString (data, value);
    type = TypeToString (data.type);
    ListCtrlTools::AddRow (list, name.GetData (), value.GetData (), type.GetData (), 0);
  }
}

// -----------------------------------------------------------------------

bool PropertyClassDialog::UpdatePC ()
{
  wxTextCtrl* tagText = XRCCTRL (*this, "tagTextCtrl", wxTextCtrl);
  csString tag = (const char*)tagText->GetValue ().mb_str (wxConvUTF8);

  wxChoicebook* book = XRCCTRL (*this, "pcChoicebook", wxChoicebook);
  int pageSel = book->GetSelection ();
  if (pageSel == wxNOT_FOUND)
  {
    uiManager->Error ("Internal error! Page not found!");
    return false;
  }
  wxString pageTxt = book->GetPageText (pageSel);
  csString page = (const char*)pageTxt.mb_str (wxConvUTF8);

  iCelPropertyClassTemplate* pc = tpl->FindPropertyClassTemplate (page, tag);
  if (!pctpl)
  {
    if (pc)
    {
      uiManager->Error ("Property class with this name and tag already exists!");
      return false;
    }
    pctpl = tpl->CreatePropertyClassTemplate ();
  }
  else
  {
    if (pc && pc != pctpl)
    {
      uiManager->Error ("Property class with this name and tag already exists!");
      return false;
    }
  }

  if (tag.IsEmpty ())
    pctpl->SetTag (0);
  else
    pctpl->SetTag (tag);

  if (page == "pctools.properties") return UpdateProperties ();
  else if (page == "pctools.inventory") return UpdateInventory ();
  else if (page == "pclogic.wire") return UpdateWire ();
  else if (page == "pclogic.quest") return UpdateQuest ();
  else if (page == "pclogic.spawn") return UpdateSpawn ();
  else
  {
    uiManager->Error ("Unknown property class type!");
    return false;
  }
}

void PropertyClassDialog::SwitchToPC (iCelEntityTemplate* tpl,
    iCelPropertyClassTemplate* pctpl)
{
  PropertyClassDialog::tpl = tpl;
  PropertyClassDialog::pctpl = pctpl;

  if (pctpl)
  {
    csString pcName = pctpl->GetName ();
    csString tagName = pctpl->GetTag ();

    wxChoicebook* book = XRCCTRL (*this, "pcChoicebook", wxChoicebook);
    size_t pageIdx = FindNotebookPage (book, pcName);
    book->ChangeSelection (pageIdx);

    wxTextCtrl* text = XRCCTRL (*this, "tagTextCtrl", wxTextCtrl);
    text->SetValue (wxString::FromUTF8 (tagName));
  }

  FillProperties ();
  FillInventory ();
  FillQuest ();
  FillSpawn ();
  FillWire ();
}

PropertyClassDialog::PropertyClassDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("PropertyClassDialog"));

  wxListCtrl* list;

  list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);
  ListCtrlTools::SetColumn (list, 2, "Type", 100);

  list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Amount", 100);

  list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Template", 100);

  list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);
  ListCtrlTools::SetColumn (list, 2, "Type", 100);

  list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Entity", 100);

  list = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);
  ListCtrlTools::SetColumn (list, 2, "Type", 100);

  propDialog = 0;
  invTempDialog = 0;
  spawnTempDialog = 0;
  questParDialog = 0;
  wireParDialog = 0;
  wireMsgDialog = 0;
}

PropertyClassDialog::~PropertyClassDialog ()
{
  delete propDialog;
  delete invTempDialog;
  delete spawnTempDialog;
  delete questParDialog;
  delete wireParDialog;
  delete wireMsgDialog;
}


