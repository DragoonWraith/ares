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

#include "pcpanel.h"
#include "entitymode.h"
#include "../ui/uimanager.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "../apparesed.h"
#include "../ui/listctrltools.h"
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

BEGIN_EVENT_TABLE(PropertyClassPanel, wxPanel)
  EVT_CONTEXT_MENU (PropertyClassPanel :: OnContextMenu)

  EVT_CHOICEBOOK_PAGE_CHANGED (XRCID("pcChoicebook"), PropertyClassPanel :: OnChoicebookPageChange)
  EVT_TEXT_ENTER (XRCID("tagTextCtrl"), PropertyClassPanel :: OnUpdateEvent)

  EVT_MENU (ID_WireMsg_Add, PropertyClassPanel :: OnWireMessageAdd)
  EVT_MENU (ID_WireMsg_Edit, PropertyClassPanel :: OnWireMessageEdit)
  EVT_MENU (ID_WireMsg_Delete, PropertyClassPanel :: OnWireMessageDel)
  EVT_LIST_ITEM_SELECTED (XRCID("wireMessageListCtrl"), PropertyClassPanel :: OnWireMessageSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("wireMessageListCtrl"), PropertyClassPanel :: OnWireMessageDeselected)

  EVT_MENU (ID_WirePar_Add, PropertyClassPanel :: OnWireParameterAdd)
  EVT_MENU (ID_WirePar_Edit, PropertyClassPanel :: OnWireParameterEdit)
  EVT_MENU (ID_WirePar_Delete, PropertyClassPanel :: OnWireParameterDel)

  EVT_MENU (ID_Spawn_Add, PropertyClassPanel :: OnSpawnTemplateAdd)
  EVT_MENU (ID_Spawn_Edit, PropertyClassPanel :: OnSpawnTemplateEdit)
  EVT_MENU (ID_Spawn_Delete, PropertyClassPanel :: OnSpawnTemplateDel)
  EVT_CHECKBOX (XRCID("spawnRepeatCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnRandomCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnUniqueCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnNameCounterCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnInhibitText"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnMinDelayText"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnMaxDelayText"), PropertyClassPanel :: OnUpdateEvent)

  EVT_MENU (ID_Quest_Add, PropertyClassPanel :: OnQuestParameterAdd)
  EVT_MENU (ID_Quest_Edit, PropertyClassPanel :: OnQuestParameterEdit)
  EVT_MENU (ID_Quest_Delete, PropertyClassPanel :: OnQuestParameterDel)
  EVT_TEXT_ENTER (XRCID("questText"), PropertyClassPanel :: OnUpdateEvent)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

bool PropertyClassPanel::CheckHitList (const char* listname, bool& hasItem,
    const wxPoint& pos)
{
  wxListCtrl* list = wxStaticCast(FindWindow (
	wxXmlResource::GetXRCID (wxString::FromUTF8 (listname))), wxListCtrl);
  return ListCtrlTools::CheckHitList (list, hasItem, pos);
}

void PropertyClassPanel::OnContextMenu (wxContextMenuEvent& event)
{
  bool hasItem;
  if (CheckHitList ("wireMessageListCtrl", hasItem, event.GetPosition ()))
    OnWireMessageRMB (hasItem);
  else if (CheckHitList ("wireParameterListCtrl", hasItem, event.GetPosition ()))
    OnWireParameterRMB (hasItem);
  else if (CheckHitList ("spawnTemplateListCtrl", hasItem, event.GetPosition ()))
    OnSpawnTemplateRMB (hasItem);
  else if (CheckHitList ("questParameterListCtrl", hasItem, event.GetPosition ()))
    OnQuestParameterRMB (hasItem);
}

void PropertyClassPanel::OnUpdateEvent (wxCommandEvent& event)
{
  printf ("Update!\n"); fflush (stdout);
  UpdatePC ();
}

