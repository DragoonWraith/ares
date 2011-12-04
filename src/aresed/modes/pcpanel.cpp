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
  EVT_CHOICEBOOK_PAGE_CHANGED (XRCID("pcChoicebook"), PropertyClassPanel :: OnChoicebookPageChange)
  EVT_TEXT_ENTER (XRCID("tagTextCtrl"), PropertyClassPanel :: OnUpdateEvent)

  EVT_LIST_ITEM_SELECTED (XRCID("wireMessageListCtrl"), PropertyClassPanel :: OnWireMessageSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("wireMessageListCtrl"), PropertyClassPanel :: OnWireMessageDeselected)

  EVT_CHECKBOX (XRCID("spawnRepeatCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnRandomCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnUniqueCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnNameCounterCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnInhibitText"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnMinDelayText"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnMaxDelayText"), PropertyClassPanel :: OnUpdateEvent)

  EVT_TEXT_ENTER (XRCID("questText"), PropertyClassPanel :: OnUpdateEvent)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

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

class WireMsgRowModel : public RowModel
{
private:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;
  size_t idx;

  void SearchNext ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    while (idx < pctpl->GetPropertyCount ())
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddOutput")
	return;
      idx++;
    }
  }

public:
  WireMsgRowModel (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0), idx (0) { }
  virtual ~WireMsgRowModel () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    WireMsgRowModel::pctpl = pctpl;
  }

  virtual void ResetIterator ()
  {
    idx = 0;
    SearchNext ();
    pcPanel->wireParams.DeleteAll ();
  }
  virtual bool HasRows () { return idx < pctpl->GetPropertyCount (); }
  virtual csStringArray NextRow ()
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
    idx++;
    SearchNext ();

    iCelPlLayer* pl = pcPanel->GetPL ();
    csStringID msgID = pl->FetchStringID ("msgid");
    csStringID entityID = pl->FetchStringID ("entity");
    csString msgName;
    csString entName;
    ParHash paramsHash;
    while (params->HasNext ())
    {
      csStringID parid;
      iParameter* par = params->Next (parid);
      if (parid == msgID) msgName = par->GetOriginalExpression ();
      else if (parid == entityID) entName = par->GetOriginalExpression ();
      else paramsHash.Put (parid, par);
    }
    pcPanel->wireParams.Put (msgName, paramsHash);

    csStringArray ar;
    ar.Push (msgName);
    ar.Push (entName);
    return ar;
  }

  virtual void StartUpdate ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    pctpl->RemoveProperty (pl->FetchStringID ("AddOutput"));
  }
  virtual bool UpdateRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      pcPanel->GetUIManager ()->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");
    csStringID actionID = pl->FetchStringID ("AddOutput");
    csString msg = row[0];
    csString entity = row[1];
    ParHash params;

    csRef<iParameter> par;
    par = pm->GetParameter (msg, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (pl->FetchStringID ("msgid"), par);
    par = pm->GetParameter (entity, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (pl->FetchStringID ("entity"), par);

    const ParHash& wparams = pcPanel->wireParams.Get (msg, ParHash ());
    ParHash::ConstGlobalIterator it = wparams.GetIterator ();
    while (it.HasNext ())
    {
      csStringID parid;
      csRef<iParameter> par = it.Next (parid);
      params.Put (parid, par);
    }

    wxListCtrl* parList = XRCCTRL (*pcPanel, "wireParameterListCtrl", wxListCtrl);
    parList->DeleteAllItems ();

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
    ar.Push ("Message");
    ar.Push ("Entity");
    return ar;
  }
  virtual bool IsEditAllowed () const { return true; }
  virtual UIDialog* GetEditorDialog () { return pcPanel->GetWireMsgDialog (); }
};

