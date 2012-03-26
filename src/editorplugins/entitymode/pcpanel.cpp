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
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "edcommon/listctrltools.h"
#include "edcommon/listview.h"
#include "edcommon/uitools.h"
#include "edcommon/rowmodel.h"
#include "edcommon/inspect.h"
#include "edcommon/tools.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/iconfig.h"
#include "editor/iapp.h"

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

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PropertyClassPanel, wxPanel)
  EVT_CHOICEBOOK_PAGE_CHANGED (XRCID("pcChoicebook"), PropertyClassPanel :: OnChoicebookPageChange)
  EVT_TEXT_ENTER (XRCID("tagTextCtrl"), PropertyClassPanel :: OnUpdateEvent)

  EVT_CHECKBOX (XRCID("spawnRepeatCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnRandomCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnUniqueCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_CHECKBOX (XRCID("spawnNameCounterCheckBox"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnInhibitText"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnMinDelayText"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("spawnMaxDelayText"), PropertyClassPanel :: OnUpdateEvent)

  EVT_TEXT_ENTER (XRCID("questText"), PropertyClassPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("wireInputMaskCombo"), PropertyClassPanel :: OnUpdateEvent)
  EVT_COMBOBOX (XRCID("wireInputMaskCombo"), PropertyClassPanel :: OnUpdateEvent)

  EVT_CHOICE (XRCID("modeChoice"), PropertyClassPanel :: OnUpdateEvent)
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

void PropertyClassPanel::SwitchPCType (const char* pcType)
{
  csString pcTypeS = pcType;
  if (pcTypeS == pctpl->GetName ()) return;
  pctpl->SetName (pcType);
  pctpl->RemoveAllProperties ();
  emode->PCWasEdited (pctpl);
}

// -----------------------------------------------------------------------

using namespace Ares;

class PcParCollectionValue : public StandardCollectionValue
{
protected:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;

public:
  PcParCollectionValue (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0) { }
  virtual ~PcParCollectionValue () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    PcParCollectionValue::pctpl = pctpl;
  }
};

class PcParRowModel : public RowModel
{
protected:
  PropertyClassPanel* pcPanel;
  iCelPropertyClassTemplate* pctpl;

public:
  PcParRowModel (PropertyClassPanel* pcPanel) : pcPanel (pcPanel), pctpl (0) { }
  virtual ~PcParRowModel () { }

  void SetPC (iCelPropertyClassTemplate* pctpl)
  {
    PcParRowModel::pctpl = pctpl;
  }

  virtual void FinishUpdate ()
  {
    pcPanel->GetEntityMode ()->PCWasEdited (pctpl);
  }
};

class WireMsgRowModel : public PcParRowModel
{
private:
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
  WireMsgRowModel (PropertyClassPanel* pcPanel) : PcParRowModel (pcPanel), idx (0) { }
  virtual ~WireMsgRowModel () { }

  /**
   * Convert the parameters to a string that can be used in the list.
   */
  csString GetParameterString (iCelParameterIterator* it,
      csString& msg, csString& entity)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csStringID msgID = pl->FetchStringID ("msgid");
    csStringID entityID = pl->FetchStringID ("entity");

    csString params;
    while (it->HasNext ())
    {
      csStringID parid;
      iParameter* par = it->Next (parid);
      if (parid == msgID) msg = par->GetOriginalExpression ();
      else if (parid == entityID) entity = par->GetOriginalExpression ();
      else
      {
	params.AppendFmt ("%s=%s", pl->FetchString (parid),
	    par->GetOriginalExpression ());
	if (it->HasNext ()) params += ',';
      }
    }
    return params;
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

    csString msgName;
    csString entName;
    csString pars = GetParameterString (params, msgName, entName);

    idx++;
    SearchNext ();

    return Tools::MakeArray (msgName.GetData (), entName.GetData (),
	pars.GetData (), (const char*)0);
  }

  virtual bool UpdateRow (const csStringArray& oldRow, const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    iParameterManager* pm = pcPanel->GetPM ();
    csStringID outputID = pl->FetchStringID ("AddOutput");
    for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> it = pctpl->GetProperty (i, id, data);
      if (id == outputID)
      {
        csString msgName;
        csString entName;
	csString pars = GetParameterString (it, msgName, entName);
	if (msgName == oldRow[0] && entName == oldRow[1] && pars == oldRow[2])
	{
	  csRef<iParameter> par;
	  InspectTools::DeleteActionParameter (pl, pctpl, i, "msgid");
	  par = pm->GetParameter (row[0], CEL_DATA_STRING);
	  InspectTools::AddActionParameter (pl, pctpl, i, "msgid", par);

	  InspectTools::DeleteActionParameter (pl, pctpl, i, "entity");
	  par = pm->GetParameter (row[1], CEL_DATA_STRING);
	  InspectTools::AddActionParameter (pl, pctpl, i, "entity", par);
	  return true;
	}
      }
    }
    return false;
  }

  virtual bool DeleteRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csStringID outputID = pl->FetchStringID ("AddOutput");
    for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> it = pctpl->GetProperty (i, id, data);
      if (id == outputID)
      {
        csString msgName;
        csString entName;
	csString pars = GetParameterString (it, msgName, entName);
	if (msgName == row[0] && entName == row[1] && pars == row[2])
	{
	  pctpl->RemovePropertyByIndex (i);
	  return true;
	}
      }
    }
    return false;
  }

  virtual bool AddRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    iParameterManager* pm = pcPanel->GetPM ();
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

    iEditorConfig* config = pcPanel->GetEntityMode ()->GetApplication ()->GetConfig ();
    const KnownMessage* message = config->GetKnownMessage (msg);
    if (message)
    {
      for (size_t i = 0 ; i < message->parameters.GetSize () ; i++)
      {
	const KnownParameter& msgpar = message->parameters.Get (i);
        par = pm->GetParameter (msgpar.value, msgpar.type);
        if (!par) return false;
        params.Put (pl->FetchStringID (msgpar.name), par);
      }
    }

    pctpl->PerformAction (actionID, params);
    return true;
  }

  virtual void FinishUpdate ()
  {
    pcPanel->UpdateWireMsg ();
  }

  virtual const char* GetColumns () { return "Message,Entity,Parameters"; }
  virtual iUIDialog* GetEditorDialog () { return pcPanel->GetWireMsgDialog (); }
};