void PropertyClassPanel::OnChoicebookPageChange (wxChoicebookEvent& event)
{
  UpdatePC ();
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

void PropertyClassPanel::SwitchPCType (const char* pcType)
{
  csString pcTypeS = pcType;
  if (pcTypeS == pctpl->GetName ()) return;
  pctpl->SetName (pcType);
  pctpl->RemoveAllProperties ();
  emode->PCWasEdited (pctpl);
}

// -----------------------------------------------------------------------

bool PropertyClassPanel::UpdateCurrentWireParams ()
{
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

void PropertyClassPanel::OnWireParameterRMB (bool hasItem)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_WirePar_Add, wxT ("&Add"));
  if (hasItem)
  {
    contextMenu.Append(ID_WirePar_Edit, wxT ("&Edit"));
    contextMenu.Append(ID_WirePar_Delete, wxT ("&Delete"));
  }
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassPanel::GetWireParDialog ()
{
  if (!wireParDialog)
  {
    wireParDialog = uiManager->CreateDialog ("Edit parameter");
    wireParDialog->AddRow ();
    wireParDialog->AddLabel ("Name:");
    wireParDialog->AddText ("name");
    wireParDialog->AddChoice ("type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", (const char*)0);
    wireParDialog->AddRow ();
    wireParDialog->AddMultiText ("value");
  }
  return wireParDialog;
}

void PropertyClassPanel::OnWireParameterEdit (wxCommandEvent& event)
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
    UpdatePC ();
  }
}

void PropertyClassPanel::OnWireParameterAdd (wxCommandEvent& event)
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
    UpdatePC ();
  }
}

void PropertyClassPanel::OnWireParameterDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
  UpdateCurrentWireParams ();
  UpdatePC ();
}

void PropertyClassPanel::OnWireMessageRMB (bool hasItem)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_WireMsg_Add, wxT ("&Add"));
  if (hasItem)
  {
    contextMenu.Append(ID_WireMsg_Edit, wxT ("&Edit"));
    contextMenu.Append(ID_WireMsg_Delete, wxT ("&Delete"));
  }
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassPanel::GetWireMsgDialog ()
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

void PropertyClassPanel::OnWireMessageEdit (wxCommandEvent& event)
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
    UpdatePC ();
  }
}

void PropertyClassPanel::OnWireMessageAdd (wxCommandEvent& event)
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
    UpdatePC ();
  }
}

void PropertyClassPanel::OnWireMessageDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
  UpdatePC ();
}

void PropertyClassPanel::OnWireMessageSelected (wxListEvent& event)
{
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

void PropertyClassPanel::OnWireMessageDeselected (wxListEvent& event)
{
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
}

bool PropertyClassPanel::UpdateWire ()
{
  SwitchPCType ("pclogic.wire");

  pctpl->RemoveAllProperties ();//@@@

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
  emode->PCWasEdited (pctpl);
  return true;
}

void PropertyClassPanel::FillWire ()
{
  wxListCtrl* outputList = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  outputList->DeleteAllItems ();
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
  wxTextCtrl* inputMaskText = XRCCTRL (*this, "wireInputMaskText", wxTextCtrl);
  inputMaskText->SetValue (wxT (""));

  if (!pctpl || csString ("pclogic.wire") != pctpl->GetName ()) return;

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

void PropertyClassPanel::OnSpawnTemplateRMB (bool hasItem)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Spawn_Add, wxT ("&Add"));
  if (hasItem)
  {
    contextMenu.Append(ID_Spawn_Edit, wxT ("&Edit"));
    contextMenu.Append(ID_Spawn_Delete, wxT ("&Delete"));
  }
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassPanel::GetSpawnTemplateDialog ()
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

void PropertyClassPanel::OnSpawnTemplateEdit (wxCommandEvent& event)
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
    UpdatePC ();
  }
}

void PropertyClassPanel::OnSpawnTemplateAdd (wxCommandEvent& event)
{
  UIDialog* dialog = GetSpawnTemplateDialog ();
  dialog->Clear ();
  if (dialog->Show (0))
  {
    wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    ListCtrlTools::AddRow (list,
	fields.Get ("name", "").GetData (), 0);
    UpdatePC ();
  }
}

void PropertyClassPanel::OnSpawnTemplateDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  UpdatePC ();
}

bool PropertyClassPanel::UpdateSpawn ()
{
  SwitchPCType ("pclogic.spawn");

  pctpl->RemoveAllProperties (); //@@@

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
  emode->PCWasEdited (pctpl);
  return true;
}

void PropertyClassPanel::FillSpawn ()
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

void PropertyClassPanel::OnQuestStateSelected (wxCommandEvent& event)
{
  UpdatePC ();
}

void PropertyClassPanel::OnQuestParameterRMB (bool hasItem)
{
  wxMenu contextMenu;
  contextMenu.Append(ID_Quest_Add, wxT ("&Add"));
  if (hasItem)
  {
    contextMenu.Append(ID_Quest_Edit, wxT ("&Edit"));
    contextMenu.Append(ID_Quest_Delete, wxT ("&Delete"));
  }
  PopupMenu (&contextMenu);
}

