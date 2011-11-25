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

  EVT_BUTTON (XRCID("propertyAddButton"), PropertyClassDialog :: OnPropertyAdd)
  EVT_BUTTON (XRCID("propertyDeleteButton"), PropertyClassDialog :: OnPropertyDel)
  EVT_LIST_ITEM_SELECTED (XRCID("propertyListCtrl"), PropertyClassDialog :: OnPropertySelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("propertyListCtrl"), PropertyClassDialog :: OnPropertyDeselected)

  EVT_BUTTON (XRCID("wireMessageAddButton"), PropertyClassDialog :: OnWireMessageAdd)
  EVT_BUTTON (XRCID("wireMessageDeleteButton"), PropertyClassDialog :: OnWireMessageDel)
  EVT_LIST_ITEM_SELECTED (XRCID("wireMessageListCtrl"), PropertyClassDialog :: OnWireMessageSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("wireMessageListCtrl"), PropertyClassDialog :: OnWireMessageDeselected)

  EVT_BUTTON (XRCID("wireParameterAddButton"), PropertyClassDialog :: OnWireParameterAdd)
  EVT_BUTTON (XRCID("wireParameterDeleteButton"), PropertyClassDialog :: OnWireParameterDel)
  EVT_LIST_ITEM_SELECTED (XRCID("wireParameterListCtrl"), PropertyClassDialog :: OnWireParameterSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("wireParameterListCtrl"), PropertyClassDialog :: OnWireParameterDeselected)

  EVT_BUTTON (XRCID("spawnTemplateAddButton"), PropertyClassDialog :: OnSpawnTemplateAdd)
  EVT_BUTTON (XRCID("spawnTemplateDeleteButton"), PropertyClassDialog :: OnSpawnTemplateDel)
  EVT_LIST_ITEM_SELECTED (XRCID("spawnTemplateListCtrl"), PropertyClassDialog :: OnSpawnTemplateSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("spawnTemplateListCtrl"), PropertyClassDialog :: OnSpawnTemplateDeselected)

  EVT_BUTTON (XRCID("questParameterAddButton"), PropertyClassDialog :: OnQuestParameterAdd)
  EVT_BUTTON (XRCID("questParameterDeleteButton"), PropertyClassDialog :: OnQuestParameterDel)
  EVT_LIST_ITEM_SELECTED (XRCID("questParameterListCtrl"), PropertyClassDialog :: OnQuestParameterSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("questParameterListCtrl"), PropertyClassDialog :: OnQuestParameterDeselected)

  EVT_BUTTON (XRCID("inventoryTemplateAddButton"), PropertyClassDialog :: OnInventoryTemplateAdd)
  EVT_BUTTON (XRCID("inventoryTemplateDeleteButton"), PropertyClassDialog :: OnInventoryTemplateDel)
  EVT_LIST_ITEM_SELECTED (XRCID("inventoryTemplateListCtrl"), PropertyClassDialog :: OnInvTemplateSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("inventoryTemplateListCtrl"), PropertyClassDialog :: OnInvTemplateDeselected)
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
  wxString iname (name, wxConvUTF8);
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
      wxString (nameComp, wxConvUTF8));
  wxTextCtrl* valueText = 0;
  if (valueComp) valueText = (wxTextCtrl*)FindWindowByName (
      wxString (valueComp, wxConvUTF8));
  wxChoice* typeChoice = 0;
  if (typeComp) typeChoice = (wxChoice*)FindWindowByName (
      wxString (typeComp, wxConvUTF8));
  wxListCtrl* list = (wxListCtrl*)FindWindowByName (
      wxString (listComp, wxConvUTF8));

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
      wxString (delComp, wxConvUTF8));
  delButton->Enable ();

  wxTextCtrl* nameText = (wxTextCtrl*)FindWindowByName (
      wxString (nameComp, wxConvUTF8));
  wxTextCtrl* valueText = 0;
  if (valueComp) valueText = (wxTextCtrl*)FindWindowByName (
      wxString (valueComp, wxConvUTF8));
  wxChoice* typeChoice = 0;
  if (typeComp) typeChoice = (wxChoice*)FindWindowByName (
      wxString (typeComp, wxConvUTF8));
  wxListCtrl* list = (wxListCtrl*)FindWindowByName (
      wxString (listComp, wxConvUTF8));

  csStringArray row = ListCtrlTools::ReadRow (list, selIndex);
  nameText->SetValue (wxString (row[0], wxConvUTF8));
  if (valueComp)
    valueText->SetValue (wxString (row[1], wxConvUTF8));
  if (typeComp)
    typeChoice->SetStringSelection (wxString (row[2], wxConvUTF8));
}