class WireParRowModel : public PcParRowModel
{
private:
  csRef<iCelParameterIterator> it;

  csStringID nextID;
  iParameter* nextPar;

  void SearchNext ()
  {
    nextID = csInvalidStringID;
    while (it->HasNext ())
    {
      nextPar = it->Next (nextID);
      iCelPlLayer* pl = pcPanel->GetPL ();
      csString name = pl->FetchString (nextID);
      if (name != "msgid" && name != "entity") return;
      nextID = csInvalidStringID;
    }
  }

public:
  WireParRowModel (PropertyClassPanel* pcPanel) : PcParRowModel (pcPanel),
     nextID (csInvalidStringID) { }
  virtual ~WireParRowModel () { }

  virtual void ResetIterator ()
  {
    nextID = csInvalidStringID;
    size_t currentMessage = pcPanel->GetMessagePropertyIndex ();
    if (currentMessage == csArrayItemNotFound) { it = 0; return; }
    csStringID id;
    celData data;
    it = pctpl->GetProperty (currentMessage, id, data);
    SearchNext ();
  }
  virtual bool HasRows () { return nextID != csInvalidStringID; }
  virtual csStringArray NextRow ()
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csString name = pl->FetchString (nextID);
    csString val = nextPar->GetOriginalExpression ();
    csString type = InspectTools::TypeToString (nextPar->GetPossibleType ());

    SearchNext ();

    return Tools::MakeArray (name.GetData (), val.GetData (), type.GetData (), (const char*)0);
  }

  virtual bool DeleteRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    size_t currentMessage = pcPanel->GetMessagePropertyIndex ();
    InspectTools::DeleteActionParameter (pl, pctpl, currentMessage, row[0]);
    return true;
  }
  virtual bool AddRow (const csStringArray& row)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    iParameterManager* pm = pcPanel->GetPM ();

    size_t currentMessage = pcPanel->GetMessagePropertyIndex ();
    if (currentMessage == csArrayItemNotFound)
    {
      pcPanel->GetUIManager ()->Error ("Select a message first!");
      return false;
    }

    csString name = row[0];
    csString value = row[1];
    csString type = row[2];

    csRef<iParameter> par = pm->GetParameter (value, InspectTools::StringToType (type));
    if (!par) return false;

    InspectTools::AddActionParameter (pl, pctpl, currentMessage, name, par);
    return true;
  }

  virtual void FinishUpdate ()
  {
    pcPanel->UpdateWireMsg ();
  }

  virtual const char* GetColumns () { return "Name,Value,Type"; }
  virtual iUIDialog* GetEditorDialog () { return pcPanel->GetWireParDialog (); }
};

