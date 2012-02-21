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

#include "rewardpanel.h"
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

BEGIN_EVENT_TABLE(RewardPanel, wxPanel)
  EVT_CHOICEBOOK_PAGE_CHANGED (XRCID("rewardChoicebook"), RewardPanel :: OnChoicebookPageChange)

  EVT_CHECKBOX (XRCID("toggle_Cp_Check"), RewardPanel :: OnUpdateEvent)

  EVT_TEXT_ENTER (XRCID("bool_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("childEntity_In_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("childTag_In_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("state_Ns_Combo"), RewardPanel :: OnUpdateEvent)
  EVT_COMBOBOX (XRCID("state_Ns_Combo"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("class_Ac_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("class_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("class_De_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("class_Me_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("class_Se_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("class_Sf_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("delay_Cs_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("delay_Se_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("diff_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Ac_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_De_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_In_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Me_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Se_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("entity_Sf_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("float_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("id_Ac_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("id_Me_Combo"), RewardPanel :: OnUpdateEvent)
  EVT_COMBOBOX (XRCID("id_Me_Combo"), RewardPanel :: OnUpdateMessageCombo)
  EVT_TEXT_ENTER (XRCID("long_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("message_Dp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("name_Ce_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("pc_Ac_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("pc_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("property_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("sequence_Cs_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("sequence_Se_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("sequence_Sf_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("string_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Ac_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Cp_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_In_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Se_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("tag_Sf_Text"), RewardPanel :: OnUpdateEvent)
  EVT_TEXT_ENTER (XRCID("template_Ce_Text"), RewardPanel :: OnUpdateEvent)

END_EVENT_TABLE()

//--------------------------------------------------------------------------

void RewardPanel::OnUpdateEvent (wxCommandEvent& event)
{
  printf ("Update Reward!\n"); fflush (stdout);
  UpdateReward ();
}

void RewardPanel::OnUpdateMessageCombo (wxCommandEvent& event)
{
  printf ("Update Message Combo!\n"); fflush (stdout);
  if (reward)
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (reward);
    csString msg = UITools::GetValue (this, "id_Me_Combo");
    if (msg != tf->GetID ())
    {
      tf->SetIDParameter (msg);
      while (tf->GetParameterCount () > 0)
	tf->RemoveParameter (tf->GetParameterID (0));
      if (msg == "ares.controller.Message")
      {
	tf->AddParameter (CEL_DATA_STRING, pl->FetchStringID ("message"), "... message ...");
	tf->AddParameter (CEL_DATA_FLOAT, pl->FetchStringID ("timeout"), "2.0");
      }
      else if (msg == "ares.controller.Spawn")
      {
	tf->AddParameter (CEL_DATA_STRING, pl->FetchStringID ("factory"), "... factory ...");
      }
      else if (msg == "ares.controller.CreateEntity")
      {
	tf->AddParameter (CEL_DATA_STRING, pl->FetchStringID ("template"), "... template ...");
	tf->AddParameter (CEL_DATA_STRING, pl->FetchStringID ("name"), "... name ...");
      }
      messageParameters->Refresh ();
    }
  }
  UpdateReward ();
}

void RewardPanel::OnChoicebookPageChange (wxChoicebookEvent& event)
{
  printf ("Update choicebook!\n"); fflush (stdout);
  UpdateReward ();
}

csString RewardPanel::GetCurrentRewardType ()
{
  if (!reward) return "";
  csString name = reward->GetRewardType ()->GetName ();
  csString nameS = name;
  if (nameS.StartsWith ("cel.rewards."))
    nameS = name.GetData ()+12;
  return nameS;
}

void RewardPanel::SwitchReward (iQuestFactory* questFact,
    iRewardFactoryArray* array, size_t idx,
    iRewardFactory* reward)
{
  RewardPanel::questFact = questFact;
  RewardPanel::rewardArray = array;
  RewardPanel::rewardIdx = idx;
  RewardPanel::reward = reward;
  UITools::SwitchPage (this, "rewardChoicebook", GetCurrentRewardType ());
  UpdatePanel ();
}

void RewardPanel::UpdatePanel ()
{
  csString type = GetCurrentRewardType ();
  if (type == "newstate")
  {
    // @@@ Add support for setting states on other entities!
    csRef<iNewStateQuestRewardFactory> tf = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    UITools::ClearChoices (this, "state_Ns_Combo");
    csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
    while (it->HasNext ())
    {
      iQuestStateFactory* state = it->Next ();
      UITools::AddChoices (this, "state_Ns_Combo", state->GetName (), (const char*)0);
    }
    UITools::SetValue (this, "state_Ns_Combo", tf->GetStateParameter ());
  }
  else if (type == "action")
  {
    csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (reward);
    UITools::SetValue (this, "entity_Ac_Text", tf->GetEntity ());
    UITools::SetValue (this, "class_Ac_Text", tf->GetClass ());
    UITools::SetValue (this, "id_Ac_Text", tf->GetID ());
    UITools::SetValue (this, "pc_Ac_Text", tf->GetPropertyClass ());
    UITools::SetValue (this, "tag_Ac_Text", tf->GetTag ());
    actionParameters->Refresh ();
  }
  else if (type == "changeproperty")
  {
    csRef<iChangePropertyRewardFactory> tf = scfQueryInterface<iChangePropertyRewardFactory> (reward);
    UITools::SetValue (this, "entity_Cp_Text", tf->GetEntity ());
    UITools::SetValue (this, "class_Cp_Text", tf->GetClass ());
    UITools::SetValue (this, "pc_Cp_Text", tf->GetPC ());
    UITools::SetValue (this, "tag_Cp_Text", tf->GetPCTag ());
    UITools::SetValue (this, "property_Cp_Text", tf->GetProperty ());
    UITools::SetValue (this, "string_Cp_Text", tf->GetString ());
    UITools::SetValue (this, "long_Cp_Text", tf->GetLong ());
    UITools::SetValue (this, "float_Cp_Text", tf->GetFloat ());
    UITools::SetValue (this, "bool_Cp_Text", tf->GetBool ());
    UITools::SetValue (this, "diff_Cp_Text", tf->GetDiff ());
    wxCheckBox* check = XRCCTRL (*this, "toggle_Cp_Check", wxCheckBox);
    check->SetValue (tf->IsToggle ());
  }
  else if (type == "createentity")
  {
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    UITools::SetValue (this, "template_Ce_Text", tf->GetEntityTemplate ());
    UITools::SetValue (this, "name_Ce_Text", tf->GetName ());
    createentityParameters->Refresh ();
  }
  else if (type == "destroyentity")
  {
    csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
    UITools::SetValue (this, "entity_De_Text", tf->GetEntity ());
    UITools::SetValue (this, "class_De_Text", tf->GetClass ());
  }
  else if (type == "debugprint")
  {
    csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (reward);
    UITools::SetValue (this, "message_Dp_Text", tf->GetMessage ());
  }
  else if (type == "inventory")
  {
    csRef<iInventoryRewardFactory> tf = scfQueryInterface<iInventoryRewardFactory> (reward);
    UITools::SetValue (this, "entity_In_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_In_Text", tf->GetTag ());
    UITools::SetValue (this, "childEntity_In_Text", tf->GetChildEntity ());
    UITools::SetValue (this, "childTag_In_Text", tf->GetChildTag ());
  }
  else if (type == "message")
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (reward);
    UITools::SetValue (this, "entity_Me_Text", tf->GetEntity ());
    UITools::SetValue (this, "class_Me_Text", tf->GetClass ());
    UITools::ClearChoices (this, "id_Me_Combo");
    UITools::AddChoices (this, "id_Me_Combo",
	"ares.controller.Message",
	"ares.controller.StartDrag",
	"ares.controller.StopDrag",
	"ares.controller.Examine",
	"ares.controller.Pickup",
	"ares.controller.Activate",
	"ares.controller.Spawn",
	"ares.controller.CreateEntity",
	(const char*)0);
    UITools::SetValue (this, "id_Me_Combo", tf->GetID ());
    messageParameters->Refresh ();
  }
  else if (type == "cssequence")
  {
    csRef<iCsSequenceRewardFactory> tf = scfQueryInterface<iCsSequenceRewardFactory> (reward);
    UITools::SetValue (this, "sequence_Cs_Text", tf->GetSequence ());
    UITools::SetValue (this, "delay_Cs_Text", tf->GetDelay ());
  }
  else if (type == "sequence")
  {
    csRef<iSequenceRewardFactory> tf = scfQueryInterface<iSequenceRewardFactory> (reward);
    UITools::SetValue (this, "entity_Se_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Se_Text", tf->GetTag ());
    UITools::SetValue (this, "class_Se_Text", tf->GetClass ());
    UITools::SetValue (this, "sequence_Se_Text", tf->GetSequence ());
    UITools::SetValue (this, "delay_Se_Text", tf->GetDelay ());
  }
  else if (type == "sequencefinish")
  {
    csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (reward);
    UITools::SetValue (this, "entity_Sf_Text", tf->GetEntity ());
    UITools::SetValue (this, "tag_Sf_Text", tf->GetTag ());
    UITools::SetValue (this, "class_Sf_Text", tf->GetClass ());
    UITools::SetValue (this, "sequence_Sf_Text", tf->GetSequence ());
  }
  else
  {
    printf ("Internal error: unknown type '%s'\n", type.GetData ());
  }
}

void RewardPanel::UpdateReward ()
{
  if (!reward) return;
  wxChoicebook* book = XRCCTRL (*this, "rewardChoicebook", wxChoicebook);
  int pageSel = book->GetSelection ();
  if (pageSel == wxNOT_FOUND)
  {
    uiManager->Error ("Internal error! Page not found!");
    return;
  }
  wxString pageTxt = book->GetPageText (pageSel);
  iQuestManager* questMgr = emode->GetQuestManager ();
  csString type = (const char*)pageTxt.mb_str (wxConvUTF8);
  if (type != GetCurrentRewardType ())
  {
    iRewardType* rewardtype = questMgr->GetRewardType ("cel.rewards."+type);
    csRef<iRewardFactory> rewardfact = rewardtype->CreateRewardFactory ();
    rewardArray->Put (rewardIdx, rewardfact);
    reward = rewardfact;
    UpdatePanel ();
  }
  else
  {
    if (type == "newstate")
    {
      // @@@ Support state for other entities.
      csRef<iNewStateQuestRewardFactory> tf = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
      tf->SetStateParameter (UITools::GetValue (this, "state_Ns_Combo"));
    }
    else if (type == "action")
    {
      csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (reward);
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Ac_Text"));
      tf->SetClassParameter (UITools::GetValue (this, "class_Ac_Text"));
      tf->SetIDParameter (UITools::GetValue (this, "id_Ac_Text"));
      tf->SetPropertyClassParameter (UITools::GetValue (this, "pc_Ac_Text"));
      tf->SetTagParameter (UITools::GetValue (this, "tag_Ac_Text"));
    }
    else if (type == "changeproperty")
    {
      csRef<iChangePropertyRewardFactory> tf = scfQueryInterface<iChangePropertyRewardFactory> (reward);
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Cp_Text"));
      tf->SetClassParameter (UITools::GetValue (this, "class_Cp_Text"));
      tf->SetPCParameter (UITools::GetValue (this, "pc_Cp_Text"),
	  UITools::GetValue (this, "tag_Cp_Text"));
      tf->SetPropertyParameter (UITools::GetValue (this, "property_Cp_Text"));
      tf->SetStringParameter (UITools::GetValue (this, "string_Cp_Text"));
      tf->SetLongParameter (UITools::GetValue (this, "long_Cp_Text"));
      tf->SetFloatParameter (UITools::GetValue (this, "float_Cp_Text"));
      tf->SetBoolParameter (UITools::GetValue (this, "bool_Cp_Text"));
      tf->SetDiffParameter (UITools::GetValue (this, "diff_Cp_Text"));
      wxCheckBox* check = XRCCTRL (*this, "toggle_Cp_Check", wxCheckBox);
      tf->SetToggle (check->GetValue ());
    }
    else if (type == "createentity")
    {
      csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (reward);
      tf->SetEntityTemplateParameter (UITools::GetValue (this, "template_Ce_Text"));
      tf->SetNameParameter (UITools::GetValue (this, "name_Ce_Text"));
    }
    else if (type == "destroyentity")
    {
      csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
      tf->SetEntityParameter (UITools::GetValue (this, "entity_De_Text"));
      tf->SetClassParameter (UITools::GetValue (this, "class_De_Text"));
    }
    else if (type == "debugprint")
    {
      csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (reward);
      tf->SetMessageParameter (UITools::GetValue (this, "message_Dp_Text"));
    }
    else if (type == "inventory")
    {
      csRef<iInventoryRewardFactory> tf = scfQueryInterface<iInventoryRewardFactory> (reward);
      tf->SetEntityParameter (UITools::GetValue (this, "entity_In_Text"),
	  UITools::GetValue (this, "tag_In_Text"));
      tf->SetChildEntityParameter (UITools::GetValue (this, "childEntity_In_Text"),
	  UITools::GetValue (this, "childTag_In_Text"));
    }
    else if (type == "message")
    {
      csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (reward);
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Me_Text"));
      tf->SetClassParameter (UITools::GetValue (this, "class_Me_Text"));
      tf->SetIDParameter (UITools::GetValue (this, "id_Me_Combo"));
    }
    else if (type == "cssequence")
    {
      csRef<iCsSequenceRewardFactory> tf = scfQueryInterface<iCsSequenceRewardFactory> (reward);
      tf->SetSequenceParameter (UITools::GetValue (this, "sequence_Cs_Text"));
      tf->SetDelayParameter (UITools::GetValue (this, "delay_Cs_Text"));
    }
    else if (type == "sequence")
    {
      csRef<iSequenceRewardFactory> tf = scfQueryInterface<iSequenceRewardFactory> (reward);
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Se_Text"),
	  UITools::GetValue (this, "tag_Se_Text"));
      tf->SetClassParameter (UITools::GetValue (this, "class_Se_Text"));
      tf->SetSequenceParameter (UITools::GetValue (this, "sequence_Se_Text"));
      tf->SetDelayParameter (UITools::GetValue (this, "delay_Se_Text"));
    }
    else if (type == "sequencefinish")
    {
      csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (reward);
      tf->SetEntityParameter (UITools::GetValue (this, "entity_Sf_Text"),
	  UITools::GetValue (this, "tag_Sf_Text"));
      tf->SetClassParameter (UITools::GetValue (this, "class_Sf_Text"));
      tf->SetSequenceParameter (UITools::GetValue (this, "sequence_Sf_Text"));
    }
    else
    {
      printf ("Internal error: unknown type '%s'\n", type.GetData ());
    }
  }
  emode->RefreshView ();
}

// -----------------------------------------------------------------------

using namespace Ares;

#if 0
class RewardTypeValue : public Value
{
private:
  size_t idx;
  RewardPanel* rewardPanel;
  csString rewardType;

public:
  SeqOpTypeValue (SequencePanel* sequencePanel, size_t idx) : idx (idx), sequencePanel (sequencePanel) { }
  virtual ~SeqOpTypeValue () { }
  virtual ValueType GetType () const { return VALUE_STRING; }
  virtual void SetStringValue (const char* s)
  {
    iQuestManager* questMgr = sequencePanel->GetQuestManager ();
    iCelSequenceFactory* sequence = sequencePanel->GetCurrentSequence ();
    if (!sequence) return;
    csRef<iSeqOpFactory> seqopFact = sequence->GetSeqOpFactory (idx);
    if (seqopFact) seqopType = seqopFact->GetSeqOpType ()->GetName ();
    else seqopType = "delay";
    if (seqopType.StartsWith ("cel.seqops.")) seqopType = seqopType.Slice (11);
    printf ("s=%s seqopType=%s\n", s, seqopType.GetData ()); fflush (stdout);
    if (seqopType != s)
    {
      if (csString ("delay") != s)
      {
        iSeqOpType* seqoptype = questMgr->GetSeqOpType (csString ("cel.seqops.")+s);
        seqopFact = seqoptype->CreateSeqOpFactory ();
      }
      sequence->UpdateSeqOpFactory (idx, seqopFact, sequence->GetSeqOpFactoryDuration (idx));
      FireValueChanged ();
    }
  }
  virtual const char* GetStringValue ()
  {
    iCelSequenceFactory* sequence = sequencePanel->GetCurrentSequence ();
    if (!sequence) return "";
    iSeqOpFactory* seqopFact = sequence->GetSeqOpFactory (idx);
    if (seqopFact) seqopType = seqopFact->GetSeqOpType ()->GetName ();
    else seqopType = "delay";
    if (seqopType.StartsWith ("cel.seqops.")) seqopType = seqopType.Slice (11);
    return seqopType;
  }
};
#endif

// -----------------------------------------------------------------------

class CreateEntityParametersCollectionValue : public StandardCollectionValue
{
private:
  RewardPanel* rewardPanel;

  csRef<iCreateEntityRewardFactory> GetReward ()
  {
    iRewardFactory* reward = rewardPanel->GetCurrentReward ();
    if (!reward) return 0;
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    return tf;
  }

  Value* NewChild (const char* name, const char* value)
  {
    csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
    composite->AddChild ("name", NEWREF(StringValue,new StringValue(name)));
    composite->AddChild ("value", NEWREF(StringValue,new StringValue(value)));
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
    csRef<iCreateEntityRewardFactory> tf = GetReward ();
    if (!tf) return;
    for (size_t i = 0 ; i < tf->GetParameterCount () ; i++)
    {
      csString name = tf->GetParameterName (i);
      csString value = tf->GetParameterValue (i);
      NewChild (name, value);
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  CreateEntityParametersCollectionValue (RewardPanel* rewardPanel) : rewardPanel (rewardPanel) { }
  virtual ~CreateEntityParametersCollectionValue () { }

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

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Par*]";
    dump += Ares::StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

// -----------------------------------------------------------------------

template <class T>
class TypedParametersCollectionValue : public StandardCollectionValue
{
private:
  RewardPanel* rewardPanel;

  csRef<T> GetReward ()
  {
    iRewardFactory* reward = rewardPanel->GetCurrentReward ();
    if (!reward) return 0;
    csRef<T> tf = scfQueryInterface<T> (reward);
    return tf;
  }

  Value* NewChild (const char* name, const char* value, const char* type)
  {
    csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
    composite->AddChild ("name", NEWREF(StringValue,new StringValue(name)));
    composite->AddChild ("value", NEWREF(StringValue,new StringValue(value)));
    composite->AddChild ("type", NEWREF(StringValue,new StringValue(type)));
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
    iRewardFactory* reward = rewardPanel->GetCurrentReward ();
    if (!reward) return;
    csRef<T> tf = scfQueryInterface<T> (reward);
    if (!tf) return;
    for (size_t i = 0 ; i < tf->GetParameterCount () ; i++)
    {
      csStringID id = tf->GetParameterID (i);
      csString name = rewardPanel->GetPL ()->FetchString (id);
      csString value = tf->GetParameterValue (i);
      celDataType type = tf->GetParameterType (i);
      csString typeS = InspectTools::TypeToString (type);
      NewChild (name, value, typeS);
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  TypedParametersCollectionValue (RewardPanel* rewardPanel) : rewardPanel (rewardPanel) { }
  virtual ~TypedParametersCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    csRef<T> tf = GetReward ();
    if (!tf) return false;
    Value* nameValue = child->GetChildByName ("name");
    csString name = nameValue->GetStringValue ();
    csStringID id = rewardPanel->GetPL ()->FetchStringID (name);
    tf->RemoveParameter (id);
    dirty = true;
    FireValueChanged ();
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    csRef<T> tf = GetReward ();
    if (!tf) return 0;
    csString name = suggestion.Get ("name", (const char*)0);
    csString value = suggestion.Get ("value", (const char*)0);
    csString type = suggestion.Get ("type", (const char*)0);
    csStringID id = rewardPanel->GetPL ()->FetchStringID (name);
    if (!tf->AddParameter (InspectTools::StringToType (type), id, value))
      dirty = true;	// Force refresh because we did an update.
    Value* child = NewChild (name, value, type);
    FireValueChanged ();
    return child;
  }
  virtual bool UpdateValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    csRef<T> tf = GetReward ();
    if (!tf) return false;
    csString name = suggestion.Get ("name", (const char*)0);
    csString value = suggestion.Get ("value", (const char*)0);
    csString type = suggestion.Get ("type", (const char*)0);
    csStringID id = rewardPanel->GetPL ()->FetchStringID (name);
    if (!tf->AddParameter (InspectTools::StringToType (type), id, value))
      dirty = true;	// Force refresh because we did an update.
    selectedValue->GetChildByName ("name")->SetStringValue (name);
    selectedValue->GetChildByName ("value")->SetStringValue (value);
    selectedValue->GetChildByName ("type")->SetStringValue (type);
    FireValueChanged ();
    return true;
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Par*]";
    dump += Ares::StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

// -----------------------------------------------------------------------

class ActionParametersCollectionValue : public TypedParametersCollectionValue<iActionRewardFactory>
{
public:
  ActionParametersCollectionValue (RewardPanel* rewardPanel) : TypedParametersCollectionValue<iActionRewardFactory> (rewardPanel) { }
  virtual ~ActionParametersCollectionValue () { }
};

// -----------------------------------------------------------------------

class MessageParametersCollectionValue : public TypedParametersCollectionValue<iMessageRewardFactory>
{
public:
  MessageParametersCollectionValue (RewardPanel* rewardPanel) : TypedParametersCollectionValue<iMessageRewardFactory> (rewardPanel) { }
  virtual ~MessageParametersCollectionValue () { }
};

// -----------------------------------------------------------------------

RewardPanel::RewardPanel (wxWindow* parent, UIManager* uiManager,
    EntityMode* emode) :
  View (this), uiManager (uiManager), emode (emode)
{
  reward = 0;
  pl = uiManager->GetApp ()->GetAresView ()->GetPL ();
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("RewardPanel"));

  // Message parameters.
  DefineHeading ("parameters_Me_List", "Name,Value,Type", "name,value,type");
  messageParameters.AttachNew (new MessageParametersCollectionValue (this));
  Bind (messageParameters, "parameters_Me_List");
  messageDialog = new UIDialog (this, "Create Parameter");
  messageDialog->AddRow ();
  messageDialog->AddLabel ("Name:");
  messageDialog->AddText ("name");
  messageDialog->AddRow ();
  messageDialog->AddLabel ("Value:");
  messageDialog->AddText ("value");
  messageDialog->AddRow ();
  messageDialog->AddLabel ("Type:");
  messageDialog->AddChoice ("type", "string", "float", "long", "bool", "vector2",
      "vector3", "color", (const char*)0);
  wxListCtrl* messageList = XRCCTRL (*this, "parameters_Me_List", wxListCtrl);
  AddAction (messageList, NEWREF(Action, new NewChildDialogAction (messageParameters, messageDialog)));
  AddAction (messageList, NEWREF(Action, new EditChildDialogAction (messageParameters, messageDialog)));
  AddAction (messageList, NEWREF(Action, new DeleteChildAction (messageParameters)));

  // Action parameters.
  DefineHeading ("parameters_Ac_List", "Name,Value,Type", "name,value,type");
  actionParameters.AttachNew (new ActionParametersCollectionValue (this));
  Bind (actionParameters, "parameters_Ac_List");
  wxListCtrl* actionList = XRCCTRL (*this, "parameters_Ac_List", wxListCtrl);
  AddAction (actionList, NEWREF(Action, new NewChildDialogAction (actionParameters, messageDialog)));
  AddAction (actionList, NEWREF(Action, new DeleteChildAction (actionParameters)));

  // Create entity parameters.
  DefineHeading ("parameters_Ce_List", "Name,Value", "name,value");
  createentityParameters.AttachNew (new CreateEntityParametersCollectionValue (this));
  Bind (createentityParameters, "parameters_Ce_List");
  createentityDialog = new UIDialog (this, "Create Entity Parameter");
  createentityDialog->AddRow ();
  createentityDialog->AddLabel ("Name:");
  createentityDialog->AddText ("name");
  createentityDialog->AddRow ();
  createentityDialog->AddLabel ("Value:");
  createentityDialog->AddText ("value");
  wxListCtrl* createentityList = XRCCTRL (*this, "parameters_Ce_List", wxListCtrl);
  AddAction (createentityList, NEWREF(Action, new NewChildDialogAction (createentityParameters, createentityDialog)));
  AddAction (createentityList, NEWREF(Action, new DeleteChildAction (createentityParameters)));
}

RewardPanel::~RewardPanel ()
{
  delete createentityDialog;
  delete messageDialog;
}


