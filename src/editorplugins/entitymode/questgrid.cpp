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
}

void RewardSupportDriver::Fill (wxPGProperty* responseProp,
    iRewardFactory* rewardFact)
{
  csString type = emode->GetRewardType (rewardFact);
  csString s;
  s.Format ("Reward (%s)", type.GetData ());
  wxPGProperty* outputProp = AppendStringPar (responseProp, s, "Reward", "<composed>");
  RewardSupport* editor = GetEditor (type);
  if (editor)
  {
    editor->Fill (outputProp, rewardFact);
    detailGrid->Collapse (outputProp);
  }
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

class QESTriggerTimeout : public TriggerSupport
{
public:
  QESTriggerTimeout (EntityMode* emode) : TriggerSupport ("Timeout", emode) { }
  virtual ~QESTriggerTimeout () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
  }
};


//---------------------------------------------------------------------------

class QESTriggerEnterSect : public TriggerSupport
{
public:
  QESTriggerEnterSect (EntityMode* emode) : TriggerSupport ("EnterSect", emode) { }
  virtual ~QESTriggerEnterSect () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
  }
};

//---------------------------------------------------------------------------

class QESTriggerSeqFinish : public TriggerSupport
{
public:
  QESTriggerSeqFinish (EntityMode* emode) : TriggerSupport ("SeqFinish", emode) { }
  virtual ~QESTriggerSeqFinish () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Sequence", "Sequence", tf->GetSequence ());	// @@@Combo!
  }
};


//---------------------------------------------------------------------------

TriggerSupportDriver::TriggerSupportDriver (const char* name, EntityMode* emode)
  : GridSupport (name, emode)
{
  RegisterEditor (new QESTriggerTimeout (emode));
  RegisterEditor (new QESTriggerEnterSect (emode));
  RegisterEditor (new QESTriggerSeqFinish (emode));
}

void TriggerSupportDriver::Fill (wxPGProperty* responseProp,
    iTriggerFactory* triggerFact)
{
  csString type = emode->GetTriggerType (triggerFact);
  csString s;
  s.Format ("Trigger (%s)", type.GetData ());
  wxPGProperty* outputProp = AppendStringPar (responseProp, s, "Trigger", "<composed>");
  TriggerSupport* editor = GetEditor (type);
  if (editor)
  {
    editor->Fill (outputProp, triggerFact);
    detailGrid->Collapse (outputProp);
  }
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