/**
 * This editor model is associated with the wire message model and
 * is responsible for updating the wire parameter list.
 */
class WireParEditorModel : public EditorModel
{
private:
  ListCtrlView* wireParView;

public:
  WireParEditorModel (ListCtrlView* wireParView) : wireParView (wireParView) { }
  virtual ~WireParEditorModel () { }
  virtual void Update (const csStringArray& row)
  {
    wireParView->Refresh ();
  }
  virtual csStringArray Read ()
  {
    // @@@ Not supported here.
    return csStringArray ();
  }
};


iUIDialog* PropertyClassPanel::GetWireParDialog ()
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

iUIDialog* PropertyClassPanel::GetWireMsgDialog ()
{
  if (!wireMsgDialog)
  {
    wireMsgDialog = uiManager->CreateDialog ("Edit message");
    wireMsgDialog->AddRow ();
    wireMsgDialog->AddLabel ("Message:");
    iEditorConfig* config = emode->GetApplication ()->GetConfig ();
    csStringArray messageArray;
    const csArray<KnownMessage>& messages = config->GetMessages ();
    for (size_t i = 0 ; i < messages.GetSize () ; i++)
      messageArray.Push (messages.Get (i).name);
    wireMsgDialog->AddCombo ("Message", messageArray);
    wireMsgDialog->AddRow ();
    wireMsgDialog->AddLabel ("Entity:");
    wireMsgDialog->AddText ("Entity");
    wireMsgDialog->AddButton ("...");	// @@@ Not implemented yet.
  }
  return wireMsgDialog;
}

size_t PropertyClassPanel::GetMessagePropertyIndex ()
{
  wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return csArrayItemNotFound;
  csStringArray row = ListCtrlTools::ReadRow (list, idx);
  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    if (name == "AddOutput")
    {
      csString msgName;
      csString entName;
      csString pars = wireMsgModel->GetParameterString (params, msgName, entName);
      if (msgName == row[0] && entName == row[1] && pars == row[2])
	return i;
    }
  }
  return csArrayItemNotFound;
}

void PropertyClassPanel::UpdateWireMsg ()
{
  wxListCtrl* list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  long sel = ListCtrlTools::GetFirstSelectedRow (list);
  wireMsgView->Refresh ();
  emode->PCWasEdited (pctpl);
  if (sel != -1)
    ListCtrlTools::SelectRow (list, sel);
  wireParView->Refresh ();
}

bool PropertyClassPanel::UpdateWire ()
{
  SwitchPCType ("pclogic.wire");

  pctpl->RemoveProperty (pl->FetchStringID ("AddInput"));

  wxComboBox* inputMaskCombo = XRCCTRL (*this, "wireInputMaskCombo", wxComboBox);

  {
    ParHash params;
    csString mask = (const char*)inputMaskCombo->GetValue ().mb_str (wxConvUTF8);
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
  wxComboBox* inputMaskCombo = XRCCTRL (*this, "wireInputMaskCombo", wxComboBox);
  inputMaskCombo->SetValue (wxT (""));

  if (!pctpl || csString ("pclogic.wire") != pctpl->GetName ()) return;

  csString inputMask = InspectTools::GetActionParameterValueString (pl, pctpl,
      "AddInput", "mask");
  inputMaskCombo->SetValue (wxString::FromUTF8 (inputMask));

  wireMsgModel->SetPC (pctpl);
  wireMsgView->Refresh ();

  wireParModel->SetPC (pctpl);
  wireParView->Refresh ();
}

// -----------------------------------------------------------------------

bool PropertyClassPanel::UpdateOldCamera ()
{
  SwitchPCType ("pccamera.old");

  wxChoice* modeChoice = XRCCTRL (*this, "modeChoice", wxChoice);
  csString modeName = (const char*)modeChoice->GetStringSelection ().mb_str(wxConvUTF8);

  InspectTools::DeleteActionParameter (pl, pctpl, "SetCamera", "modename");
  csRef<iParameter> par = pm->GetParameter (modeName, CEL_DATA_STRING);
  if (!par) return false;
  InspectTools::AddActionParameter (pl, pctpl, "SetCamera", "modename", par);

  emode->PCWasEdited (pctpl);
  return true;
}

void PropertyClassPanel::FillOldCamera ()
{
  if (!pctpl || csString ("pccamera.old") != pctpl->GetName ()) return;
  csString modeName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "SetCamera", "modename");
  wxChoice* modeChoice = XRCCTRL (*this, "modeChoice", wxChoice);
  modeChoice->SetStringSelection (wxString::FromUTF8 (modeName));
}