class WireParRowModel : public RowModel
{
private:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;
  csString currentMessage;
  ParHash::ConstGlobalIterator it;

public:
  WireParRowModel (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0) { }
  virtual ~WireParRowModel () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    WireParRowModel::pctpl = pctpl;
  }
  void SetCurrentMessage (const char* msg)
  {
    currentMessage = msg;
  }

  virtual void ResetIterator ()
  {
    const ParHash& wparams = pcPanel->wireParams.Get (currentMessage, ParHash ());
    it = wparams.GetIterator ();
  }
  virtual bool HasRows () { return it.HasNext (); }
  virtual csStringArray NextRow ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csStringID parid;
    csRef<iParameter> par = it.Next (parid);

    csString name = pl->FetchString (parid);
    csString val = par->GetOriginalExpression ();
    csString type = TypeToString (par->GetPossibleType ());
    csStringArray ar;
    ar.Push (name);
    ar.Push (val);
    ar.Push (type);
    return ar;
  }

  virtual void StartUpdate ()
  {
    ParHash ph;
    ParHash& wparams = pcPanel->wireParams.Get (currentMessage, ph);
    wparams.DeleteAll ();
  }
  virtual bool UpdateRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      pcPanel->GetUIManager ()->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

    ParHash ph;
    ParHash& wparams = pcPanel->wireParams.Get (currentMessage, ph);
    csString name = row[0];
    csString value = row[1];
    csString type = row[2];
    csRef<iParameter> par = pm->GetParameter (value, StringToType (type));
    if (!par) return false;
    wparams.Put (pl->FetchStringID (name), par);
    return true;
  }
  virtual void FinishUpdate ()
  {
    pcPanel->UpdateWireMsg ();
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
  virtual UIDialog* GetEditorDialog () { return pcPanel->GetWireParDialog (); }
};

UIDialog* PropertyClassPanel::GetWireParDialog ()
{
  if (!wireParDialog)
  {
    wireParDialog = uiManager->CreateDialog ("Edit parameter");
    wireParDialog->AddRow ();
    wireParDialog->AddLabel ("Name:");
    wireParDialog->AddText ("Name");
    wireParDialog->AddChoice ("Type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", (const char*)0);
    wireParDialog->AddRow ();
    wireParDialog->AddMultiText ("Value");
  }
  return wireParDialog;
}

UIDialog* PropertyClassPanel::GetWireMsgDialog ()
{
  if (!wireMsgDialog)
  {
    wireMsgDialog = uiManager->CreateDialog ("Edit message");
    wireMsgDialog->AddRow ();
    wireMsgDialog->AddLabel ("Message:");
    wireMsgDialog->AddText ("Message");
    wireMsgDialog->AddRow ();
    wireMsgDialog->AddLabel ("Entity:");
    wireMsgDialog->AddText ("Entity");
    wireMsgDialog->AddButton ("...");	// @@@ Not implemented yet.
  }
  return wireMsgDialog;
}

void PropertyClassPanel::OnWireMessageSelected (wxListEvent& event)
{
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();

  long idx = event.GetIndex ();
  wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  csString msg = row[0];

  wireParModel->SetCurrentMessage (msg);
  wireParView->Refresh ();
}

void PropertyClassPanel::OnWireMessageDeselected (wxListEvent& event)
{
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
}

void PropertyClassPanel::UpdateWireMsg ()
{
  wireMsgView->Update ();
  wireParView->Refresh ();
}

bool PropertyClassPanel::UpdateWire ()
{
  SwitchPCType ("pclogic.wire");

  pctpl->RemoveProperty (pl->FetchStringID ("AddInput"));

  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxTextCtrl* inputMaskText = XRCCTRL (*this, "wireInputMaskText", wxTextCtrl);

  {
    ParHash params;
    csString mask = (const char*)inputMaskText->GetValue ().mb_str (wxConvUTF8);
    csRef<iParameter> par = pm->GetParameter (mask, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (pl->FetchStringID ("mask"), par);
    pctpl->PerformAction (pl->FetchStringID ("AddInput"), params);
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

  wireMsgModel->SetPC (pctpl);
  wireMsgView->Refresh ();

  wireParModel->SetCurrentMessage ("");
  wireParModel->SetPC (pctpl);
  wireParView->Refresh ();
}

// -----------------------------------------------------------------------

class SpawnRowModel : public RowModel
{
private:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;
  size_t idx;

  void SearchNext ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    while (idx < pctpl->GetPropertyCount ())
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddEntityTemplateType")
	return;
      idx++;
    }
  }

public:
  SpawnRowModel (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0), idx (0) { }
  virtual ~SpawnRowModel () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    SpawnRowModel::pctpl = pctpl;
  }

  virtual void ResetIterator ()
  {
    idx = 0;
    SearchNext ();
  }
  virtual bool HasRows () { return idx < pctpl->GetPropertyCount (); }
  virtual csStringArray NextRow ()
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
    idx++;
    SearchNext ();

    iCelPlLayer* pl = pcPanel->GetPL ();
    csStringID nameID = pl->FetchStringID ("template");
    csString parName;
    while (params->HasNext ())
    {
      csStringID parid;
      iParameter* par = params->Next (parid);
      if (parid == nameID) parName = par->GetOriginalExpression ();
    }
    csStringArray ar;
    ar.Push (parName);
    return ar;
  }

  virtual void StartUpdate ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    pctpl->RemoveProperty (pl->FetchStringID ("AddEntityTemplateType"));
  }
  virtual bool UpdateRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      pcPanel->GetUIManager ()->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");
    csStringID actionID = pl->FetchStringID ("AddEntityTemplateType");
    csStringID nameID = pl->FetchStringID ("template");
    csString name = row[0];
    ParHash params;

    csRef<iParameter> par;
    par = pm->GetParameter (name, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (nameID, par);

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
    return ar;
  }
  virtual bool IsEditAllowed () const { return true; }
  virtual UIDialog* GetEditorDialog () { return pcPanel->GetSpawnTemplateDialog (); }
};