// -----------------------------------------------------------------------

void PropertyClassDialog::UpdateCurrentWireParams ()
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
    params.Put (pl->FetchStringID (name), pm->GetParameter (value, StringToType (type)));
  }
}

void PropertyClassDialog::OnWireParameterSelected (wxListEvent& event)
{
  wireMsgSelIndex = event.GetIndex ();
  FillFieldsFromRow ("wireParameterListCtrl", "wireParameterNameText",
      "wireParameterValueText", "wireParameterTypeChoice",
      "wireParameterDeleteButton", wireParSelIndex);
}

void PropertyClassDialog::OnWireParameterDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "wireParameterDeleteButton", wxButton);
  delButton->Disable ();
}

void PropertyClassDialog::OnWireParameterAdd (wxCommandEvent& event)
{
  AddRowFromInput ("wireParameterListCtrl", "wireParameterNameText",
      "wireParameterValueText", "wireParameterTypeChoice");
  UpdateCurrentWireParams ();
}

void PropertyClassDialog::OnWireParameterDel (wxCommandEvent& event)
{
  if (wireParSelIndex == -1) return;
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteItem (wireParSelIndex);
  parList->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  parList->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  parList->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
  wireParSelIndex = -1;
  UpdateCurrentWireParams ();
}

void PropertyClassDialog::OnWireMessageSelected (wxListEvent& event)
{
  wireMsgSelIndex = event.GetIndex ();
  FillFieldsFromRow ("wireMessageListCtrl", "wireMessageText",
      "wireEntityText", 0,
      "wireMessageDeleteButton", wireMsgSelIndex);

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
  wxTextCtrl* msgText = XRCCTRL (*this, "wireMessageText", wxTextCtrl);
  csString msg = (const char*)msgText->GetValue ().mb_str (wxConvUTF8);
  const ParHash& params = wireParams.Get (msg, ParHash ());
  ParHash::ConstGlobalIterator it = params.GetIterator ();
  while (it.HasNext ())
  {
    csStringID parid;
    csRef<iParameter> par = it.Next (parid);

    // @@@ No support for expressions for now.
    csString name = pl->FetchString (parid);
    if (name == "msgid") continue;	// Ignore this one.
    if (name == "entity") continue;	// Ignore this one.
    const celData* parData = par->GetData (0);
    csString val = par->Get (0);
    csString type = TypeToString (parData->type);
    ListCtrlTools::AddRow (parList, name.GetData (), val.GetData (), type.GetData (), 0);
  }
}

void PropertyClassDialog::OnWireMessageDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "wireMessageDeleteButton", wxButton);
  delButton->Disable ();

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
}

void PropertyClassDialog::OnWireMessageAdd (wxCommandEvent& event)
{
  AddRowFromInput ("wireMessageListCtrl", "wireMessageText", "wireEntityText", 0);

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
}

void PropertyClassDialog::OnWireMessageDel (wxCommandEvent& event)
{
  if (wireMsgSelIndex == -1) return;
  wxListCtrl* msgList = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  msgList->DeleteItem (wireMsgSelIndex);
  msgList->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  msgList->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  wireMsgSelIndex = -1;

  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
}