// -----------------------------------------------------------------------

class SpawnCollectionValue : public PcParCollectionValue
{
private:
  Value* NewChild (const char* name)
  {
    return NewCompositeChild (
	  VALUE_STRING, "Name", name,
	  VALUE_NONE);
  }

protected:
  virtual void UpdateChildren ()
  {
    if (!pctpl) return;
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    iCelPlLayer* pl = pcPanel->GetPL ();
    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddEntityTemplateType")
      {
        iCelPlLayer* pl = pcPanel->GetPL ();
        csStringID nameID = pl->FetchStringID ("template");
        csString parName;
        while (params->HasNext ())
        {
          csStringID parid;
          iParameter* par = params->Next (parid);
          if (parid == nameID) parName = par->GetOriginalExpression ();
        }
	NewChild (parName);
      }
    }
  }

  bool RemoveActionWithName (const char* name)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    size_t idx = InspectTools::FindActionWithParameter (pl, pctpl,
	"AddEntityTemplateType", "template", name);
    if (idx == csArrayItemNotFound) return false;
    pctpl->RemovePropertyByIndex (idx);
    return true;
  }

  bool UpdateAction (const char* name)
  {
    ParHash params;
    iCelPlLayer* pl = pcPanel->GetPL ();
    iParameterManager* pm = pcPanel->GetPM ();
    csStringID actionID = pl->FetchStringID ("AddEntityTemplateType");
    csStringID nameID = pl->FetchStringID ("template");

    iCelEntityTemplate* t = pl->FindEntityTemplate (name);
    if (!t)
    {
      pcPanel->GetUIManager ()->Error ("Can't find template '%s'!", name);
      return false;
    }

    csRef<iParameter> par;
    par = pm->GetParameter (name, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (nameID, par);

    RemoveActionWithName (name);
    pctpl->PerformAction (actionID, params);
    return true;
  }

public:
  SpawnCollectionValue (PropertyClassPanel* pcPanel) : PcParCollectionValue (pcPanel) { }
  virtual ~SpawnCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    if (!pctpl) return false;
    Value* nameValue = child->GetChildByName ("Name");
    if (!RemoveActionWithName (nameValue->GetStringValue ()))
      return false;
    RemoveChild (child);
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!pctpl) return 0;
    csString name = suggestion.Get ("Name", (const char*)0);
    if (!UpdateAction (name))
      return 0;

    Value* child = NewChild (name);
    FireValueChanged ();
    return child;
  }
};

iUIDialog* PropertyClassPanel::GetSpawnTemplateDialog ()
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

  spawnCollectionValue->SetPC (pctpl);
  spawnCollectionValue->Refresh ();
}

// -----------------------------------------------------------------------

class QuestCollectionValue : public PcParCollectionValue
{
private:
  Value* NewChild (const char* name, const char* value, const char* type)
  {
    return NewCompositeChild (
	  VALUE_STRING, "Name", name,
	  VALUE_STRING, "Value", value,
	  VALUE_STRING, "Type", type,
	  VALUE_NONE);
  }

protected:
  virtual void UpdateChildren ()
  {
    if (!pctpl) return;
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    iCelPlLayer* pl = pcPanel->GetPL ();
    size_t nqIdx = pctpl->FindProperty (pl->FetchStringID ("NewQuest"));
    if (nqIdx == csArrayItemNotFound) return;
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> it = pctpl->GetProperty (nqIdx, id, data);
    while (it->HasNext ())
    {
      csStringID nextID;
      iParameter* nextPar = it->Next (nextID);
      csString name = pl->FetchString (nextID);
      if (name == "name") continue;
      csString val = nextPar->GetOriginalExpression ();
      csString type = InspectTools::TypeToString (nextPar->GetPossibleType ());
      NewChild (name, val, type);
    }
  }

public:
  QuestCollectionValue (PropertyClassPanel* pcPanel) : PcParCollectionValue (pcPanel) { }
  virtual ~QuestCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    if (!pctpl) return false;
    iCelPlLayer* pl = pcPanel->GetPL ();
    Value* nameValue = child->GetChildByName ("Name");
    if (!InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", nameValue->GetStringValue ()))
      return false;
    RemoveChild (child);
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!pctpl) return 0;
    iCelPlLayer* pl = pcPanel->GetPL ();
    iParameterManager* pm = pcPanel->GetPM ();
    csString name = suggestion.Get ("Name", (const char*)0);
    csString value = suggestion.Get ("Value", (const char*)0);
    csString type = suggestion.Get ("Type", (const char*)0);

