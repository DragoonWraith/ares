/*
The MIT License

Copyright (c) 2013 by Jorrit Tyberghein

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
#include "edcommon/inspect.h"
#include "edcommon/uitools.h"
#include "entitymode.h"
#include "questgrid.h"
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/imodelrepository.h"
#include "editor/iconfig.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/parameters.h"
#include "tools/questmanager.h"
#include "propclass/chars.h"

#include <wx/xrc/xmlres.h>
#include <wx/listbox.h>
#include "cseditor/wx/propgrid/propdev.h"

//---------------------------------------------------------------------------

// -----------------------------------------------------------------------
// Templates Property
// -----------------------------------------------------------------------

//WX_PG_DECLARE_ARRAYSTRING_PROPERTY_WITH_DECL(wxEntitiesProperty, class wxEMPTY_PARAMETER_VALUE)
class wxEMPTY_PARAMETER_VALUE wxPG_PROPCLASS(wxEntitiesProperty) : public wxPG_PROPCLASS(wxArrayStringProperty)
{
  WX_PG_DECLARE_PROPERTY_CLASS(wxPG_PROPCLASS(wxEntitiesProperty))
  EntityMode* emode;

public:
  wxPG_PROPCLASS(wxEntitiesProperty)( const wxString& label = wxPG_LABEL,
      const wxString& name = wxPG_LABEL, const wxArrayString& value = wxArrayString());
  virtual ~wxPG_PROPCLASS(wxEntitiesProperty)();
  virtual void GenerateValueAsString();
  virtual bool StringToValue( wxVariant& value, const wxString& text, int = 0 ) const;
  virtual bool OnEvent( wxPropertyGrid* propgrid, wxWindow* primary, wxEvent& event );
  virtual bool OnCustomStringEdit( wxWindow* parent, wxString& value );
  void SetEntityMode (EntityMode* emode)
  {
    this->emode = emode;
  }
  WX_PG_DECLARE_VALIDATOR_METHODS()
};


WX_PG_IMPLEMENT_ARRAYSTRING_PROPERTY(wxEntitiesProperty, wxT (','), wxT ("Browse"))

bool wxEntitiesProperty::OnCustomStringEdit( wxWindow* parent, wxString& value )
{
  using namespace Ares;
  csRef<Value> objects = emode->Get3DView ()->GetModelRepository ()->GetObjectsWithEntityValue ();
  iUIManager* ui = emode->GetApplication ()->GetUI ();
  Value* chosen = ui->AskDialog ("Select an entity", objects, "Entity,Template,Dynfact,Logic",
      DYNOBJ_COL_ENTITY, DYNOBJ_COL_TEMPLATE, DYNOBJ_COL_FACTORY, DYNOBJ_COL_LOGIC);
  if (chosen)
  {
    csString name = chosen->GetStringArrayValue ()->Get (DYNOBJ_COL_ENTITY);
    value = wxString::FromUTF8 (name);
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------

class RSNewState : public RewardSupport
{
public:
  RSNewState (EntityMode* emode) : RewardSupport ("NewState", emode) { }
  virtual ~RSNewState () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iNewStateQuestRewardFactory> tf = scfQueryInterface<iNewStateQuestRewardFactory> (rewardFact);
    wxArrayString states;
    states.Add (wxT ("-"));
    iQuestFactory* questFact = emode->GetSelectedQuest ();
    if (questFact)
    {
      csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
      while (it->HasNext ())
      {
        iQuestStateFactory* stateFact = it->Next ();
        states.Add (wxString::FromUTF8 (stateFact->GetName ()));
      }
    }
    wxPGProperty* stateProp = AppendEditEnumPar (responseProp, "State", "State", states,
	wxArrayInt (), tf->GetStateParameter ());

    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntityParameter ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTagParameter ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClassParameter ());
  }
};

//---------------------------------------------------------------------------

class RSDbPrint : public RewardSupport
{
public:
  RSDbPrint (EntityMode* emode) : RewardSupport ("DbPrint", emode) { }
  virtual ~RSDbPrint () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (rewardFact);
    AppendStringPar (responseProp, "Message", "Message", tf->GetMessage ());
  }
};

//---------------------------------------------------------------------------

class RSInventory : public RewardSupport
{
public:
  RSInventory (EntityMode* emode) : RewardSupport ("Inventory", emode) { }
  virtual ~RSInventory () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iInventoryRewardFactory> tf = scfQueryInterface<iInventoryRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendButtonPar (responseProp, "ChildEntity", "E:", tf->GetChildEntity ());
    AppendStringPar (responseProp, "ChildTag", "Tag", tf->GetChildTag ());
  }
};

//---------------------------------------------------------------------------

class RSSequence : public RewardSupport
{
public:
  RSSequence (EntityMode* emode) : RewardSupport ("Sequence", emode) { }
  virtual ~RSSequence () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iSequenceRewardFactory> tf = scfQueryInterface<iSequenceRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
    AppendStringPar (responseProp, "Sequence", "Sequence", tf->GetSequence ());	// @@@ Enum!
    AppendStringPar (responseProp, "Delay", "Delay", tf->GetDelay ());
  }
};

//---------------------------------------------------------------------------

class RSCsSequence : public RewardSupport
{
public:
  RSCsSequence (EntityMode* emode) : RewardSupport ("CsSequence", emode) { }
  virtual ~RSCsSequence () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iCsSequenceRewardFactory> tf = scfQueryInterface<iCsSequenceRewardFactory> (rewardFact);
    AppendStringPar (responseProp, "Sequence", "Sequence", tf->GetSequence ());	// @@@ Enum!
    AppendStringPar (responseProp, "Delay", "Delay", tf->GetDelay ());
  }
};

//---------------------------------------------------------------------------

class RSSeqFinish : public RewardSupport
{
public:
  RSSeqFinish (EntityMode* emode) : RewardSupport ("SeqFinish", emode) { }
  virtual ~RSSeqFinish () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
    AppendStringPar (responseProp, "Sequence", "Sequence", tf->GetSequence ());	// @@@ Enum!
  }
};

//---------------------------------------------------------------------------

class RSChangeProp : public RewardSupport
{
public:
  RSChangeProp (EntityMode* emode) : RewardSupport ("ChangeProp", emode) { }
  virtual ~RSChangeProp () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iChangePropertyRewardFactory> tf = scfQueryInterface<iChangePropertyRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
    AppendStringPar (responseProp, "PC", "PC", tf->GetPC ());	// @@@ Enum?
    AppendStringPar (responseProp, "PC Tag", "PCTag", tf->GetPCTag ());
    AppendStringPar (responseProp, "Property", "Property", tf->GetProperty ());
    AppendStringPar (responseProp, "String", "String", tf->GetString ());
    AppendStringPar (responseProp, "Long", "Long", tf->GetLong ());
    AppendStringPar (responseProp, "Float", "Float", tf->GetFloat ());
    AppendStringPar (responseProp, "Bool", "Bool", tf->GetBool ());
    AppendStringPar (responseProp, "Diff", "Diff", tf->GetDiff ());
    AppendBoolPar (responseProp, "Toggle", "Toggle", tf->IsToggle ());
  }
};

//---------------------------------------------------------------------------

class RSCreateEnt : public RewardSupport
{
public:
  RSCreateEnt (EntityMode* emode) : RewardSupport ("CreateEnt", emode) { }
  virtual ~RSCreateEnt () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Template", "T:", tf->GetEntityTemplate ());
    AppendStringPar (responseProp, "Name", "Name", tf->GetName ());
    // @@@ Add support for parameters
  }
};

//---------------------------------------------------------------------------

class RSDestroyEnt : public RewardSupport
{
public:
  RSDestroyEnt (EntityMode* emode) : RewardSupport ("DestroyEnt", emode) { }
  virtual ~RSDestroyEnt () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
  }
};

//---------------------------------------------------------------------------

class RSChangeClass : public RewardSupport
{
public:
  RSChangeClass (EntityMode* emode) : RewardSupport ("ChangeClass", emode) { }
  virtual ~RSChangeClass () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iChangeClassRewardFactory> tf = scfQueryInterface<iChangeClassRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());

    wxArrayString entitiesArray;
    if (tf->GetEntities ())
    {
      csStringArray array (tf->GetEntities (), ",");
      for (size_t i = 0 ; i < array.GetSize () ; i++)
        entitiesArray.Add (wxString::FromUTF8 (array.Get (i)));
    }
    wxEntitiesProperty* tempProp = new wxEntitiesProperty (
	  wxT ("Entities"), wxPG_LABEL, entitiesArray);
    tempProp->SetEntityMode (emode);
    detailGrid->AppendIn (responseProp, tempProp);
  }
};

//---------------------------------------------------------------------------

class RSAction : public RewardSupport
{
public:
  RSAction (EntityMode* emode) : RewardSupport ("Action", emode) { }
  virtual ~RSAction () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
    AppendButtonPar (responseProp, "Action", "A:", tf->GetID ());
    AppendStringPar (responseProp, "PC", "PC", tf->GetPropertyClass ());	// @@@ Enum?
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
  }
};

//---------------------------------------------------------------------------

class RSMessage : public RewardSupport
{
public:
  RSMessage (EntityMode* emode) : RewardSupport ("Message", emode) { }
  virtual ~RSMessage () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());

    wxArrayString entitiesArray;
    if (tf->GetEntities ())
    {
      csStringArray array (tf->GetEntities (), ",");
      for (size_t i = 0 ; i < array.GetSize () ; i++)
        entitiesArray.Add (wxString::FromUTF8 (array.Get (i)));
    }
    wxEntitiesProperty* tempProp = new wxEntitiesProperty (
	  wxT ("Entities"), wxPG_LABEL, entitiesArray);
    tempProp->SetEntityMode (emode);
    detailGrid->AppendIn (responseProp, tempProp);

    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
    AppendButtonPar (responseProp, "Message", "A:", tf->GetID ());
  }
};


//---------------------------------------------------------------------------

RewardSupportDriver::RewardSupportDriver (const char* name, EntityMode* emode)
  : GridSupport (name, emode)
{
  RegisterEditor (new RSNewState (emode));
  RegisterEditor (new RSDbPrint (emode));
  RegisterEditor (new RSInventory (emode));
  RegisterEditor (new RSSequence (emode));
  RegisterEditor (new RSCsSequence (emode));
  RegisterEditor (new RSSeqFinish (emode));
  RegisterEditor (new RSChangeProp (emode));
  RegisterEditor (new RSCreateEnt (emode));
  RegisterEditor (new RSDestroyEnt (emode));
  RegisterEditor (new RSChangeClass (emode));
  RegisterEditor (new RSAction (emode));
  RegisterEditor (new RSMessage (emode));

  rewardtypesArray.Add (wxT ("NewState"));
  rewardtypesArray.Add (wxT ("DbPrint"));
  rewardtypesArray.Add (wxT ("Inventory"));
  rewardtypesArray.Add (wxT ("Sequence"));
  rewardtypesArray.Add (wxT ("CsSequence"));
  rewardtypesArray.Add (wxT ("SeqFinish"));
  rewardtypesArray.Add (wxT ("ChangeProp"));
  rewardtypesArray.Add (wxT ("CreateEnt"));
  rewardtypesArray.Add (wxT ("DestroyEnt"));
  rewardtypesArray.Add (wxT ("ChangeClass"));
  rewardtypesArray.Add (wxT ("Action"));
  rewardtypesArray.Add (wxT ("Message"));
}

void RewardSupportDriver::Fill (wxPGProperty* responseProp,
    iRewardFactory* rewardFact)
{
  csString type = emode->GetRewardType (rewardFact);
  wxPGProperty* outputProp = AppendStringPar (responseProp, "Reward", "Reward", "<composed>");
  AppendEnumPar (outputProp, "Type", "RewType", rewardtypesArray,
      wxArrayInt (), rewardtypesArray.Index (wxString::FromUTF8 (type)));
  RewardSupport* editor = GetEditor (type);
  if (editor)
  {
    editor->Fill (outputProp, rewardFact);
  }
  detailGrid->Collapse (outputProp);
}

void RewardSupportDriver::FillRewards (wxPGProperty* responseProp,
    iRewardFactoryArray* rewards)
{
  for (size_t j = 0 ; j < rewards->GetSize () ; j++)
  {
    iRewardFactory* reward = rewards->Get (j);
    Fill (responseProp, reward);
  }
}

//---------------------------------------------------------------------------

class TSTimeout : public TriggerSupport
{
public:
  TSTimeout (EntityMode* emode) : TriggerSupport ("Timeout", emode) { }
  virtual ~TSTimeout () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iTimeoutTriggerFactory> tf = scfQueryInterface<iTimeoutTriggerFactory> (triggerFact);
    AppendStringPar (responseProp, "Timeout", "Timeout", tf->GetTimeout ());
  }
};


//---------------------------------------------------------------------------

class TSEnterSect : public TriggerSupport
{
public:
  TSEnterSect (EntityMode* emode) : TriggerSupport ("EnterSect", emode) { }
  virtual ~TSEnterSect () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Sector", "Sector", tf->GetSector ());	// @@@Button?
  }
};

//---------------------------------------------------------------------------

class TSSeqFinish : public TriggerSupport
{
public:
  TSSeqFinish (EntityMode* emode) : TriggerSupport ("SeqFinish", emode) { }
  virtual ~TSSeqFinish () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Sequence", "Sequence", tf->GetSequence ());	// @@@Combo!
  }
};

//---------------------------------------------------------------------------

class TSInventory : public TriggerSupport
{
public:
  TSInventory (EntityMode* emode) : TriggerSupport ("Inventory", emode) { }
  virtual ~TSInventory () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendButtonPar (responseProp, "Child", "E:", tf->GetChildEntity ());
    AppendButtonPar (responseProp, "ChildTemplate", "T:", tf->GetChildTemplate ());
  }
};

//---------------------------------------------------------------------------

class TSMeshSelect : public TriggerSupport
{
public:
  TSMeshSelect (EntityMode* emode) : TriggerSupport ("MeshSelect", emode) { }
  virtual ~TSMeshSelect () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iMeshSelectTriggerFactory> tf = scfQueryInterface<iMeshSelectTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
  }
};

//---------------------------------------------------------------------------

class TSMessage : public TriggerSupport
{
public:
  TSMessage (EntityMode* emode) : TriggerSupport ("Message", emode) { }
  virtual ~TSMessage () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iMessageTriggerFactory> tf = scfQueryInterface<iMessageTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendButtonPar (responseProp, "Mask", "A:", tf->GetMask ());
  }
};

//---------------------------------------------------------------------------

class TSPropertyChange : public TriggerSupport
{
public:
  TSPropertyChange (EntityMode* emode) : TriggerSupport ("PropChange", emode) { }
  virtual ~TSPropertyChange () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iPropertyChangeTriggerFactory> tf = scfQueryInterface<iPropertyChangeTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Property", "Property", tf->GetProperty ());
    AppendStringPar (responseProp, "Value", "Value", tf->GetValue ());
    AppendStringPar (responseProp, "Operation", "Operation", tf->GetOperation ());	// @TODO Enum
    AppendBoolPar (responseProp, "ChangeOnly", "ChangeOnly", tf->IsOnChangeOnly ());
  }
};

//---------------------------------------------------------------------------

class TSTrigger : public TriggerSupport
{
public:
  TSTrigger (EntityMode* emode) : TriggerSupport ("Trigger", emode) { }
  virtual ~TSTrigger () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iTriggerTriggerFactory> tf = scfQueryInterface<iTriggerTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendBoolPar (responseProp, "Leave", "Leave", tf->IsLeaveEnabled ());
  }
};

//---------------------------------------------------------------------------

class TSWatch : public TriggerSupport
{
public:
  TSWatch (EntityMode* emode) : TriggerSupport ("Watch", emode) { }
  virtual ~TSWatch () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendButtonPar (responseProp, "Target", "E:", tf->GetTargetEntity ());
    AppendStringPar (responseProp, "Target Tag", "Tag", tf->GetTargetTag ());
    AppendStringPar (responseProp, "CheckTime", "CheckTimeTag", tf->GetChecktime ());
    AppendStringPar (responseProp, "Radius", "Radius", tf->GetRadius ());
    AppendStringPar (responseProp, "Offset X", "OffsetX", tf->GetOffsetX ());	// @@@ Make vector!
    AppendStringPar (responseProp, "Offset Y", "OffsetY", tf->GetOffsetY ());
    AppendStringPar (responseProp, "Offset Z", "OffsetZ", tf->GetOffsetZ ());
  }
};

//---------------------------------------------------------------------------

TriggerSupportDriver::TriggerSupportDriver (const char* name, EntityMode* emode)
  : GridSupport (name, emode)
{
  RegisterEditor (new TSTimeout (emode));
  RegisterEditor (new TSEnterSect (emode));
  RegisterEditor (new TSSeqFinish (emode));
  RegisterEditor (new TSInventory (emode));
  RegisterEditor (new TSMeshSelect (emode));
  RegisterEditor (new TSMessage (emode));
  RegisterEditor (new TSPropertyChange (emode));
  RegisterEditor (new TSTrigger (emode));
  RegisterEditor (new TSWatch (emode));

  trigtypesArray.Add (wxT ("Timeout"));
  trigtypesArray.Add (wxT ("EnterSect"));
  trigtypesArray.Add (wxT ("SeqFinish"));
  trigtypesArray.Add (wxT ("PropChange"));
  trigtypesArray.Add (wxT ("Trigger"));
  trigtypesArray.Add (wxT ("Watch"));
  trigtypesArray.Add (wxT ("Operation"));
  trigtypesArray.Add (wxT ("Inventory"));
  trigtypesArray.Add (wxT ("Message"));
  trigtypesArray.Add (wxT ("MeshSel"));
}

void TriggerSupportDriver::Fill (wxPGProperty* responseProp,
    iTriggerFactory* triggerFact)
{
  csString type = emode->GetTriggerType (triggerFact);
  wxPGProperty* outputProp = AppendStringPar (responseProp, "Trigger", "Trigger", "<composed>");
  AppendEnumPar (outputProp, "Type", "TrigType", trigtypesArray,
      wxArrayInt (), trigtypesArray.Index (wxString::FromUTF8 (type)));
  TriggerSupport* editor = GetEditor (type);
  if (editor)
  {
    editor->Fill (outputProp, triggerFact);
  }
  detailGrid->Collapse (outputProp);
}

//---------------------------------------------------------------------------

QuestEditorSupportMain::QuestEditorSupportMain (EntityMode* emode) :
  GridSupport ("main", emode)
{
#if 0
  idNewChar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnNewCharacteristic));
  idDelChar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnDeleteCharacteristic));
  idCreatePC = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreatePC));
  idDelPC = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnDeletePC));
#endif

  triggerEditor.AttachNew (new TriggerSupportDriver ("main", emode));
  rewardEditor.AttachNew (new RewardSupportDriver ("main", emode));
}

void QuestEditorSupportMain::FillResponses (wxPGProperty* stateProp, size_t idx, iQuestStateFactory* state)
{
  csString s;
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    iQuestTriggerResponseFactory* response = responses->Get (i);
    s.Format ("Response:%d:%d", int (idx), int (i));
    wxPGProperty* responseProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("Response"), wxString::FromUTF8 (s)));
    iTriggerFactory* triggerFact = response->GetTriggerFactory ();
    triggerEditor->Fill (responseProp, triggerFact);
    csRef<iRewardFactoryArray> rewards = response->GetRewardFactories ();
    rewardEditor->FillRewards (responseProp, rewards);
  }

}

void QuestEditorSupportMain::FillOnInit (wxPGProperty* stateProp, size_t idx,
    iQuestStateFactory* state)
{
  csString s;
  csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
  if (initRewards->GetSize () > 0)
  {
    s.Format ("OnInit:%d", int (idx));
    wxPGProperty* oninitProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("OnInit"), wxString::FromUTF8 (s)));
    rewardEditor->FillRewards (oninitProp, initRewards);
  }
}

void QuestEditorSupportMain::FillOnExit (wxPGProperty* stateProp, size_t idx,
    iQuestStateFactory* state)
{
  csString s;
  csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
  if (exitRewards->GetSize () > 0)
  {
    s.Format ("OnExit:%d", int (idx));
    wxPGProperty* onexitProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("OnExit"), wxString::FromUTF8 (s)));
    rewardEditor->FillRewards (onexitProp, exitRewards);
  }
}

void QuestEditorSupportMain::Fill (wxPGProperty* questProp, iQuestFactory* questFact)
{
  csString s, ss;

  size_t idx = 0;
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    s.Format ("State:%s", stateFact->GetName ());
    ss.Format ("State (%s)", stateFact->GetName ());
    wxPGProperty* stateProp = detailGrid->AppendIn (questProp,
      new wxPropertyCategory (wxString::FromUTF8 (ss), wxString::FromUTF8 (s)));
    FillOnInit (stateProp, idx, stateFact);
    FillResponses (stateProp, idx, stateFact);
    FillOnExit (stateProp, idx, stateFact);
    idx++;
  }
  csRef<iCelSequenceFactoryIterator> seqIt = questFact->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seqFact = seqIt->Next ();
    s.Format ("Sequence:%s", seqFact->GetName ());
    ss.Format ("Sequence (%s)", seqFact->GetName ());
    wxPGProperty* seqProp = detailGrid->AppendIn (questProp,
      new wxPropertyCategory (wxString::FromUTF8 (ss), wxString::FromUTF8 (s)));
  }
}

