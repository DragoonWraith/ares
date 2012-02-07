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

#include "triggerpanel.h"
#include "entitymode.h"
#include "../ui/uimanager.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "../apparesed.h"
#include "../ui/uitools.h"
#include "../tools/inspect.h"
#include "../tools/tools.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(TriggerPanel, wxPanel)
  EVT_CHOICEBOOK_PAGE_CHANGED (XRCID("triggerChoicebook"), TriggerPanel :: OnChoicebookPageChange)
  //EVT_TEXT_ENTER (XRCID("tagTextCtrl"), PropertyClassPanel :: OnUpdateEvent)

  //EVT_CHECKBOX (XRCID("spawnRepeatCheckBox"), PropertyClassPanel :: OnUpdateEvent)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void TriggerPanel::OnUpdateEvent (wxCommandEvent& event)
{
}

void TriggerPanel::OnChoicebookPageChange (wxChoicebookEvent& event)
{
}

void TriggerPanel::SwitchTrigger (const char* trigger)
{
  csString triggerS = trigger;
  if (triggerS.StartsWith ("cel.triggers."))
    triggerS = trigger+13;
  UITools::SwitchPage (this, "triggerChoicebook", triggerS);
  //if (triggerS == pctpl->GetName ()) return;
  //pctpl->SetName (pcType);
  //pctpl->RemoveAllProperties ();
  //emode->PCWasEdited (pctpl);
}

// -----------------------------------------------------------------------

TriggerPanel::TriggerPanel (wxWindow* parent, UIManager* uiManager,
    EntityMode* emode) :
  uiManager (uiManager), emode (emode)
{
  pl = uiManager->GetApp ()->GetAresView ()->GetPL ();
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("TriggerPanel"));
}

TriggerPanel::~TriggerPanel ()
{
}


