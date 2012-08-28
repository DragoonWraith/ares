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
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "edcommon/uitools.h"
#include "edcommon/listctrltools.h"
#include "edcommon/inspect.h"
#include "edcommon/tools.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/iapp.h"

//--------------------------------------------------------------------------

iQuestManager* SequencePanel::GetQuestManager () const
{
  return emode->GetQuestManager ();
}

long SequencePanel::GetSeqOpSelection () const
{
  if (!sequence) return 0;
  return ListCtrlTools::GetFirstSelectedRow (operationsList);
}

iSeqOpFactory* SequencePanel::GetSeqOpFactory ()
{
  if (!sequence) return 0;
  long selection = ListCtrlTools::GetFirstSelectedRow (operationsList);
  if (selection == -1) return 0;
  return sequence->GetSeqOpFactory (selection);
}

void SequencePanel::SwitchSequence (iQuestFactory* questFact,
    iCelSequenceFactory* sequence)
{
  SequencePanel::questFact = questFact;
  SequencePanel::sequence = sequence;
  operations->Refresh ();
}

// -----------------------------------------------------------------------

using namespace Ares;

class SeqOpTypeValue : public Value
{
private:
  size_t idx;
  SequencePanel* sequencePanel;
  csString seqopType;

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

class DurationValue : public Value
{
private:
  size_t idx;
  SequencePanel* sequencePanel;
  csString duration;

public:
  DurationValue (SequencePanel* sequencePanel, size_t idx) : idx (idx), sequencePanel (sequencePanel) { }
  virtual ~DurationValue () { }
  virtual ValueType GetType () const { return VALUE_STRING; }
  virtual void SetStringValue (const char* s)
  {
    iCelSequenceFactory* sequence = sequencePanel->GetCurrentSequence ();
    if (!sequence) return;
    csString duration = sequence->GetSeqOpFactoryDuration (idx);
    if (duration != s)
    {
      sequence->UpdateSeqOpFactory (idx, sequence->GetSeqOpFactory (idx), s);
      FireValueChanged ();
    }
  }
  virtual const char* GetStringValue ()
  {
    iCelSequenceFactory* sequence = sequencePanel->GetCurrentSequence ();
    if (!sequence) return "";
    duration = sequence->GetSeqOpFactoryDuration (idx);
    return duration;
  }
};

class SeqOpCollectionValue : public StandardCollectionValue
{
private:
  SequencePanel* sequencePanel;