void PropertyClassDialog::UpdateWire ()
{
  pctpl->SetName ("pclogic.wire");
  pctpl->RemoveAllProperties ();

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxListCtrl* outputList = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  wxTextCtrl* inputMaskText = XRCCTRL (*this, "wireInputMaskText", wxTextCtrl);

  {
    ParHash params;
    csString mask = (const char*)inputMaskText->GetValue ().mb_str (wxConvUTF8);
    params.Put (pl->FetchStringID ("mask"), pm->GetParameter (mask, CEL_DATA_STRING));
    pctpl->PerformAction (pl->FetchStringID ("AddInput"), params);
  }

  for (int r = 0 ; r < outputList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (outputList, r);
    csString message = row[0];
    csString entity = row[1];
    ParHash params;
    params.Put (pl->FetchStringID ("msgid"), pm->GetParameter (message, CEL_DATA_STRING));
    params.Put (pl->FetchStringID ("entity"), pm->GetParameter (entity, CEL_DATA_STRING));
    const ParHash& wparams = wireParams.Get (message, ParHash ());
    ParHash::ConstGlobalIterator it = wparams.GetIterator ();
    while (it.HasNext ())
    {
      csStringID parid;
      csRef<iParameter> par = it.Next (parid);

      // @@@ No support for expressions for now.
      params.Put (parid, par);
    }

    pctpl->PerformAction (pl->FetchStringID ("AddOutput"), params);
  }

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
  inputMaskText->SetValue (wxString (inputMask, wxConvUTF8));

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
	// @@@ No support for expressions.
	if (parid == msgID) msgName = par->Get (0);
	else if (parid == entityID) entName = par->Get (0);
	paramsHash.Put (parid, par);
      }
      wireParams.Put (msgName, paramsHash);
      ListCtrlTools::AddRow (outputList, msgName.GetData (), entName.GetData (), 0);
    }
  }
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnSpawnTemplateSelected (wxListEvent& event)
{
  spawnSelIndex = event.GetIndex ();
  FillFieldsFromRow ("spawnTemplateListCtrl", "spawnTemplateNameText",
      0, 0,
      "spawnTemplateDeleteButton", spawnSelIndex);
}

void PropertyClassDialog::OnSpawnTemplateDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "spawnTemplateDeleteButton", wxButton);
  delButton->Disable ();
}

void PropertyClassDialog::OnSpawnTemplateAdd (wxCommandEvent& event)
{
  AddRowFromInput ("spawnTemplateListCtrl", "spawnTemplateNameText", 0, 0);
}

void PropertyClassDialog::OnSpawnTemplateDel (wxCommandEvent& event)
{
  if (spawnSelIndex == -1) return;
  wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  list->DeleteItem (spawnSelIndex);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  spawnSelIndex = -1;
}