UIDialog* PropertyClassPanel::GetSpawnTemplateDialog ()
{
  if (!spawnTempDialog)
  {
    spawnTempDialog = uiManager->CreateDialog ("Edit template");
    spawnTempDialog->AddRow ();
    spawnTempDialog->AddLabel ("Template:");
    spawnTempDialog->AddText ("Name");
  }
  return spawnTempDialog;
}

bool PropertyClassPanel::UpdateSpawn ()
{
  SwitchPCType ("pclogic.spawn");

  pctpl->RemoveProperty (pl->FetchStringID ("SetTiming"));
  pctpl->RemoveProperty (pl->FetchStringID ("Inhibit"));
  pctpl->RemoveProperty (pl->FetchStringID ("spawnunique"));
  pctpl->RemoveProperty (pl->FetchStringID ("namecounter"));

  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      uiManager->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

  wxCheckBox* repeatCB = XRCCTRL (*this, "spawnRepeatCheckBox", wxCheckBox);
  wxCheckBox* randomCB = XRCCTRL (*this, "spawnRandomCheckBox", wxCheckBox);
  wxCheckBox* spawnuniqueCB = XRCCTRL (*this, "spawnUniqueCheckBox", wxCheckBox);
  wxCheckBox* namecounterCB = XRCCTRL (*this, "spawnNameCounterCheckBox", wxCheckBox);
  wxTextCtrl* minDelayText = XRCCTRL (*this, "spawnMinDelayText", wxTextCtrl);
  wxTextCtrl* maxDelayText = XRCCTRL (*this, "spawnMaxDelayText", wxTextCtrl);
  wxTextCtrl* inhibitText = XRCCTRL (*this, "spawnInhibitText", wxTextCtrl);

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

  spawnModel->SetPC (pctpl);
  spawnView->Refresh ();
}

// -----------------------------------------------------------------------

class QuestRowModel : public RowModel
{
private:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;

  csRef<iCelParameterIterator> it;
  csStringID nextID;
  iParameter* nextPar;

  ParHash params;
  csString newQuestName;	// If set this will overwrite the quest name.