UIDialog* PropertyClassPanel::GetQuestDialog ()
{
  if (!questParDialog)
  {
    questParDialog = uiManager->CreateDialog ("Edit parameter");
    questParDialog->AddRow ();
    questParDialog->AddLabel ("Name:");
    questParDialog->AddText ("name");
    questParDialog->AddChoice ("type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", (const char*)0);
    questParDialog->AddRow ();
    questParDialog->AddMultiText ("value");
  }
  return questParDialog;
}

void PropertyClassPanel::OnQuestParameterEdit (wxCommandEvent& event)
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
    UpdatePC ();
  }
}

void PropertyClassPanel::OnQuestParameterAdd (wxCommandEvent& event)
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
    UpdatePC ();
  }
}

void PropertyClassPanel::OnQuestParameterDel (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
  UpdatePC ();
}

bool PropertyClassPanel::UpdateQuest ()
{
  SwitchPCType ("pclogic.quest");

  pctpl->RemoveAllProperties ();//@@@

  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxListCtrl* parList = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
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

  emode->PCWasEdited (pctpl);
  return true;
}

void PropertyClassPanel::FillQuest ()
{
  wxListCtrl* parList = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
  wxTextCtrl* text = XRCCTRL (*this, "questText", wxTextCtrl);
  text->SetValue (wxT (""));

  if (!pctpl || csString ("pclogic.quest") != pctpl->GetName ()) return;

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
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    states.Add (wxString::FromUTF8 (stateFact->GetName ()));
  }

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

class InventoryRowModel : public RowModel
{
private:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;
  size_t idx;

  void SearchNextAddTemplate ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    while (idx < pctpl->GetPropertyCount ())
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddTemplate")
	return;
      idx++;
    }
  }

public:
  InventoryRowModel (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0), idx (0) { }
  virtual ~InventoryRowModel () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    InventoryRowModel::pctpl = pctpl;
  }

  virtual void ResetIterator ()
  {
    idx = 0;
    SearchNextAddTemplate ();
  }
  virtual bool HasRows () { return idx < pctpl->GetPropertyCount (); }
  virtual csStringArray NextRow ()
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
    idx++;
    SearchNextAddTemplate ();

    iCelPlLayer* pl = pcPanel->GetPL ();
    csStringID nameID = pl->FetchStringID ("name");
    csStringID amountID = pl->FetchStringID ("amount");
    csString parName;
    csString parAmount;
    while (params->HasNext ())
    {
      csStringID parid;
      iParameter* par = params->Next (parid);
      if (parid == nameID) parName = par->GetOriginalExpression ();
      else if (parid == amountID) parAmount = par->GetOriginalExpression ();
    }
    csStringArray ar;
    ar.Push (parName);
    ar.Push (parAmount);
    return ar;
  }

  virtual void StartUpdate ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    pctpl->RemoveProperty (pl->FetchStringID ("AddTemplate"));
  }
  virtual bool UpdateRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      pcPanel->GetUIManager ()->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");
    csStringID actionID = pl->FetchStringID ("AddTemplate");
    csStringID nameID = pl->FetchStringID ("name");
    csStringID amountID = pl->FetchStringID ("amount");
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
    return true;
  }
  virtual void FinishUpdate ()
  {
    pcPanel->GetEntityMode ()->PCWasEdited (pctpl);
  }

  virtual csStringArray GetColumns ()
  {
    csStringArray ar;
    ar.Push ("Name");
    ar.Push ("Amount");
    return ar;
  }
  virtual bool IsEditAllowed () const { return true; }
  virtual UIDialog* GetEditorDialog () { return pcPanel->GetInventoryTemplateDialog (); }
};

UIDialog* PropertyClassPanel::GetInventoryTemplateDialog ()
{
  if (!invTempDialog)
  {
    invTempDialog = uiManager->CreateDialog ("Edit template/amount");
    invTempDialog->AddRow ();
    invTempDialog->AddLabel ("Template:");
    invTempDialog->AddText ("Name");
    invTempDialog->AddText ("Amount");
  }
  return invTempDialog;
}

bool PropertyClassPanel::UpdateInventory ()
{
  SwitchPCType ("pctools.inventory");

  pctpl->RemoveProperty (pl->FetchStringID ("SetLootGenerator"));

  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxTextCtrl* lootText = XRCCTRL (*this, "inventoryLootTextCtrl", wxTextCtrl);
  csString loot = (const char*)lootText->GetValue ().mb_str (wxConvUTF8);
  if (!loot.IsEmpty ())
  {
    ParHash params;
    csRef<iParameter> par = pm->GetParameter (loot, CEL_DATA_STRING);
    if (!par) return false;
    csStringID nameID = pl->FetchStringID ("name");
    params.Put (nameID, par);
    pctpl->PerformAction (pl->FetchStringID ("SetLootGenerator"), params);
  }

  emode->PCWasEdited (pctpl);
  return true;
}

