/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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

#include "sequencepanel.h"
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

BEGIN_EVENT_TABLE(SequencePanel, wxPanel)
  // down_Button, up_Button
  //<object class="wxChoice" name="axis_Tr_Choice">
  //<object class="wxListCtrl" name="operations_List">
  //<object class="wxListCtrl" name="path_List">
  //EVT_CHECKBOX (XRCID("relative_Pr_Check"), SequencePanel :: OnUpdateEvent)

  EVT_CHOICEBOOK_PAGE_CHANGED (XRCID("seqopChoicebook"), SequencePanel :: OnChoicebookPageChange)

  EVT_TEXT_ENTER (XRCID("absblue_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("absblue_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("absgreen_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("absgreen_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("absred_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("absred_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("angle_Tr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("duration_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Mp_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Tr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("float_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("long_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("message_Dp_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("pc_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("property_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("relblue_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("relblue_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("relgreen_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("relgreen_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("relred_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("relred_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Am_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Li_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Mp_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Tr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("vectorx_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("vectorx_Tr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("vectory_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("vectory_Tr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("vectorz_Pr_Text"), SequencePanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("vectorz_Tr_Text"), SequencePanel :: OnUpdateEvent)

END_EVENT_TABLE()

//--------------------------------------------------------------------------

void SequencePanel::OnUpdateEvent (wxCommandEvent& event)
{
  printf ("Update Sequence!\n"); fflush (stdout);
  UpdateSequence ();
}

void SequencePanel::OnChoicebookPageChange (wxChoicebookEvent& event)
{
  UpdateSequence ();
}

csString SequencePanel::GetCurrentSequenceType ()
{
  return "";
  //if (!sequence) return "";
  //csString name = reward->GetSequenceType ()->GetName ();
  //csString nameS = name;
  //if (nameS.StartsWith ("cel.rewards."))
    //nameS = name.GetData ()+12;
  //return nameS;
}

void SequencePanel::SwitchSequence (iCelSequenceFactory* sequence)
{
  SequencePanel::sequence = sequence;
  seqopCollection->Refresh ();
  //UITools::SwitchPage (this, "rewardChoicebook", GetCurrentSequenceType ());
  //UpdatePanel ();
}

void SequencePanel::UpdatePanel ()
{
  csString type = GetCurrentSequenceType ();
  //else if (type == "action")
  //{
    //csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (reward);
    //UITools::SetValue (this, "entity_Ac_Text", tf->GetEntity ());
    //UITools::SetValue (this, "class_Ac_Text", tf->GetClass ());
    //UITools::SetValue (this, "id_Ac_Text", tf->GetID ());
    //UITools::SetValue (this, "pc_Ac_Text", tf->GetPropertyClass ());
    //UITools::SetValue (this, "tag_Ac_Text", tf->GetTag ());
    //actionParameters->Refresh ();
  //}
  //else
  //{
    //printf ("Internal error: unknown type '%s'\n", type.GetData ());
  //}
}

void SequencePanel::UpdateSequence ()
{
  if (!sequence) return;
  return;
  wxChoicebook* book = XRCCTRL (*this, "seqopChoicebook", wxChoicebook);
  int pageSel = book->GetSelection ();
  if (pageSel == wxNOT_FOUND)
  {
    uiManager->Error ("Internal error! Page not found!");
    return;
  }
  wxString pageTxt = book->GetPageText (pageSel);
  iQuestManager* questMgr = emode->GetQuestManager ();
  csString type = (const char*)pageTxt.mb_str (wxConvUTF8);
  if (type != GetCurrentSequenceType ())
  {
    //iSeqOpType* seqoptype = questMgr->GetSeqOpType ("cel.seqops."+type);
    //csRef<iCelSequenceFactory> rewardfact = rewardtype->CreateRewardFactory ();
    //triggerResp->SetTriggerFactory (triggerfact);
    //UpdatePanel ();
  }
  else
  {
    //else if (type == "action")
    //{
      //csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (reward);
      //tf->SetEntityParameter (UITools::GetValue (this, "entity_Ac_Text"));
      //tf->SetClassParameter (UITools::GetValue (this, "class_Ac_Text"));
      //tf->SetIDParameter (UITools::GetValue (this, "id_Ac_Text"));
      //tf->SetPropertyClassParameter (UITools::GetValue (this, "pc_Ac_Text"));
      //tf->SetTagParameter (UITools::GetValue (this, "tag_Ac_Text"));
    //}
    //else
    //{
      //printf ("Internal error: unknown type '%s'\n", type.GetData ());
    //}
  }
  emode->RefreshView ();
}

// -----------------------------------------------------------------------

using namespace Ares;

class SeqOpCollectionValue : public StandardCollectionValue
{
private:
  SequencePanel* sequencePanel;

  Value* NewChild (const char* type, const char* duration)
  {
    csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
    composite->AddChild ("type", NEWREF(StringValue,new StringValue(type)));
    composite->AddChild ("duration", NEWREF(StringValue,new StringValue(duration)));
    children.Push (composite);
    composite->SetParent (this);
    return composite;
  }

protected:
  virtual void UpdateChildren ()
  {
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    iCelSequenceFactory* seqFact = sequencePanel->GetCurrentSequence ();
    if (!seqFact) return;
    for (size_t i = 0 ; i < seqFact->GetSeqOpFactoryCount () ; i++)
    {
      iSeqOpFactory* seqopFact = seqFact->GetSeqOpFactory (i);
      csString name;
      if (seqopFact)
      {
        name = seqopFact->GetSeqOpType ()->GetName ();
	if (name.StartsWith ("cel.seqops."))
	  name = name.Slice (11);
      }
      else
	name = "delay";
      csString duration = seqFact->GetSeqOpFactoryDuration (i);
      NewChild (name, duration);
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  SeqOpCollectionValue (SequencePanel* sequencePanel) : sequencePanel (sequencePanel) { }
  virtual ~SeqOpCollectionValue () { }

#if 0
  virtual bool DeleteValue (Value* child)
  {
    csRef<iCreateEntityRewardFactory> tf = GetReward ();
    if (!tf) return false;
    Value* nameValue = child->GetChildByName ("name");
    tf->RemoveParameter (nameValue->GetStringValue ());
    dirty = true;
    FireValueChanged ();
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    csRef<iCreateEntityRewardFactory> tf = GetReward ();
    if (!tf) return 0;
    csString name = suggestion.Get ("name", (const char*)0);
    csString value = suggestion.Get ("value", (const char*)0);
    if (!tf->AddParameter (name, value))
      dirty = true;	// Force refresh because we did an update.
    Value* child = NewChild (name, value);
    FireValueChanged ();
    return child;
  }
#endif

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[SeqOp*]";
    dump += Ares::StandardCollectionValue::Dump (verbose);
    return dump;
  }
};



// -----------------------------------------------------------------------

SequencePanel::SequencePanel (wxWindow* parent, UIManager* uiManager,
    EntityMode* emode) :
  View (this), uiManager (uiManager), emode (emode)
{
  sequence = 0;
  pl = uiManager->GetApp ()->GetAresView ()->GetPL ();
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("SequencePanel"));

  DefineHeading ("operations_List", "Type,Duration", "type,duration");
  seqopCollection.AttachNew (new SeqOpCollectionValue (this));
  Bind (seqopCollection, "operations_List");
}

SequencePanel::~SequencePanel ()
{
}