    csRef<iParameter> par = pm->GetParameter (value, InspectTools::StringToType (type));
    if (!par) return 0;
    InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", name);
    InspectTools::AddActionParameter (pl, pctpl, "NewQuest", name, par);

    Value* child = NewChild (name, value, type);
    FireValueChanged ();
    return child;
  }
  virtual bool UpdateValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!pctpl) return false;
    iCelPlLayer* pl = pcPanel->GetPL ();
    iParameterManager* pm = pcPanel->GetPM ();
    csString name = suggestion.Get ("Name", (const char*)0);
    csString value = suggestion.Get ("Value", (const char*)0);
    csString type = suggestion.Get ("Type", (const char*)0);

    csRef<iParameter> par = pm->GetParameter (value, InspectTools::StringToType (type));
    if (!par) return false;
    InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", name);
    InspectTools::AddActionParameter (pl, pctpl, "NewQuest", name, par);
    selectedValue->GetChildByName ("Name")->SetStringValue (name);
    selectedValue->GetChildByName ("Value")->SetStringValue (value);
    selectedValue->GetChildByName ("Type")->SetStringValue (type);
    FireValueChanged ();
    return true;
  }
};

iUIDialog* PropertyClassPanel::GetQuestDialog ()
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

  csRef<iParameter> par = pm->GetParameter (questName, CEL_DATA_STRING);
  if (!par)
  {
    uiManager->Error ("Invalid quest name!");
    return false;
  }
  InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", "name");
  InspectTools::AddActionParameter (pl, pctpl, "NewQuest", "name", par);
  GetEntityMode ()->PCWasEdited (pctpl);

  return true;
}

void PropertyClassPanel::FillQuest ()
{
  wxListCtrl* parList = XRCCTRL (*this, "questParameterListCtrl", wxListCtrl);
  parList->DeleteAllItems ();
  wxTextCtrl* text = XRCCTRL (*this, "questText", wxTextCtrl);
  text->SetValue (wxT (""));

  if (!pctpl || csString ("pclogic.quest") != pctpl->GetName ()) return;

  csRef<iQuestManager> quest_mgr = csQueryRegistryOrLoad<iQuestManager> (
    emode->GetObjectRegistry (), "cel.manager.quests");

  iQuestFactory* questFact = 0;
  csString questName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
  if (!questName.IsEmpty ())
  {
    text->SetValue (wxString::FromUTF8 (questName));
    questFact = quest_mgr->GetQuestFactory (questName);
  }
  if (questFact)
  {
    // Fill all states and mark the default state.
    // @@@ Check? Does this actually do anything?
    wxArrayString states;
    csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
    while (it->HasNext ())
    {
      iQuestStateFactory* stateFact = it->Next ();
      states.Add (wxString::FromUTF8 (stateFact->GetName ()));
    }
  }

  questCollectionValue->SetPC (pctpl);
  questCollectionValue->Refresh ();
}

// -----------------------------------------------------------------------