void PropertyClassPanel::FillInventory ()
{
  wxTextCtrl* lootText = XRCCTRL (*this, "inventoryLootTextCtrl", wxTextCtrl);
  lootText->SetValue (wxT (""));

  if (!pctpl || csString ("pctools.inventory") != pctpl->GetName ()) return;

  inventoryModel->SetPC (pctpl);
  inventoryView->Refresh ();

  csString lootName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "SetLootGenerator", "name");
  lootText->SetValue (wxString::FromUTF8 (lootName));
}

// -----------------------------------------------------------------------

class PropertyRowModel : public RowModel
{
private:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;
  size_t idx;

public:
  PropertyRowModel (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0), idx (0) { }
  virtual ~PropertyRowModel () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    PropertyRowModel::pctpl = pctpl;
  }

  virtual void ResetIterator () { idx = 0; }
  virtual bool HasRows () { return idx < pctpl->GetPropertyCount (); }
  virtual csStringArray NextRow ()
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
    idx++;
    csString value;
    celParameterTools::ToString (data, value);
    csStringArray ar;
    ar.Push (pcPanel->GetPL ()->FetchString (id));
    ar.Push (value);
    ar.Push (TypeToString (data.type));
    return ar;
  }

  virtual void StartUpdate ()
  {
    pctpl->RemoveAllProperties ();
  }
  virtual bool UpdateRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
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
      pcPanel->GetUIManager ()->Error ("Unknown type '%s'\n", type.GetData ());
      return false;
    }
    return true;
  }
  virtual void FinishUpdate ()
  {
    pcPanel->GetEntityMode ()->PCWasEdited (pctpl);
  }

  virtual csStringArray GetColumns ()
  {
    csStringArray ar;
    ar.Push ("Name");
    ar.Push ("Value");
    ar.Push ("Type");
    return ar;
  }
  virtual bool IsEditAllowed () const { return true; }
  virtual UIDialog* GetEditorDialog () { return pcPanel->GetPropertyDialog (); }
};

UIDialog* PropertyClassPanel::GetPropertyDialog ()
{
  if (!propDialog)
  {
    propDialog = uiManager->CreateDialog ("Edit property");
    propDialog->AddRow ();
    propDialog->AddLabel ("Name:");
    propDialog->AddText ("Name");
    propDialog->AddChoice ("Type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", (const char*)0);
    propDialog->AddRow ();
    propDialog->AddMultiText ("Value");
  }
  return propDialog;
}

bool PropertyClassPanel::UpdateProperties ()
{
  SwitchPCType ("pctools.properties");
  return true;
}

void PropertyClassPanel::FillProperties ()
{
  if (!pctpl || csString ("pctools.properties") != pctpl->GetName ()) return;

  propertyModel->SetPC (pctpl);
  propertyView->Refresh ();
}

// -----------------------------------------------------------------------

bool PropertyClassPanel::UpdatePC ()
{
  if (!tpl || !pctpl) return true;

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
  if (pc && pc != pctpl)
  {
    uiManager->Error ("Property class with this name and tag already exists!");
    return false;
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

void PropertyClassPanel::SwitchToPC (iCelEntityTemplate* tpl,
    iCelPropertyClassTemplate* pctpl)
{
  PropertyClassPanel::tpl = tpl;
  PropertyClassPanel::pctpl = pctpl;

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

PropertyClassPanel::PropertyClassPanel (wxWindow* parent, UIManager* uiManager,
    EntityMode* emode) :
  uiManager (uiManager), emode (emode), tpl (0), pctpl (0)
{
  pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("PropertyClassPanel"));

  wxListCtrl* list;

  list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  propertyModel.AttachNew (new PropertyRowModel (this));
  propertyView = new ListCtrlView (list, propertyModel);

  list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  inventoryModel.AttachNew (new InventoryRowModel (this));
  inventoryView = new ListCtrlView (list, inventoryModel);

  list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Template", 100);

  list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);
  ListCtrlTools::SetColumn (list, 2, "Type", 100);

  list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Message", 100);
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

PropertyClassPanel::~PropertyClassPanel ()
{
  delete propDialog;
  delete invTempDialog;
  delete spawnTempDialog;
  delete questParDialog;
  delete wireParDialog;
  delete wireMsgDialog;

  delete propertyView;
  delete inventoryView;
}