  Value* NewChild (const char* type, const char* duration, size_t index)
  {
    csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
    composite->AddChild ("type", NEWREF(SeqOpTypeValue,new SeqOpTypeValue(sequencePanel,index)));
    composite->AddChild ("duration", NEWREF(DurationValue,new DurationValue(sequencePanel,index)));
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
	if (name.StartsWith ("cel.seqops.")) name = name.Slice (11);
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

  virtual void FireValueChanged ()
  {
    StandardCollectionValue::FireValueChanged ();
    // @@@ This may be a bit too much. This also register a modification if we just look at the
    // sequence.
    sequencePanel->GetEntityMode ()->GetApplication ()->RegisterModification (
	sequencePanel->GetQuestFactory ()->QueryObject ());
  }

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
 * A composite value used for representing sequence parameters.
 */
template <class T>
class SequenceValue : public CompositeValue
{
protected:
  SequencePanel* sequencePanel;
  T* sequence;
  bool updating;

  T* GetSequence ()
  {
    iSeqOpFactory* seqopFactory = sequencePanel->GetSeqOpFactory ();
    if (!seqopFactory) return 0;
    csRef<T> s = scfQueryInterface<T> (seqopFactory);
    sequence = s;
    return sequence;
  }

  const bool GetBool (const char* name)
  {
    return GetChildByName (name)->GetBoolValue ();
  }
  const char* GetStr (const char* name)
  {
    return GetChildByName (name)->GetStringValue ();
  }
  void SetBool (const char* name, bool value)
  {
    GetChildByName (name)->SetBoolValue (value);
  }
  void SetStr (const char* name, const char* value)
  {
    GetChildByName (name)->SetStringValue (value);
  }

  void AddStringChildren (const char* n, ...)
  {
    AddChild (n, NEWREF(Value,new StringValue ()));
    va_list args;
    va_start (args, n);
    const char* c = va_arg (args, char*);
    while (c != (const char*)0)
    {
      AddChild (c, NEWREF(Value,new StringValue ()));
      c = va_arg (args, char*);
    }
    va_end (args);
  }

public:
  SequenceValue (SequencePanel* sequencePanel) :
    sequencePanel (sequencePanel), updating (false) { }
  virtual ~SequenceValue () { }
};

// -----------------------------------------------------------------------

/**
 * A composite value representing a debugprint seqop.
 */
class DebugPrintValue : public SequenceValue<iDebugPrintSeqOpFactory>
{
protected:
  virtual void ChildChanged (Value* child)
  {
    if (updating || !GetSequence ()) return;
    sequence->SetMessageParameter (GetStr ("message"));
  }

public:
  DebugPrintValue (SequencePanel* sequencePanel)
    : SequenceValue<iDebugPrintSeqOpFactory> (sequencePanel)
  {
    AddChild ("message", NEWREF(Value,new StringValue ()));
  }
  virtual ~DebugPrintValue () { }
  virtual void FireValueChanged ()
  {
    SequenceValue<iDebugPrintSeqOpFactory>::FireValueChanged ();
    if (!GetSequence ()) return;
    updating = true;
    SetStr ("message", sequence->GetMessage ());
    updating = false;
  }
};

/**
 * A composite value representing a seqop with color animation.
 */
template <class T>
class ColorAnimValue : public SequenceValue<T>
{
protected:
  virtual void ChildChanged (Value* child)
  {
    if (this->updating || !this->GetSequence ()) return;
    this->sequence->SetEntityParameter (this->GetStr ("entity"), this->GetStr ("tag"));
    this->sequence->SetRelColorParameter (this->GetStr ("relred"), this->GetStr ("relgreen"),
	this->GetStr ("relblue"));
    this->sequence->SetAbsColorParameter (this->GetStr ("absred"), this->GetStr ("absgreen"),
	this->GetStr ("absblue"));
  }

public:
  ColorAnimValue (SequencePanel* sequencePanel) : SequenceValue<T> (sequencePanel)
  {
    this->AddStringChildren ("entity", "tag", "relred", "relgreen", "relblue",
	"absred", "absgreen", "absblue", (const char*)0);
  }
  virtual ~ColorAnimValue () { }
  virtual void FireValueChanged ()
  {
    SequenceValue<T>::FireValueChanged ();
    if (!this->GetSequence ()) return;
    this->updating = true;
    this->SetStr ("entity", this->sequence->GetEntity ());
    this->SetStr ("tag", this->sequence->GetTag ());
    this->SetStr ("relred", this->sequence->GetRelColorRed ());
    this->SetStr ("relgreen", this->sequence->GetRelColorGreen ());
    this->SetStr ("relblue", this->sequence->GetRelColorBlue ());
    this->SetStr ("absred", this->sequence->GetAbsColorRed ());
    this->SetStr ("absgreen", this->sequence->GetAbsColorGreen ());
    this->SetStr ("absblue", this->sequence->GetAbsColorBlue ());
    this->updating = false;
  }
};

/**
 * A composite value representing a ambientmesh seqop.
 */
class AmbientMeshValue : public ColorAnimValue<iAmbientMeshSeqOpFactory>
{
public:
  AmbientMeshValue (SequencePanel* sequencePanel)
    : ColorAnimValue<iAmbientMeshSeqOpFactory> (sequencePanel) { }
  virtual ~AmbientMeshValue () { }
};

/**
 * A composite value representing a light seqop.
 */
class LightValue : public ColorAnimValue<iLightSeqOpFactory>
{
public:
  LightValue (SequencePanel* sequencePanel)
    : ColorAnimValue<iLightSeqOpFactory> (sequencePanel) { }
  virtual ~LightValue () { }
};

/**
 * A composite value representing a movepath seqop.
 */
class MovePathValue : public SequenceValue<iMovePathSeqOpFactory>
{
protected:
  virtual void ChildChanged (Value* child)
  {
    if (updating || !GetSequence ()) return;
    sequence->SetEntityParameter (GetStr ("entity"), GetStr ("tag"));
    // @@@ Todo path
  }

public:
  MovePathValue (SequencePanel* sequencePanel)
    : SequenceValue<iMovePathSeqOpFactory> (sequencePanel)
  {
    AddStringChildren ("entity", "tag", (const char*)0);
  }
  virtual ~MovePathValue () { }
  virtual void FireValueChanged ()
  {
    SequenceValue<iMovePathSeqOpFactory>::FireValueChanged ();
    if (!GetSequence ()) return;
    updating = true;
    SetStr ("entity", sequence->GetEntity ());
    SetStr ("tag", sequence->GetTag ());
    updating = false;
  }
};

/**
 * A composite value representing a property seqop.
 */
class PropertyValue : public SequenceValue<iPropertySeqOpFactory>
{
protected:
  virtual void ChildChanged (Value* child)
  {
    if (updating || !GetSequence ()) return;
    sequence->SetEntityParameter (GetStr ("entity"));
    sequence->SetPCParameter (GetStr ("pc"), GetStr ("tag"));
    sequence->SetPropertyParameter (GetStr ("property"));
    sequence->SetLongParameter (GetStr ("long"));
    sequence->SetFloatParameter (GetStr ("float"));
    sequence->SetVector3Parameter (GetStr ("vectorx"), GetStr ("vectory"),
	GetStr ("vectorz"));
    sequence->SetRelative (GetBool ("relative"));
  }

public:
  PropertyValue (SequencePanel* sequencePanel)
    : SequenceValue<iPropertySeqOpFactory> (sequencePanel)
  {
    AddStringChildren ("entity", "pc", "tag", "property", "long", "float",
	"vectorx", "vectory", "vectorz", (const char*)0);
    AddChild ("relative", NEWREF(Value,new BoolValue ()));
  }
  virtual ~PropertyValue () { }
  virtual void FireValueChanged ()
  {
    SequenceValue<iPropertySeqOpFactory>::FireValueChanged ();
    if (!GetSequence ()) return;
    updating = true;
    SetStr ("entity", sequence->GetEntity ());
    SetStr ("pc", sequence->GetPC ());
    SetStr ("tag", sequence->GetPCTag ());
    SetStr ("property", sequence->GetProperty ());
    SetStr ("float", sequence->GetFloat ());
    SetStr ("long", sequence->GetLong ());
    SetStr ("vectorx", sequence->GetVectorX ());
    SetStr ("vectory", sequence->GetVectorY ());
    SetStr ("vectorz", sequence->GetVectorZ ());
    SetBool ("relative", sequence->IsRelative ());
    updating = false;
  }
};

/**
 * A composite value representing a transform seqop.
 */
class TransformValue : public SequenceValue<iTransformSeqOpFactory>
{
protected:
  virtual void ChildChanged (Value* child)
  {
    if (updating || !GetSequence ()) return;
    sequence->SetEntityParameter (GetStr ("entity"), GetStr ("tag"));
    sequence->SetVectorParameter (GetStr ("vectorx"), GetStr ("vectory"),
	GetStr ("vectorz"));
    csString rotAxis = GetStr ("axis");
    int axis;
    if (rotAxis == "none") axis = -1;
    else if (rotAxis == "x") axis = 0;
    else if (rotAxis == "y") axis = 1;
    else axis = 2;
    sequence->SetRotationParameter (axis, GetStr ("angle"));
    
    csString r = GetStr ("reversed");
    sequence->SetReversed (r == "true");
  }

public:
  TransformValue (SequencePanel* sequencePanel)
    : SequenceValue<iTransformSeqOpFactory> (sequencePanel)
  {
    AddStringChildren ("entity", "tag", "axis", "angle",
	"vectorx", "vectory", "vectorz", "reversed", (const char*)0);
  }
  virtual ~TransformValue () { }
  virtual void FireValueChanged ()
  {
    SequenceValue<iTransformSeqOpFactory>::FireValueChanged ();
    if (!GetSequence ()) return;
    updating = true;
    SetStr ("entity", sequence->GetEntity ());
    SetStr ("tag", sequence->GetTag ());
    SetStr ("vectorx", sequence->GetVectorX ());
    SetStr ("vectory", sequence->GetVectorY ());
    SetStr ("vectorz", sequence->GetVectorZ ());
    static const char* axisStr[] = { "none", "0", "1", "2" };
    int axis = sequence->GetRotationAxis ();
    if (axis >= -1)
      SetStr ("axis", axisStr[axis+1]);
    SetStr ("angle", sequence->GetRotationAngle ());
    SetStr ("reversed", sequence->IsReversed () ? "true" : "false");
    updating = false;
  }
};

// -----------------------------------------------------------------------

SequencePanel::SequencePanel (wxWindow* parent, iUIManager* uiManager,
    EntityMode* emode) :
  View (this), uiManager (uiManager), emode (emode)
{
  sequence = 0;
  pl = emode->GetPL ();
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("SequencePanel"));

  newopDialog = uiManager->CreateDialog (this, "New Operation");
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
  Bind (operationsSelectedValue->GetChildByName ("duration"), "duration_Text");

  csRef<Value> v;
  v.AttachNew (new DebugPrintValue (this));
  Bind (v, "debugprintPanel");
  Signal (operationsSelectedValue, v);

  v.AttachNew (new AmbientMeshValue (this));
  Bind (v, "ambientmeshPanel");
  Signal (operationsSelectedValue, v);

  v.AttachNew (new LightValue (this));
  Bind (v, "lightPanel");
  Signal (operationsSelectedValue, v);

  v.AttachNew (new MovePathValue (this));
  Bind (v, "movepathPanel");
  Signal (operationsSelectedValue, v);

  v.AttachNew (new PropertyValue (this));
  Bind (v, "propertyPanel");
  Signal (operationsSelectedValue, v);

  v.AttachNew (new TransformValue (this));
  Bind (v, "transformPanel");
  Signal (operationsSelectedValue, v);
}

SequencePanel::~SequencePanel ()
{
}