class InventoryCollectionValue : public PcParCollectionValue
{
private:
  Value* NewChild (const char* name, const char* amount)
  {
    return NewCompositeChild (
	  VALUE_STRING, "Name", name,
	  VALUE_STRING, "Amount", amount,
	  VALUE_NONE);
  }

protected:
  virtual void UpdateChildren ()
  {
    if (!pctpl) return;
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    iCelPlLayer* pl = pcPanel->GetPL ();
    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddTemplate")
      {
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
	NewChild (parName, parAmount);
      }
    }
  }

  bool RemoveActionWithName (const char* name)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    size_t idx = InspectTools::FindActionWithParameter (pl, pctpl,
	"AddTemplate", "name", name);
    if (idx == csArrayItemNotFound) return false;
    pctpl->RemovePropertyByIndex (idx);
    return true;
  }

  bool UpdateAction (const char* name, const char* amount)
  {
    ParHash params;
    iCelPlLayer* pl = pcPanel->GetPL ();
    iParameterManager* pm = pcPanel->GetPM ();
    csStringID actionID = pl->FetchStringID ("AddTemplate");
    csStringID nameID = pl->FetchStringID ("name");
    csStringID amountID = pl->FetchStringID ("amount");

    iCelEntityTemplate* t = pl->FindEntityTemplate (name);
    if (!t)
    {
      pcPanel->GetUIManager ()->Error ("Can't find template '%s'!", name);
      return false;
    }

    csRef<iParameter> par;
    par = pm->GetParameter (name, CEL_DATA_STRING);
    if (!par) return false;
    params.Put (nameID, par);

    par = pm->GetParameter (amount, CEL_DATA_LONG);
    if (!par) return false;
    params.Put (amountID, par);

    RemoveActionWithName (name);
    pctpl->PerformAction (actionID, params);
    return true;
  }


public:
  InventoryCollectionValue (PropertyClassPanel* pcPanel) : PcParCollectionValue (pcPanel) { }
  virtual ~InventoryCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    if (!pctpl) return false;
    Value* nameValue = child->GetChildByName ("Name");
    if (!RemoveActionWithName (nameValue->GetStringValue ()))
      return false;
    RemoveChild (child);
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!pctpl) return 0;
    csString name = suggestion.Get ("Name", (const char*)0);
    csString amount = suggestion.Get ("Amount", (const char*)0);
    if (!UpdateAction (name, amount))
      return 0;

    Value* child = NewChild (name, amount);
    FireValueChanged ();
    return child;
  }

  virtual bool UpdateValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!pctpl) return false;
    csString name = suggestion.Get ("Name", (const char*)0);
    csString amount = suggestion.Get ("Amount", (const char*)0);
    if (!UpdateAction (name, amount))
      return false;
    selectedValue->GetChildByName ("Name")->SetStringValue (name);
    selectedValue->GetChildByName ("Amount")->SetStringValue (amount);
    FireValueChanged ();
    return true;
  }
};

iUIDialog* PropertyClassPanel::GetInventoryTemplateDialog ()
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

  inventoryCollectionValue->SetPC (pctpl);
  inventoryCollectionValue->Refresh ();

  csString lootName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "SetLootGenerator", "name");
  lootText->SetValue (wxString::FromUTF8 (lootName));
}

// -----------------------------------------------------------------------

class PropertyCollectionValue : public PcParCollectionValue
{
private:
  Value* NewChild (const char* name, const char* value, const char* type)
  {
    return NewCompositeChild (
	  VALUE_STRING, "Name", name,
	  VALUE_STRING, "Value", value,
	  VALUE_STRING, "Type", type,
	  VALUE_NONE);
  }

  bool SetProperty (const csString& type, const char* name, csString& value)
  {
    iCelPlLayer* pl = pcPanel->GetPL ();
    csStringID nameID = pl->FetchStringID (name);
    pctpl->RemoveProperty (nameID);
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

protected:
  virtual void UpdateChildren ()
  {
    if (!pctpl) return;
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString value;
      celParameterTools::ToString (data, value);
      csString name = pcPanel->GetPL ()->FetchString (id);
      csString type = InspectTools::TypeToString (data.type);
      NewChild (name, value, type);
    }
  }

public:
  PropertyCollectionValue (PropertyClassPanel* pcPanel) : PcParCollectionValue (pcPanel) { }
  virtual ~PropertyCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    if (!pctpl) return false;
    Value* nameValue = child->GetChildByName ("Name");
    iCelPlLayer* pl = pcPanel->GetPL ();
    pctpl->RemoveProperty (pl->FetchStringID (nameValue->GetStringValue ()));
    RemoveChild (child);
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!pctpl) return false;
    csString name = suggestion.Get ("Name", (const char*)0);
    csString value = suggestion.Get ("Value", (const char*)0);
    csString type = suggestion.Get ("Type", (const char*)0);
    if (!SetProperty (type, name, value)) return 0;
    Value* child = NewChild (name, value, type);
    FireValueChanged ();
    return child;
  }
  virtual bool UpdateValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!pctpl) return false;
    csString name = suggestion.Get ("Name", (const char*)0);
    csString value = suggestion.Get ("Value", (const char*)0);
    csString type = suggestion.Get ("Type", (const char*)0);
    if (!SetProperty (type, name, value)) return false;
    selectedValue->GetChildByName ("Name")->SetStringValue (name);
    selectedValue->GetChildByName ("Value")->SetStringValue (value);
    selectedValue->GetChildByName ("Type")->SetStringValue (type);
    FireValueChanged ();
    return true;
  }
};