  void SearchNext ()
  {
    nextID = csInvalidStringID;
    while (it->HasNext ())
    {
      nextPar = it->Next (nextID);
      iCelPlLayer* pl = pcPanel->GetPL ();
      csString name = pl->FetchString (nextID);
      if (name != "name") return;
      nextID = csInvalidStringID;
    }
  }

public:
  QuestRowModel (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0),
    nextID (csInvalidStringID) { }
  virtual ~QuestRowModel () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    QuestRowModel::pctpl = pctpl;
  }

  virtual void ResetIterator ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    size_t nqIdx = pctpl->FindProperty (pl->FetchStringID ("NewQuest"));
    if (nqIdx != csArrayItemNotFound)
    {
      csStringID id;
      celData data;
      it = pctpl->GetProperty (nqIdx, id, data);
      SearchNext ();
    }
    else
      it = 0;
  }
  virtual bool HasRows () { return nextID != csInvalidStringID; }
  virtual csStringArray NextRow ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csString name = pl->FetchString (nextID);
    csString val = nextPar->GetOriginalExpression ();
    csString type = TypeToString (nextPar->GetPossibleType ());
    csStringArray ar;
    ar.Push (name);
    ar.Push (val);
    ar.Push (type);
    SearchNext ();
    return ar;
  }

  void OverrideQuestName (const char* newname)
  {
    newQuestName = newname;
  }
  virtual void StartUpdate ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csString questName;
    if (newQuestName.IsEmpty ())
      questName = InspectTools::GetActionParameterValueString (pl, pctpl,
        "NewQuest", "name");
    else
      questName = newQuestName;
    newQuestName = "";
    pctpl->RemoveProperty (pl->FetchStringID ("NewQuest"));
    csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      pcPanel->GetUIManager ()->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");
    params.DeleteAll ();
    csRef<iParameter> par = pm->GetParameter (questName, CEL_DATA_STRING);
    params.Put (pl->FetchStringID ("name"), par);
  }
  virtual bool UpdateRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      pcPanel->GetUIManager ()->GetApp ()->GetObjectRegistry (), "cel.parameters.manager");

    csStringID nameID = pl->FetchStringID (row[0]);
    csRef<iParameter> par = pm->GetParameter (row[1], StringToType (row[2]));
    if (!par) return false;
    params.Put (nameID, par);
    return true;
  }
  virtual void FinishUpdate ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    pctpl->PerformAction (pl->FetchStringID ("NewQuest"), params);
    pcPanel->GetEntityMode ()->PCWasEdited (pctpl);
  }

  virtual csStringArray GetColumns ()
  {
    csStringArray ar;
    ar.Push ("Name");
    ar.Push ("Type");
    ar.Push ("Value");
    return ar;
  }
  virtual bool IsEditAllowed () const { return true; }
  virtual UIDialog* GetEditorDialog () { return pcPanel->GetQuestDialog (); }
};

UIDialog* PropertyClassPanel::GetQuestDialog ()
{
  if (!questParDialog)
  {
    questParDialog = uiManager->CreateDialog ("Edit parameter");
    questParDialog->AddRow ();
    questParDialog->AddLabel ("Name:");
    questParDialog->AddText ("Name");
    questParDialog->AddChoice ("Type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", (const char*)0);
    questParDialog->AddRow ();
    questParDialog->AddMultiText ("Value");
  }
  return questParDialog;
}

bool PropertyClassPanel::UpdateQuest ()
{
  SwitchPCType ("pclogic.quest");

  wxTextCtrl* questText = XRCCTRL (*this, "questText", wxTextCtrl);
  csString questName = (const char*)questText->GetValue ().mb_str (wxConvUTF8);
  if (questName.IsEmpty ())
  {
    uiManager->Error ("Empty quest name is not allowed!");
    return false;
  }

  questModel->OverrideQuestName (questName);
  questView->Update ();
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

  // Fill all states and mark the default state.
  wxArrayString states;
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    states.Add (wxString::FromUTF8 (stateFact->GetName ()));
  }

  questModel->SetPC (pctpl);
  questView->Refresh ();
}

// -----------------------------------------------------------------------

class InventoryRowModel : public RowModel
{
private:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;
  size_t idx;

  void SearchNext ()
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
    SearchNext ();
  }
  virtual bool HasRows () { return idx < pctpl->GetPropertyCount (); }
  virtual csStringArray NextRow ()
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
    idx++;
    SearchNext ();

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
  wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  list->DeleteAllItems ();
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
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  list->DeleteAllItems ();

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
  spawnModel.AttachNew (new SpawnRowModel (this));
  spawnView = new ListCtrlView (list, spawnModel);

  list = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  questModel.AttachNew (new QuestRowModel (this));
  questView = new ListCtrlView (list, questModel);

  list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  wireMsgModel.AttachNew (new WireMsgRowModel (this));
  wireMsgView = new ListCtrlView (list, wireMsgModel);

  list = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  wireParModel.AttachNew (new WireParRowModel (this));
  wireParView = new ListCtrlView (list, wireParModel);
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
  delete questView;
  delete spawnView;
  delete wireMsgView;
  delete wireParView;
}


