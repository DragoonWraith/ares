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

QuestEditorSupportTrigger::QuestEditorSupportTrigger (const char* name, EntityMode* emode)
  : GridSupport (name, emode)
{
#if 0
  RegisterEditor (new PcEditorSupportQuest (emode));
  RegisterEditor (new PcEditorSupportWire (emode));
  RegisterEditor (new PcEditorSupportOldCamera (emode));
  RegisterEditor (new PcEditorSupportDynworld (emode));
  RegisterEditor (new PcEditorSupportSpawn (emode));
  RegisterEditor (new PcEditorSupportInventory (emode));
  RegisterEditor (new PcEditorSupportMessenger (emode));
  RegisterEditor (new PcEditorSupportProperties (emode));
  RegisterEditor (new PcEditorSupportTrigger (emode));
#endif
}

void QuestEditorSupportTrigger::Fill (wxPGProperty* pcProp, iTriggerFactory* triggerFact)
{
  csString type = emode->GetTriggerType (triggerFact);
  csString s;
  s.Format ("Trigger (%s)", type.GetData ());
  wxPGProperty* outputProp = AppendStringPar (pcProp, s, "Trigger", "<composed>");
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

  triggerEditor.AttachNew (new QuestEditorSupportTrigger ("main", emode));
}

void QuestEditorSupportMain::FillResponses (wxPGProperty* stateProp, size_t idx, iQuestStateFactory* state)
{
  csString s;
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    printf ("%d\n", i); fflush (stdout);
    iQuestTriggerResponseFactory* response = responses->Get (i);
    s.Format ("%d:Response:%d", int (idx), int (i));
    wxPGProperty* responseProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("Response"), wxString::FromUTF8 (s)));
    iTriggerFactory* triggerFact = response->GetTriggerFactory ();
    triggerEditor->Fill (responseProp, triggerFact);
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
    wxPGProperty* oninitProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("OnExit"), wxString::FromUTF8 (s)));
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