void PropertyClassDialog::UpdateSpawn ()
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
    params.Put (nameID, pm->GetParameter (name, CEL_DATA_STRING));
    pctpl->PerformAction (actionID, params);
  }

  {
    csString mindelay = (const char*)minDelayText->GetValue ().mb_str (wxConvUTF8);
    csString maxdelay = (const char*)maxDelayText->GetValue ().mb_str (wxConvUTF8);
    ParHash params;
    params.Put (pl->FetchStringID ("repeat"), pm->GetParameter (repeatCB->IsChecked () ? "true" : "false", CEL_DATA_BOOL));
    params.Put (pl->FetchStringID ("random"), pm->GetParameter (randomCB->IsChecked () ? "true" : "false", CEL_DATA_BOOL));
    params.Put (pl->FetchStringID ("mindelay"), pm->GetParameter (mindelay, CEL_DATA_LONG));
    params.Put (pl->FetchStringID ("maxdelay"), pm->GetParameter (maxdelay, CEL_DATA_LONG));
    pctpl->PerformAction (pl->FetchStringID ("SetTiming"), params);
  }

  {
    csString inhibit = (const char*)inhibitText->GetValue ().mb_str (wxConvUTF8);
    ParHash params;
    params.Put (pl->FetchStringID ("count"), pm->GetParameter (inhibit, CEL_DATA_LONG));
    pctpl->PerformAction (pl->FetchStringID ("Inhibit"), params);
  }

  pctpl->SetProperty (pl->FetchStringID ("spawnunique"), spawnuniqueCB->IsChecked ());
  pctpl->SetProperty (pl->FetchStringID ("namecounter"), namecounterCB->IsChecked ());

  // @@@ TODO AddSpawnPosition
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
    minDelayText->SetValue (wxString (s, wxConvUTF8));
  }
  long maxdelay = InspectTools::GetActionParameterValueLong (pl, pctpl,
      "SetTiming", "maxdelay", &valid);
  if (valid)
  {
    csString s; s.Format ("%ld", maxdelay);
    maxDelayText->SetValue (wxString (s, wxConvUTF8));
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
    inhibitText->SetValue (wxString (s, wxConvUTF8));
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
	// @@@ No support for expressions.
	if (parid == tplID) tplName = par->Get (0);
      }
      if (!tplName.IsEmpty ())
        ListCtrlTools::AddRow (list, tplName.GetData (), 0);
    }
  }
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnQuestParameterSelected (wxListEvent& event)
{
  questSelIndex = event.GetIndex ();
  FillFieldsFromRow ("questParameterListCtrl", "questParameterNameText",
      "questParameterValueText", "questParameterTypeChoice",
      "questParameterDeleteButton", questSelIndex);
}

void PropertyClassDialog::OnQuestParameterDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "questParameterDeleteButton", wxButton);
  delButton->Disable ();
}

void PropertyClassDialog::OnQuestParameterAdd (wxCommandEvent& event)
{
  AddRowFromInput ("questParameterListCtrl", "questParameterNameText",
      "questParameterValueText", "questParameterTypeChoice");
}

void PropertyClassDialog::OnQuestParameterDel (wxCommandEvent& event)
{
  if (questSelIndex == -1) return;
  wxListCtrl* list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  list->DeleteItem (questSelIndex);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
  questSelIndex = -1;
}

void PropertyClassDialog::UpdateQuest ()
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
  if (questName.IsEmpty ()) return;

  ParHash params;
  params.Put (pl->FetchStringID ("name"), pm->GetParameter (questName, CEL_DATA_STRING));
  for (int r = 0 ; r < parList->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (parList, r);
    csString name = row[0];
    csString value = row[1];
    csString type = row[2];
    csStringID nameID = pl->FetchStringID (name);
    params.Put (nameID, pm->GetParameter (value, StringToType (type)));
  }
  pctpl->PerformAction (pl->FetchStringID ("NewQuest"), params);

  csString state = (const char*)stateList->GetStringSelection ().mb_str (wxConvUTF8);
  if (!state.IsEmpty ())
    pctpl->SetProperty (pl->FetchStringID ("state"), state.GetData ());
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

  text->SetValue (wxString (questName, wxConvUTF8));

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
    states.Add (wxString (stateFact->GetName (), wxConvUTF8));
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
      // @@@ No support for expressions for now.
      csString name = pl->FetchString (parid);
      if (name == "name") continue;	// Ignore this one.
      const celData* parData = par->GetData (0);
      csString val = par->Get (0);
      csString type = TypeToString (parData->type);
      ListCtrlTools::AddRow (parList, name.GetData (), val.GetData (), type.GetData (), 0);
    }
  }
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnInvTemplateSelected (wxListEvent& event)
{
  invSelIndex = event.GetIndex ();
  FillFieldsFromRow ("inventoryTemplateListCtrl", "inventoryTemplateNameText",
      "inventoryTemplateValueText", 0,
      "inventoryTemplateDeleteButton", invSelIndex);
}

void PropertyClassDialog::OnInvTemplateDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "inventoryTemplateDeleteButton", wxButton);
  delButton->Disable ();
}

