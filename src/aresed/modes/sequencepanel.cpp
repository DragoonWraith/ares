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
#include "../ui/listctrltools.h"
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

END_EVENT_TABLE()

//--------------------------------------------------------------------------

void SequencePanel::OnChoicebookPageChange (wxChoicebookEvent& event)
{
  //UpdateSequence ();
}

iQuestManager* SequencePanel::GetQuestManager () const
{
  return emode->GetQuestManager ();
}

iSeqOpFactory* SequencePanel::GetSeqOpFactory ()
{
  if (!sequence) return 0;
  long selection = ListCtrlTools::GetFirstSelectedRow (operationsList);
  if (selection == -1) return 0;
  return sequence->GetSeqOpFactory (selection);
}

void SequencePanel::SwitchSequence (iCelSequenceFactory* sequence)
{
  SequencePanel::sequence = sequence;
  operations->Refresh ();
}

// -----------------------------------------------------------------------

using namespace Ares;

class SeqOpCollectionValue : public StandardCollectionValue
{
private:
  SequencePanel* sequencePanel;

  Value* NewChild (const char* type, const char* duration, size_t index)
  {
    csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
    composite->AddChild ("type", NEWREF(StringValue,new StringValue(type)));
    composite->AddChild ("duration", NEWREF(StringValue,new StringValue(duration)));
    composite->AddChild ("index", NEWREF(LongValue,new LongValue(index)));
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
      NewChild (name, duration, i);
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  SeqOpCollectionValue (SequencePanel* sequencePanel) : sequencePanel (sequencePanel) { }
  virtual ~SeqOpCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    iCelSequenceFactory* seqFact = sequencePanel->GetCurrentSequence ();
    if (!seqFact) return false;
    Value* indexValue = child->GetChildByName ("index");
    seqFact->RemoveSeqOpFactory (indexValue->GetLongValue ());
    dirty = true;
    FireValueChanged ();
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    iCelSequenceFactory* seqFact = sequencePanel->GetCurrentSequence ();
    if (!seqFact) return 0;
    csString type = suggestion.Get ("type", (const char*)0);
    csString duration = suggestion.Get ("duration", (const char*)0);
    if (type == "delay")
      seqFact->AddDelay (duration);
    else
    {
      iQuestManager* questMgr = sequencePanel->GetQuestManager ();
      iSeqOpType* seqoptype = questMgr->GetSeqOpType ("cel.seqops."+type);
      csRef<iSeqOpFactory> seqopFact = seqoptype->CreateSeqOpFactory ();
      seqFact->AddSeqOpFactory (seqopFact, duration);
    }
    Value* child = NewChild (type, duration, seqFact->GetSeqOpFactoryCount ()-1);
    FireValueChanged ();
    return child;
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[SeqOp*]";
    dump += Ares::StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

// -----------------------------------------------------------------------

/**
 * A composite value representing a debugprint seqop.
 */
class DebugPrintValue : public CompositeValue
{
private:
  SequencePanel* sequencePanel;

protected:
  virtual void ChildChanged (Value* child)
  {
    iSeqOpFactory* seqopFactory = sequencePanel->GetSeqOpFactory ();
    if (!seqopFactory) return;
    csRef<iDebugPrintSeqOpFactory> d = scfQueryInterface<iDebugPrintSeqOpFactory> (seqopFactory);
    if (!d) return;
    d->SetMessageParameter (GetChildByName ("message")->GetStringValue ());
  }

public:
  DebugPrintValue (SequencePanel* sequencePanel) : sequencePanel (sequencePanel)
  {
    AddChild ("message", NEWREF(Value,new StringValue ()));
  }
  virtual ~DebugPrintValue () { }
  virtual void FireValueChanged ()
  {
    CompositeValue::FireValueChanged ();
    iSeqOpFactory* seqopFactory = sequencePanel->GetSeqOpFactory ();
    if (!seqopFactory) return;
    csRef<iDebugPrintSeqOpFactory> d = scfQueryInterface<iDebugPrintSeqOpFactory> (seqopFactory);
    if (!d) return;
    GetChildByName ("message")->SetStringValue (d->GetMessage ());
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

  newopDialog = new UIDialog (this, "New Operation");
  newopDialog->AddRow ();
  newopDialog->AddLabel ("Type:");
  newopDialog->AddChoice ("type", "delay", "debugprint", "ambientmesh", "light", "movepath", "transform",
      (const char*)0);
  newopDialog->AddRow ();
  newopDialog->AddLabel ("Duration:");
  newopDialog->AddText ("duration");

  DefineHeading ("operations_List", "Type,Duration", "type,duration");
  operations.AttachNew (new SeqOpCollectionValue (this));
  Bind (operations, "operations_List");
  operationsList = XRCCTRL (*this, "operations_List", wxListCtrl);
  AddAction (operationsList, NEWREF(Action, new NewChildDialogAction (operations, newopDialog)));
  AddAction (operationsList, NEWREF(Action, new DeleteChildAction (operations)));

  operationsSelectedValue.AttachNew (new ListSelectedValue (operationsList, operations, VALUE_COMPOSITE));
  operationsSelectedValue->AddChild ("type", NEWREF(MirrorValue,new MirrorValue(VALUE_STRING)));
  operationsSelectedValue->AddChild ("duration", NEWREF(MirrorValue,new MirrorValue(VALUE_STRING)));
  operationsSelectedValue->AddChild ("index", NEWREF(MirrorValue,new MirrorValue(VALUE_LONG)));

  Bind (operationsSelectedValue->GetChildByName ("type"), "seqopChoicebook");

  csRef<Value> v;
  v.AttachNew (new DebugPrintValue (this));
  Bind (v, "debugprintPanel");
  Signal (operationsSelectedValue, v);

}

SequencePanel::~SequencePanel ()
{
  delete newopDialog;
}


