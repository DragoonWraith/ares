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

RewardSupportDriver::RewardSupportDriver (const char* name, EntityMode* emode)
  : GridSupport (name, emode)
{
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
  wxPGProperty* typeProp = AppendEnumPar (outputProp, "Type", "RewType", rewardtypesArray,
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
  wxPGProperty* typeProp = AppendEnumPar (outputProp, "Type", "TrigType", trigtypesArray,
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
    wxPGProperty* stateProp = detailGrid->AppendIn (questProp,
      new wxPropertyCategory (wxString::FromUTF8 (ss), wxString::FromUTF8 (s)));
  }
}