iUIDialog* PropertyClassPanel::GetPropertyDialog ()
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

  propertyCollectionValue->SetPC (pctpl);
  propertyCollectionValue->Refresh ();
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
  else if (page == "pccamera.old") return UpdateOldCamera ();
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

    UITools::SwitchPage (this, "pcChoicebook", pcName);
    UITools::SetValue (this, "tagTextCtrl", tagName);
  }

  FillProperties ();
  FillInventory ();
  FillQuest ();
  FillSpawn ();
  FillWire ();
  FillOldCamera ();
}

void PropertyClassPanel::SetupList (const char* listName, const char* heading, const char* names,
      Value* collectionValue, iUIDialog* dialog, bool do_edit)
{
  DefineHeading (listName, heading, names);
  Bind (collectionValue, listName);
  wxWindow* comp = FindComponentByName (this, listName);
  wxListCtrl* list = wxStaticCast (comp, wxListCtrl);
  AddAction (list, NEWREF(Action, new NewChildDialogAction (collectionValue, dialog)));
  if (do_edit)
    AddAction (list, NEWREF(Action, new EditChildDialogAction (collectionValue, dialog)));
  AddAction (list, NEWREF(Action, new DeleteChildAction (collectionValue)));
}

PropertyClassPanel::PropertyClassPanel (wxWindow* parent, iUIManager* uiManager,
    EntityMode* emode) :
  View (this), uiManager (uiManager), emode (emode), tpl (0), pctpl (0)
{
  pl = emode->GetPL ();
  pm = csQueryRegistryOrLoad<iParameterManager> (emode->GetObjectRegistry (),
      "cel.parameters.manager");
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("PropertyClassPanel"));

  wxComboBox* wireInputMaskCombo = XRCCTRL (*this, "wireInputMaskCombo", wxComboBox);
  iEditorConfig* config = emode->GetApplication ()->GetConfig ();
  const csArray<KnownMessage>& messages = config->GetMessages ();
  for (size_t i = 0 ; i < messages.GetSize () ; i++)
    wireInputMaskCombo->Append (wxString::FromUTF8 (messages.Get (i).name));

  wxListCtrl* list;

  propertyCollectionValue.AttachNew (new PropertyCollectionValue (this));
  SetupList ("propertyListCtrl", "Name,Value,Type", "Name,Value,Type",
      propertyCollectionValue, GetPropertyDialog ());

  inventoryCollectionValue.AttachNew (new InventoryCollectionValue (this));
  SetupList ("inventoryTemplateListCtrl", "Name,Amount", "Name,Amount",
      inventoryCollectionValue, GetInventoryTemplateDialog ());

  spawnCollectionValue.AttachNew (new SpawnCollectionValue (this));
  SetupList ("spawnTemplateListCtrl", "Template", "Name",
      spawnCollectionValue, GetSpawnTemplateDialog (), false);

  questCollectionValue.AttachNew (new QuestCollectionValue (this));
  SetupList ("questParameterListCtrl", "Name,Value,Type", "Name,Value,Type",
      questCollectionValue, GetQuestDialog ());

  list = XRCCTRL (*this, "wireMessageListCtrl", wxListCtrl);
  wireMsgModel.AttachNew (new WireMsgRowModel (this));
  wireMsgView = new ListCtrlView (list, wireMsgModel);

  list = XRCCTRL (*this, "wireParameterListCtrl", wxListCtrl);
  wireParModel.AttachNew (new WireParRowModel (this));
  wireParView = new ListCtrlView (list, wireParModel);
  wireParEditorModel.AttachNew (new WireParEditorModel (wireParView));
  wireMsgView->SetEditorModel (wireParEditorModel);
}

PropertyClassPanel::~PropertyClassPanel ()
{
  delete wireMsgView;
  delete wireParView;
}


