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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iNewStateQuestRewardFactory> tf = scfQueryInterface<iNewStateQuestRewardFactory> (rewardFact);
    if (field == "State")
    {
      tf->SetStateParameter (value);
      return REFRESH_NO;
    }
    else if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTagParameter ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntityParameter (), value);
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class RSDbPrint : public RewardSupport
{
public:
  RSDbPrint (EntityMode* emode) : RewardSupport ("DebugPrint", emode) { }
  virtual ~RSDbPrint () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (rewardFact);
    AppendStringPar (responseProp, "Message", "Message", tf->GetMessage ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iDebugPrintRewardFactory> tf = scfQueryInterface<iDebugPrintRewardFactory> (rewardFact);
    if (field == "Message")
    {
      tf->SetMessageParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
    AppendStringPar (responseProp, "ChildTag", "ChildTag", tf->GetChildTag ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iInventoryRewardFactory> tf = scfQueryInterface<iInventoryRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "E:ChildEntity")
    {
      tf->SetChildEntityParameter (value, tf->GetChildTag ());
      return REFRESH_NO;
    }
    else if (field == "ChildTag")
    {
      tf->SetChildEntityParameter (tf->GetChildEntity (), value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iSequenceRewardFactory> tf = scfQueryInterface<iSequenceRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Sequence")
    {
      tf->SetSequenceParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Delay")
    {
      tf->SetDelayParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iCsSequenceRewardFactory> tf = scfQueryInterface<iCsSequenceRewardFactory> (rewardFact);
    if (field == "Sequence")
    {
      tf->SetSequenceParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Delay")
    {
      tf->SetDelayParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class RSSeqFinish : public RewardSupport
{
public:
  RSSeqFinish (EntityMode* emode) : RewardSupport ("SequenceFinish", emode) { }
  virtual ~RSSeqFinish () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
    AppendStringPar (responseProp, "Sequence", "Sequence", tf->GetSequence ());	// @@@ Enum!
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iSequenceFinishRewardFactory> tf = scfQueryInterface<iSequenceFinishRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Sequence")
    {
      tf->SetSequenceParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

static bool ToBool (const char* value)
{
  csString lvalue = csString (value).Downcase ();
  return lvalue == "1" || lvalue == "true" || lvalue == "yes" || lvalue == "on";
}

class RSChangeProp : public RewardSupport
{
public:
  RSChangeProp (EntityMode* emode) : RewardSupport ("ChangeProperty", emode) { }
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iChangePropertyRewardFactory> tf = scfQueryInterface<iChangePropertyRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value);
      return REFRESH_NO;
    }
    else if (field == "PCTag")
    {
      tf->SetPCParameter (tf->GetPC (), value);
      return REFRESH_NO;
    }
    else if (field == "PC")
    {
      tf->SetPCParameter (value, tf->GetPCTag ());
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Property")
    {
      tf->SetPropertyParameter (value);
      return REFRESH_NO;
    }
    else if (field == "String")
    {
      tf->SetStringParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Long")
    {
      tf->SetLongParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Float")
    {
      tf->SetFloatParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Bool")
    {
      tf->SetBoolParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Diff")
    {
      tf->SetDiffParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Toggle")
    {
      tf->SetToggle (ToBool (value));
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class RSCreateEnt : public RewardSupport
{
public:
  RSCreateEnt (EntityMode* emode) : RewardSupport ("CreateEntity", emode) { }
  virtual ~RSCreateEnt () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Template", "T:", tf->GetEntityTemplate ());
    AppendStringPar (responseProp, "Name", "Name", tf->GetName ());
    // @@@ Add support for parameters
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iCreateEntityRewardFactory> tf = scfQueryInterface<iCreateEntityRewardFactory> (rewardFact);
    if (field == "T:Template")
    {
      tf->SetEntityTemplateParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Name")
    {
      tf->SetNameParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class RSDestroyEnt : public RewardSupport
{
public:
  RSDestroyEnt (EntityMode* emode) : RewardSupport ("DestroyEntity", emode) { }
  virtual ~RSDestroyEnt () { }

  virtual void Fill (wxPGProperty* responseProp, iRewardFactory* rewardFact)
  {
    csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (rewardFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Class", "Class", tf->GetClass ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iDestroyEntityRewardFactory> tf = scfQueryInterface<iDestroyEntityRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
    AppendBoolPar (responseProp, "Remove", "Remove", tf->IsRemove ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iChangeClassRewardFactory> tf = scfQueryInterface<iChangeClassRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Entities")
    {
      tf->SetEntitiesParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Remove")
    {
      tf->SetRemove (ToBool (value));
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iActionRewardFactory> tf = scfQueryInterface<iActionRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetTagParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    else if (field == "PC")
    {
      tf->SetPropertyClassParameter (value);
      return REFRESH_NO;
    }
    else if (field == "A:Action")
    {
      tf->SetIDParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
    // @@@ Support for message parameters!
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (rewardFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Entities")
    {
      tf->SetEntitiesParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Class")
    {
      tf->SetClassParameter (value);
      return REFRESH_NO;
    }
    else if (field == "A:Message")
    {
      tf->SetIDParameter (value);
      return REFRESH_NO;
    }
    // @@@ TODO Parameters
    return REFRESH_NOCHANGE;
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
  rewardtypesArray.Add (wxT ("DebugPrint"));
  rewardtypesArray.Add (wxT ("Inventory"));
  rewardtypesArray.Add (wxT ("Sequence"));
  rewardtypesArray.Add (wxT ("CsSequence"));
  rewardtypesArray.Add (wxT ("SequenceFinish"));
  rewardtypesArray.Add (wxT ("ChangeProperty"));
  rewardtypesArray.Add (wxT ("CreateEntity"));
  rewardtypesArray.Add (wxT ("DestroyEntity"));
  rewardtypesArray.Add (wxT ("ChangeClass"));
  rewardtypesArray.Add (wxT ("Action"));
  rewardtypesArray.Add (wxT ("Message"));
}

void RewardSupportDriver::Fill (wxPGProperty* responseProp,
    size_t idx, iRewardFactory* rewardFact)
{
  csString type = emode->GetRewardType (rewardFact);
  csString s;
  s.Format ("Reward:%d", int (idx));
  wxPGProperty* outputProp = AppendStringPar (responseProp, "Reward", s, "<composed>");
  AppendEnumPar (outputProp, "Type", "Type", rewardtypesArray,
      wxArrayInt (), rewardtypesArray.Index (wxString::FromUTF8 (type)));
  RewardSupport* editor = GetEditor (type);
  if (editor)
    editor->Fill (outputProp, rewardFact);
  detailGrid->Collapse (outputProp);
}

void RewardSupportDriver::FillRewards (wxPGProperty* responseProp,
    iRewardFactoryArray* rewards)
{
  for (size_t j = 0 ; j < rewards->GetSize () ; j++)
  {
    iRewardFactory* reward = rewards->Get (j);
    Fill (responseProp, j, reward);
  }
}

RefreshType RewardSupportDriver::Update (const csString& field,
    wxPGProperty* selectedProperty, iRewardFactoryArray* rewards, size_t idx)
{
  iRewardFactory* rewardFact = rewards->Get (idx);
  csString type = emode->GetRewardType (rewardFact);
  csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);
  printf ("Update '%s' in reward '%s' with value '%s'\n",
      field.GetData (), type.GetData (), value.GetData ()); fflush (stdout);
  if (field == "Type")
  {
    if (value != type)
    {
      value.Downcase ();
      iRewardType* rewardtype = emode->GetQuestManager ()->GetRewardType ("cel.rewards."+value);
      if (!rewardtype)
      {
	csPrintf ("INTERNAL ERROR: Unknown reward type '%s'!\n", value.GetData ());
	return REFRESH_NOCHANGE;
      }
      csRef<iRewardFactory> newRewardFact = rewardtype->CreateRewardFactory ();
      rewards->Put (idx, newRewardFact);
      return REFRESH_STATE;
    }
  }
  else
  {
    RewardSupport* editor = GetEditor (type);
    if (editor)
      return editor->Update (field, value, selectedProperty, rewardFact);
  }
  return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iTimeoutTriggerFactory> tf = scfQueryInterface<iTimeoutTriggerFactory> (triggerFact);
    if (field == "Timeout")
    {
      tf->SetTimeoutParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};


//---------------------------------------------------------------------------

class TSEnterSect : public TriggerSupport
{
public:
  TSEnterSect (EntityMode* emode) : TriggerSupport ("EnterSector", emode) { }
  virtual ~TSEnterSect () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Sector", "Sector", tf->GetSector ());	// @@@Button?
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iEnterSectorTriggerFactory> tf = scfQueryInterface<iEnterSectorTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "Sector")
    {
      tf->SetSectorParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class TSSeqFinish : public TriggerSupport
{
public:
  TSSeqFinish (EntityMode* emode) : TriggerSupport ("SequenceFinish", emode) { }
  virtual ~TSSeqFinish () { }

  virtual void Fill (wxPGProperty* responseProp, iTriggerFactory* triggerFact)
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (triggerFact);
    AppendButtonPar (responseProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (responseProp, "Tag", "Tag", tf->GetTag ());
    AppendStringPar (responseProp, "Sequence", "Sequence", tf->GetSequence ());	// @@@Combo!
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iSequenceFinishTriggerFactory> tf = scfQueryInterface<iSequenceFinishTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "Sequence")
    {
      tf->SetSequenceParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iInventoryTriggerFactory> tf = scfQueryInterface<iInventoryTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "E:Child")
    {
      tf->SetChildEntityParameter (value);
      return REFRESH_NO;
    }
    else if (field == "T:ChildTemplate")
    {
      tf->SetChildTemplateParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iMeshSelectTriggerFactory> tf = scfQueryInterface<iMeshSelectTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iMessageTriggerFactory> tf = scfQueryInterface<iMessageTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value);
      return REFRESH_NO;
    }
    else if (field == "A:Mask")
    {
      tf->SetMaskParameter (value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class TSPropertyChange : public TriggerSupport
{
public:
  TSPropertyChange (EntityMode* emode) : TriggerSupport ("PropertyChange", emode) { }
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iPropertyChangeTriggerFactory> tf = scfQueryInterface<iPropertyChangeTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "Property")
    {
      tf->SetPropertyParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Value")
    {
      tf->SetValueParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Operation")
    {
      tf->SetOperationParameter (value);
      return REFRESH_NO;
    }
    else if (field == "ChangeOnly")
    {
      tf->SetOnChangeOnly (ToBool (value));
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iTriggerTriggerFactory> tf = scfQueryInterface<iTriggerTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "Leave")
    {
      tf->EnableLeave (ToBool (value));
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
    AppendStringPar (responseProp, "Target Tag", "TargetTag", tf->GetTargetTag ());
    AppendStringPar (responseProp, "CheckTime", "CheckTimeTag", tf->GetChecktime ());
    AppendStringPar (responseProp, "Radius", "Radius", tf->GetRadius ());
    AppendVectorPar (responseProp, "Offset", "Offset",
	tf->GetOffsetX (), tf->GetOffsetY (), tf->GetOffsetZ ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (triggerFact);
    if (field == "E:Entity")
    {
      tf->SetEntityParameter (value, tf->GetTag ());
      return REFRESH_NO;
    }
    else if (field == "Tag")
    {
      tf->SetEntityParameter (tf->GetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "E:Target")
    {
      tf->SetTargetEntityParameter (value, tf->GetTargetTag ());
      return REFRESH_NO;
    }
    else if (field == "TargetTag")
    {
      tf->SetTargetEntityParameter (tf->GetTargetEntity (), value);
      return REFRESH_NO;
    }
    else if (field == "CheckTime")
    {
      tf->SetChecktimeParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Radius")
    {
      tf->SetRadiusParameter (value);
      return REFRESH_NO;
    }
    else if (field == "Offset.X")
    {
      tf->SetOffsetParameter (value, tf->GetOffsetY (), tf->GetOffsetZ ());
      return REFRESH_NO;
    }
    else if (field == "Offset.Y")
    {
      tf->SetOffsetParameter (tf->GetOffsetX (), value, tf->GetOffsetZ ());
      return REFRESH_NO;
    }
    else if (field == "Offset.Z")
    {
      tf->SetOffsetParameter (tf->GetOffsetX (), tf->GetOffsetY (), value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
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
  trigtypesArray.Add (wxT ("EnterSector"));
  trigtypesArray.Add (wxT ("SequenceFinish"));
  trigtypesArray.Add (wxT ("PropertyChange"));
  trigtypesArray.Add (wxT ("Trigger"));
  trigtypesArray.Add (wxT ("Watch"));
  trigtypesArray.Add (wxT ("Operation"));
  trigtypesArray.Add (wxT ("Inventory"));
  trigtypesArray.Add (wxT ("Message"));
  trigtypesArray.Add (wxT ("MeshSelect"));
}

void TriggerSupportDriver::Fill (wxPGProperty* responseProp,
    size_t idx, iTriggerFactory* triggerFact)
{
  csString type = emode->GetTriggerType (triggerFact);
  wxPGProperty* outputProp = AppendStringPar (responseProp, "Trigger", "Trigger", "<composed>");
  AppendEnumPar (outputProp, "Type", "Type", trigtypesArray,
      wxArrayInt (), trigtypesArray.Index (wxString::FromUTF8 (type)));
  TriggerSupport* editor = GetEditor (type);
  if (editor)
    editor->Fill (outputProp, triggerFact);
  detailGrid->Collapse (outputProp);
}

RefreshType TriggerSupportDriver::Update (const csString& field,
    wxPGProperty* selectedProperty, iQuestTriggerResponseFactory* response)
{
  iTriggerFactory* triggerFact = response->GetTriggerFactory ();
  csString type = emode->GetTriggerType (triggerFact);
  csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);
  printf ("Update '%s' in trigger '%s' with value '%s'\n",
      field.GetData (), type.GetData (), value.GetData ()); fflush (stdout);
  if (field == "Type")
  {
    if (value != type)
    {
      value.Downcase ();
      iTriggerType* triggertype = emode->GetQuestManager ()->GetTriggerType ("cel.triggers."+value);
      if (!triggertype)
      {
	csPrintf ("INTERNAL ERROR: Unknown trigger type '%s'!\n", value.GetData ());
	return REFRESH_NOCHANGE;
      }
      csRef<iTriggerFactory> newTriggerFact = triggertype->CreateTriggerFactory ();
      response->SetTriggerFactory (newTriggerFact);
      return REFRESH_STATE;
    }
  }
  else
  {
    TriggerSupport* editor = GetEditor (type);
    if (editor)
      return editor->Update (field, value, selectedProperty, triggerFact);
  }
  return REFRESH_NOCHANGE;
}

//---------------------------------------------------------------------------

class SSDelay : public SequenceSupport
{
public:
  SSDelay (EntityMode* emode) : SequenceSupport ("Delay", emode) { }
  virtual ~SSDelay () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact)
  {
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory)
  {
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class SSDebugPrint : public SequenceSupport
{
public:
  SSDebugPrint (EntityMode* emode) : SequenceSupport ("DebugPrint", emode) { }
  virtual ~SSDebugPrint () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact)
  {
    csRef<iDebugPrintSeqOpFactory> tf = scfQueryInterface<iDebugPrintSeqOpFactory> (seqopFact);
    AppendStringPar (seqProp, "Message", "Message", tf->GetMessage ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory)
  {
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class SSAmbientMesh : public SequenceSupport
{
public:
  SSAmbientMesh (EntityMode* emode) : SequenceSupport ("AmbientMesh", emode) { }
  virtual ~SSAmbientMesh () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact)
  {
    csRef<iAmbientMeshSeqOpFactory> tf = scfQueryInterface<iAmbientMeshSeqOpFactory> (seqopFact);
    AppendButtonPar (seqProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (seqProp, "Tag", "Tag", tf->GetTag ());
    AppendColorPar (seqProp, "Relative Color", "RelColor",
	tf->GetRelColorRed (),
	tf->GetRelColorGreen (),
	tf->GetRelColorBlue ());
    AppendColorPar (seqProp, "Absolute Color", "AbsColor",
	tf->GetAbsColorRed (),
	tf->GetAbsColorGreen (),
	tf->GetAbsColorBlue ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory)
  {
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class SSLight : public SequenceSupport
{
public:
  SSLight (EntityMode* emode) : SequenceSupport ("Light", emode) { }
  virtual ~SSLight () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact)
  {
    csRef<iLightSeqOpFactory> tf = scfQueryInterface<iLightSeqOpFactory> (seqopFact);
    AppendButtonPar (seqProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (seqProp, "Tag", "Tag", tf->GetTag ());
    AppendColorPar (seqProp, "Relative Color", "RelColor",
	tf->GetRelColorRed (),
	tf->GetRelColorGreen (),
	tf->GetRelColorBlue ());
    AppendColorPar (seqProp, "Absolute Color", "AbsColor",
	tf->GetAbsColorRed (),
	tf->GetAbsColorGreen (),
	tf->GetAbsColorBlue ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory)
  {
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class SSMovePath : public SequenceSupport
{
public:
  SSMovePath (EntityMode* emode) : SequenceSupport ("MovePath", emode) { }
  virtual ~SSMovePath () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact)
  {
    // @@@ TODO
    wxPGProperty* outputProp = AppendStringPar (seqProp, "TODO", "TODO", "<composed>");
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory)
  {
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class SSTransform : public SequenceSupport
{
public:
  SSTransform (EntityMode* emode) : SequenceSupport ("Transform", emode) { }
  virtual ~SSTransform () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact)
  {
    csRef<iTransformSeqOpFactory> tf = scfQueryInterface<iTransformSeqOpFactory> (seqopFact);
    AppendButtonPar (seqProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (seqProp, "Tag", "Tag", tf->GetTag ());
    AppendVectorPar (seqProp, "Vector", "Vector",
	tf->GetVectorX (), tf->GetVectorY (), tf->GetVectorZ ());
    wxArrayString rotaxisArray;
    rotaxisArray.Add (wxT ("none"));
    rotaxisArray.Add (wxT ("x"));
    rotaxisArray.Add (wxT ("y"));
    rotaxisArray.Add (wxT ("z"));
    AppendEnumPar (seqProp, "Rotation Axis", "RotAxis", rotaxisArray,
      wxArrayInt (), tf->GetRotationAxis ());
    AppendStringPar (seqProp, "Angle", "Angle", tf->GetRotationAngle ());
    AppendBoolPar (seqProp, "Reversed", "Reversed", tf->IsReversed ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory)
  {
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

class SSProperty : public SequenceSupport
{
public:
  SSProperty (EntityMode* emode) : SequenceSupport ("Property", emode) { }
  virtual ~SSProperty () { }

  virtual void Fill (wxPGProperty* seqProp, iSeqOpFactory* seqopFact)
  {
    csRef<iPropertySeqOpFactory> tf = scfQueryInterface<iPropertySeqOpFactory> (seqopFact);
    AppendButtonPar (seqProp, "Entity", "E:", tf->GetEntity ());
    AppendStringPar (seqProp, "PC", "PC", tf->GetPC ());
    AppendStringPar (seqProp, "PC Tag", "Tag", tf->GetPCTag ());
    AppendStringPar (seqProp, "Property", "Property", tf->GetProperty ());
    AppendStringPar (seqProp, "Float", "Float", tf->GetFloat ());
    AppendStringPar (seqProp, "Long", "Long", tf->GetLong ());
    AppendVectorPar (seqProp, "Vector", "Vector",
	tf->GetVectorX (), tf->GetVectorY (), tf->GetVectorZ ());
    AppendBoolPar (seqProp, "Relative", "Relative", tf->IsRelative ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqOpFactory)
  {
    return REFRESH_NOCHANGE;
  }
};

//---------------------------------------------------------------------------

SequenceSupportDriver::SequenceSupportDriver (const char* name, EntityMode* emode) :
  GridSupport (name, emode)
{
  RegisterEditor (new SSDelay (emode));
  RegisterEditor (new SSDebugPrint (emode));
  RegisterEditor (new SSAmbientMesh (emode));
  RegisterEditor (new SSLight (emode));
  RegisterEditor (new SSMovePath (emode));
  RegisterEditor (new SSTransform (emode));
  RegisterEditor (new SSProperty (emode));

  seqoptypesArray.Add (wxT ("Delay"));
  seqoptypesArray.Add (wxT ("DebugPrint"));
  seqoptypesArray.Add (wxT ("AmbientMesh"));
  seqoptypesArray.Add (wxT ("Light"));
  seqoptypesArray.Add (wxT ("MovePath"));
  seqoptypesArray.Add (wxT ("Transform"));
  seqoptypesArray.Add (wxT ("Property"));
}

void SequenceSupportDriver::FillSeqOps (wxPGProperty* seqProp, size_t idx,
    iCelSequenceFactory* state)
{
  csString s;
  for (size_t i = 0 ; i < state->GetSeqOpFactoryCount () ; i++)
  {
    iSeqOpFactory* seqopFact = state->GetSeqOpFactory (i);
    s.Format ("Operation:%d", int (i));
    wxPGProperty* outputProp = AppendStringPar (seqProp, "Operation", s, "<composed>");

    csString type = emode->GetSeqOpType (seqopFact);;

    AppendEnumPar (outputProp, "Type", "Type", seqoptypesArray,
      wxArrayInt (), seqoptypesArray.Index (wxString::FromUTF8 (type)));
    AppendStringPar (outputProp, "Duration", "Duration", state->GetSeqOpFactoryDuration (i));

    SequenceSupport* editor = GetEditor (type);
    if (editor)
      editor->Fill (outputProp, seqopFact);

    detailGrid->Collapse (outputProp);
  }
}

void SequenceSupportDriver::Fill (wxPGProperty* questProp, iQuestFactory* questFact)
{
  csString s, ss;
  size_t idx = 0;
  csRef<iCelSequenceFactoryIterator> seqIt = questFact->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seqFact = seqIt->Next ();
    s.Format ("Sequence:%s", seqFact->GetName ());
    ss.Format ("Sequence (%s)", seqFact->GetName ());
    wxPGProperty* seqProp = detailGrid->AppendIn (questProp,
      new wxPropertyCategory (wxString::FromUTF8 (ss), wxString::FromUTF8 (s)));
    FillSeqOps (seqProp, idx, seqFact);
    idx++;
  }
}

RefreshType SequenceSupportDriver::Update (const csString& field,
    wxPGProperty* selectedProperty, iCelSequenceFactory* seqFact, size_t index)
{
  iSeqOpFactory* seqOpFact = seqFact->GetSeqOpFactory (index);
  csString type = emode->GetSeqOpType (seqOpFact);
  csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);
  printf ("Update '%s' in seqop '%s' with value '%s'\n",
      field.GetData (), type.GetData (), value.GetData ()); fflush (stdout);
  if (field == "Type")
  {
    if (value != type)
    {
      value.Downcase ();
      csRef<iSeqOpFactory> newSeqopFact;
      if (value != "delay")
      {
        iSeqOpType* seqoptype = emode->GetQuestManager ()->GetSeqOpType ("cel.seqops."+value);
        if (!seqoptype)
        {
	  csPrintf ("INTERNAL ERROR: Unknown seqop type '%s'!\n", value.GetData ());
	  return REFRESH_NOCHANGE;
        }
        newSeqopFact = seqoptype->CreateSeqOpFactory ();
      }
      csString duration = seqFact->GetSeqOpFactoryDuration (index);
      seqFact->UpdateSeqOpFactory (index, newSeqopFact, duration);
      return REFRESH_SEQUENCE;
    }
  }
  else if (field == "Duration")
  {
    seqFact->UpdateSeqOpFactory (index, seqOpFact, value);
    return REFRESH_NO;
  }
  else
  {
    SequenceSupport* editor = GetEditor (type);
    if (editor)
      return editor->Update (field, value, selectedProperty, seqOpFact);
  }
  return REFRESH_NOCHANGE;
}

//---------------------------------------------------------------------------

static wxPGProperty* FindSequenceProperty (wxPGProperty* prop)
{
  while (prop)
  {
    csString propName = (const char*)prop->GetName ().mb_str (wxConvUTF8);
    if (propName.StartsWith ("Sequence:")) return prop;
    prop = prop->GetParent ();
  }
  return prop;
}

static wxPGProperty* FindStateProperty (wxPGProperty* prop)
{
  while (prop)
  {
    csString propName = (const char*)prop->GetName ().mb_str (wxConvUTF8);
    if (propName.StartsWith ("State:")) return prop;
    prop = prop->GetParent ();
  }
  return prop;
}

#define ONINIT_INDEX -3
#define ONEXIT_INDEX -2

static int FindResponseProperty (wxPGProperty* prop)
{
  while (prop)
  {
    csString propName = (const char*)prop->GetName ().mb_str (wxConvUTF8);
    if (propName.StartsWith ("Response:"))
    {
      int idx;
      csScanStr (propName.GetData () + 9, "%d:", &idx);
      return idx;
    }
    else if (propName.StartsWith ("OnInit:")) return ONINIT_INDEX;
    else if (propName.StartsWith ("OnExit:")) return ONEXIT_INDEX;
    prop = prop->GetParent ();
  }
  return -1;
}

iCelSequenceFactory* QuestEditorSupportMain::GetSequenceForProperty (wxPGProperty* property,
    csString& selectedPropName)
{
  selectedPropName = (const char*)property->GetName ().mb_str (wxConvUTF8);

  wxPGProperty* seqProperty = FindSequenceProperty (property);
  if (seqProperty)
  {
    csString seqPropName = (const char*)seqProperty->GetName ().mb_str (wxConvUTF8);
    csString seqName = seqPropName.Slice (9);
    iQuestFactory* questFact = emode->GetSelectedQuest ();
    return questFact->GetSequence (seqName);
  }
  return 0;
}


iQuestStateFactory* QuestEditorSupportMain::GetStateForProperty (wxPGProperty* property,
    csString& selectedPropName, int& responseIndex)
{
  selectedPropName = (const char*)property->GetName ().mb_str (wxConvUTF8);

  responseIndex = FindResponseProperty (property);

  wxPGProperty* stateProperty = FindStateProperty (property);
  if (stateProperty)
  {
    csString statePropName = (const char*)stateProperty->GetName ().mb_str (wxConvUTF8);
    csString stateName = statePropName.Slice (6);
    iQuestFactory* questFact = emode->GetSelectedQuest ();
    return questFact->GetState (stateName);
  }
  return 0;
}

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
  sequenceEditor.AttachNew (new SequenceSupportDriver ("main", emode));
}

void QuestEditorSupportMain::FillResponses (wxPGProperty* stateProp, size_t idx, iQuestStateFactory* state)
{
  csString s;
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    iQuestTriggerResponseFactory* response = responses->Get (i);
    s.Format ("Response:%d:%d", int (i), int (idx));
    wxPGProperty* responseProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("Response"), wxString::FromUTF8 (s)));
    iTriggerFactory* triggerFact = response->GetTriggerFactory ();
    triggerEditor->Fill (responseProp, i, triggerFact);
    csRef<iRewardFactoryArray> rewards = response->GetRewardFactories ();
    rewardEditor->FillRewards (responseProp, rewards);
  }

}

void QuestEditorSupportMain::FillOnInit (wxPGProperty* stateProp, size_t idx,
    iQuestStateFactory* state)
{
  csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
  if (initRewards->GetSize () > 0)
  {
    csString s;
    s.Format ("OnInit:%d\n", int (idx));
    wxPGProperty* oninitProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("OnInit"), wxString::FromUTF8 (s)));
    rewardEditor->FillRewards (oninitProp, initRewards);
  }
}

void QuestEditorSupportMain::FillOnExit (wxPGProperty* stateProp, size_t idx,
    iQuestStateFactory* state)
{
  csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
  if (exitRewards->GetSize () > 0)
  {
    csString s;
    s.Format ("OnExit:%d\n", int (idx));
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
  sequenceEditor->Fill (questProp, questFact);
}

RefreshType QuestEditorSupportMain::Update (iQuestFactory* questFact,
    iQuestStateFactory* stateFact, wxPGProperty* selectedProperty,
    const csString& selectedPropName, int responseIndex)
{
  size_t idx = selectedPropName.FindFirst ('.');
  if (idx == csArrayItemNotFound) return REFRESH_NOCHANGE;

  csString field = selectedPropName.Slice (idx+1);

  if (selectedPropName.StartsWith ("Trigger"))
  {
    csRef<iQuestTriggerResponseFactoryArray> responses = stateFact->GetTriggerResponseFactories ();
    iQuestTriggerResponseFactory* response = responses->Get (responseIndex);
    return triggerEditor->Update (field, selectedProperty, response);
  }
  else if (selectedPropName.StartsWith ("Reward:"))
  {
    int rewardIndex;
    csScanStr (selectedPropName.GetData () + 7, "%d", &rewardIndex);
    csRef<iRewardFactoryArray> rewards;
    if (responseIndex == ONINIT_INDEX)
      rewards = stateFact->GetInitRewardFactories ();
    else if (responseIndex == ONEXIT_INDEX)
      rewards = stateFact->GetExitRewardFactories ();
    else
    {
      csRef<iQuestTriggerResponseFactoryArray> responses = stateFact->GetTriggerResponseFactories ();
      iQuestTriggerResponseFactory* response = responses->Get (responseIndex);
      rewards = response->GetRewardFactories ();
    }
    return rewardEditor->Update (field, selectedProperty, rewards, size_t (rewardIndex));
  }

  return REFRESH_NOCHANGE;
}

RefreshType QuestEditorSupportMain::Update (iQuestFactory* questFact,
    iCelSequenceFactory* seqFact, wxPGProperty* selectedProperty,
    const csString& selectedPropName)
{
  size_t idx = selectedPropName.FindFirst ('.');
  if (idx == csArrayItemNotFound) return REFRESH_NOCHANGE;

  csString field = selectedPropName.Slice (idx+1);

  int operationIndex;
  csScanStr (selectedPropName.GetData () + 10, "%d", &operationIndex);
  return sequenceEditor->Update (field, selectedProperty, seqFact, size_t (operationIndex));
}

RefreshType QuestEditorSupportMain::Update (wxPGProperty* selectedProperty,
    iQuestStateFactory*& state, iCelSequenceFactory*& sequence)
{
  csString selectedPropName;
  int responseIndex;
  sequence = GetSequenceForProperty (selectedProperty, selectedPropName);
  state = GetStateForProperty (selectedProperty, selectedPropName, responseIndex);
  if (state)
  {
    printf ("Quest/PG changed %s state=%s response=%d!\n", selectedPropName.GetData (),
        state ? state->GetName () : "-", responseIndex); fflush (stdout);
    return Update (emode->GetSelectedQuest (), state, selectedProperty, selectedPropName,
        responseIndex);
  }
  else if (sequence)
  {
    printf ("Quest/PG changed %s sequence=%s!\n", selectedPropName.GetData (),
        sequence ? sequence->GetName () : "-"); fflush (stdout);
    return Update (emode->GetSelectedQuest (), sequence, selectedProperty, selectedPropName);
  }
  return REFRESH_NOCHANGE;
}