void PropertyClassDialog::OnInventoryTemplateAdd (wxCommandEvent& event)
{
  AddRowFromInput ("inventoryTemplateListCtrl", "inventoryTemplateNameText",
      "inventoryTemplateAmountText", 0);
}

void PropertyClassDialog::OnInventoryTemplateDel (wxCommandEvent& event)
{
  if (invSelIndex == -1) return;
  wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  list->DeleteItem (invSelIndex);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  invSelIndex = -1;
}

void PropertyClassDialog::UpdateInventory ()
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
    params.Put (nameID, pm->GetParameter (name, CEL_DATA_STRING));
    params.Put (amountID, pm->GetParameter (amount, CEL_DATA_LONG));
    pctpl->PerformAction (actionID, params);
  }

  wxTextCtrl* lootText = XRCCTRL (*this, "inventoryLootTextCtrl", wxTextCtrl);
  csString loot = (const char*)lootText->GetValue ().mb_str (wxConvUTF8);
  if (!loot.IsEmpty ())
  {
    ParHash params;
    params.Put (nameID, pm->GetParameter (loot, CEL_DATA_STRING));
    pctpl->PerformAction (pl->FetchStringID ("SetLootGenerator"),
	params);
  }
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
	// @@@ No support for expressions.
	if (parid == nameID) parName = par->Get (0);
	else if (parid == amountID) parAmount = par->Get (0);
      }
      ListCtrlTools::AddRow (list, parName.GetData (), parAmount.GetData (), 0);
    }
  }

  csString lootName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "SetLootGenerator", "name");
  lootText->SetValue (wxString (lootName, wxConvUTF8));
}

// -----------------------------------------------------------------------

void PropertyClassDialog::OnPropertySelected (wxListEvent& event)
{
  propSelIndex = event.GetIndex ();
  FillFieldsFromRow ("propertyListCtrl", "propertyNameText",
      "propertyValueText", "propertyTypeChoice",
      "propertyDeleteButton", propSelIndex);
}

void PropertyClassDialog::OnPropertyDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "propertyDeleteButton", wxButton);
  delButton->Disable ();
}

void PropertyClassDialog::OnPropertyAdd (wxCommandEvent& event)
{
  AddRowFromInput ("propertyListCtrl", "propertyNameText",
      "propertyValueText", "propertyTypeChoice");
}

void PropertyClassDialog::OnPropertyDel (wxCommandEvent& event)
{
  if (propSelIndex == -1) return;
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  list->DeleteItem (propSelIndex);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
  propSelIndex = -1;
}

void PropertyClassDialog::UpdateProperties ()
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
    else printf ("Unknown type '%s'\n", type.GetData ());
  }
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

  if (page == "pctools.properties") UpdateProperties ();
  else if (page == "pctools.inventory") UpdateInventory ();
  else if (page == "pclogic.wire") UpdateWire ();
  else if (page == "pclogic.quest") UpdateQuest ();
  else if (page == "pclogic.spawn") UpdateSpawn ();
  else printf ("Unknown page '%s'\n", page.GetData ());
  return true;
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
    text->SetValue (wxString (tagName, wxConvUTF8));
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
  wxButton* button;

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

  button = XRCCTRL (*this, "propertyDeleteButton", wxButton);
  button->Disable ();
  button = XRCCTRL (*this, "inventoryTemplateDeleteButton", wxButton);
  button->Disable ();
  button = XRCCTRL (*this, "questParameterDeleteButton", wxButton);
  button->Disable ();
  button = XRCCTRL (*this, "spawnTemplateDeleteButton", wxButton);
  button->Disable ();
  button = XRCCTRL (*this, "wireMessageDeleteButton", wxButton);
  button->Disable ();
  button = XRCCTRL (*this, "wireParameterDeleteButton", wxButton);
  button->Disable ();

  propSelIndex = -1;
  questSelIndex = -1;
  invSelIndex = -1;
  spawnSelIndex = -1;
  wireMsgSelIndex = -1;
  wireParSelIndex = -1;
}

PropertyClassDialog::~PropertyClassDialog ()
{
}


