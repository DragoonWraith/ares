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
    AppendEditEnumPar (responseProp, "State", "State", states,
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
      tf->SetStateParameter (value);
    else if (field == "E:Entity")
      tf->SetEntityParameter (value, tf->GetTagParameter ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntityParameter (), value);
    else if (field == "Class")
      tf->SetClassParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "E:ChildEntity")
      tf->SetChildEntityParameter (value, tf->GetChildTag ());
    else if (field == "ChildTag")
      tf->SetChildEntityParameter (tf->GetChildEntity (), value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "Class")
      tf->SetClassParameter (value);
    else if (field == "Sequence")
      tf->SetSequenceParameter (value);
    else if (field == "Delay")
      tf->SetDelayParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetSequenceParameter (value);
    else if (field == "Delay")
      tf->SetDelayParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "Class")
      tf->SetClassParameter (value);
    else if (field == "Sequence")
      tf->SetSequenceParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value);
    else if (field == "PCTag")
      tf->SetPCParameter (tf->GetPC (), value);
    else if (field == "PC")
      tf->SetPCParameter (value, tf->GetPCTag ());
    else if (field == "Class")
      tf->SetClassParameter (value);
    else if (field == "Property")
      tf->SetPropertyParameter (value);
    else if (field == "String")
      tf->SetStringParameter (value);
    else if (field == "Long")
      tf->SetLongParameter (value);
    else if (field == "Float")
      tf->SetFloatParameter (value);
    else if (field == "Bool")
      tf->SetBoolParameter (value);
    else if (field == "Diff")
      tf->SetDiffParameter (value);
    else if (field == "Toggle")
      tf->SetToggle (ToBool (value));
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityTemplateParameter (value);
    else if (field == "Name")
      tf->SetNameParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value);
    else if (field == "Class")
      tf->SetClassParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value);
    else if (field == "Entities")
      tf->SetEntitiesParameter (value);
    else if (field == "Class")
      tf->SetClassParameter (value);
    else if (field == "Remove")
      tf->SetRemove (ToBool (value));
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value);
    else if (field == "Tag")
      tf->SetTagParameter (value);
    else if (field == "Class")
      tf->SetClassParameter (value);
    else if (field == "PC")
      tf->SetPropertyClassParameter (value);
    else if (field == "A:Action")
      tf->SetIDParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
  }
};

//---------------------------------------------------------------------------

class RSMessage : public RewardSupport
{
private:
  int idNewPar, idDelPar;

public:
  RSMessage (EntityMode* emode) : RewardSupport ("Message", emode)
  {
    idNewPar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::Message_OnCreatePar));
    idDelPar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::Message_OnDeletePar));
  }
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

    for (size_t i = 0 ; i < tf->GetParameterCount () ; i++)
    {
      csStringID id = tf->GetParameterID (i);
      csString name = pl->FetchString (id);
      csString value = tf->GetParameterValue (i);
      celDataType type = tf->GetParameterType (i);
      AppendPar (responseProp, "Par", name, type, value);
    }
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iRewardFactory* rewardFact)
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (rewardFact);
    if (field == "E:Entity")
      tf->SetEntityParameter (value);
    else if (field == "Entities")
      tf->SetEntitiesParameter (value);
    else if (field == "Class")
      tf->SetClassParameter (value);
    else if (field == "A:Message")
      tf->SetIDParameter (value);
    else if (field.StartsWith ("Par:") && field.EndsWith (".Type"))
    {
      size_t idx = field.FindFirst ('.');
      csString par = field.Slice (4, idx-4);
      csStringID id = pl->FetchStringID (par);
      size_t paridx = tf->GetParameterIndex (id);
      csString parValue = tf->GetParameterValue (paridx);
      tf->AddParameter (InspectTools::StringToType (value), id, parValue);
    }
    else if (field.StartsWith ("Par:") && field.EndsWith (".Name"))
    {
      size_t idx = field.FindFirst ('.');
      csString par = field.Slice (4, idx-4);
      csStringID id = pl->FetchStringID (par);
      size_t paridx = tf->GetParameterIndex (id);
      celDataType type = tf->GetParameterType (paridx);
      csString parValue = tf->GetParameterValue (paridx);
      tf->RemoveParameter (id);
      csStringID newID = pl->FetchStringID (value);
      tf->AddParameter (type, newID, parValue);

    }
    else if (field.StartsWith ("Par:") && field.EndsWith (".Value"))
    {
      size_t idx = field.FindFirst ('.');
      csString par = field.Slice (4, idx-4);
      csStringID id = pl->FetchStringID (par);
      size_t paridx = tf->GetParameterIndex (id);
      celDataType type = tf->GetParameterType (paridx);
      tf->AddParameter (type, id, value);
    }
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
  }

  virtual void DoContext (const csString& field, iRewardFactory* rewardFact,
      wxMenu* contextMenu, csString& todelete)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewPar, wxT ("New Parameter..."));
    if (field.StartsWith ("Par:"))
      contextMenu->Append (idDelPar, wxT ("Delete Parameter..."));
  }
  virtual bool OnCreatePar (wxPGProperty* property, iRewardFactory* rewardFact)
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (rewardFact);
    csRef<iUIDialog> dialog = ui->CreateDialog ("New Message Parameter",
        "LName:;TName;CType,string,float,long,bool,vector2,vector3,color\nMValue");
    if (dialog->Show (0) == 0) return false;
    DialogResult result = dialog->GetFieldContents ();
    csString name = result.Get ("Name", (const char*)0);
    csString value = result.Get ("Value", (const char*)0);
    csString typeS = result.Get ("Type", (const char*)0);
    csStringID id = pl->FetchStringID (name);

    if (name.IsEmpty ())
    {
      ui->Error ("Empty name is not allowed!");
      return false;
    }
    else if (tf->GetParameterIndex (id) != csArrayItemNotFound)
    {
      ui->Error ("There is already a parameter with this name!");
      return false;
    }

    celDataType type = InspectTools::StringToType (typeS);
    tf->AddParameter (type, id, value);
    return true;
  }
  virtual bool OnDeletePar (wxPGProperty* property, iRewardFactory* rewardFact)
  {
    csRef<iMessageRewardFactory> tf = scfQueryInterface<iMessageRewardFactory> (rewardFact);
    csString field = GetPropertyName (property);
    size_t paridx = field.Find (".Par:");
    csString par = field.Slice (paridx+5);
    size_t idx = par.FindFirst ('.');
    if (idx != csArrayItemNotFound)
      par = par.Slice (0, idx);
    printf ("Delete parameter (%s) '%s'\n", field.GetData (), par.GetData ()); fflush (stdout);
    csStringID id = pl->FetchStringID (par);
    tf->RemoveParameter (id);
    return true;
  }
};


//---------------------------------------------------------------------------

RewardSupportDriver::RewardSupportDriver (const char* name, EntityMode* emode)
  : GridSupport (name, emode)
{
  idCreateReward = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreateReward));
  idMoveTop = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnRewardTop));
  idMoveBottom = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnRewardBottom));
  idMoveUp = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnRewardUp));
  idMoveDown = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnRewardDown));

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

void RewardSupportDriver::DoContext (const csString& field,
      iRewardFactoryArray* rewards, size_t idx, wxMenu* contextMenu, csString& todelete)
{
  contextMenu->AppendSeparator ();
  contextMenu->Append (idCreateReward, wxT ("Create Reward..."));
  contextMenu->Append (idMoveTop, wxT ("Move First"));
  contextMenu->Append (idMoveUp, wxT ("Move Up"));
  contextMenu->Append (idMoveDown, wxT ("Move Down"));
  contextMenu->Append (idMoveBottom, wxT ("Move Last"));

  iRewardFactory* rewardFact = rewards->Get (idx);
  csString type = emode->GetRewardType (rewardFact);
  printf ("Context for field '%s' in reward '%s'\n", field.GetData (), type.GetData ()); fflush (stdout);
  RewardSupport* editor = GetEditor (type);
  if (editor)
  {
    editor->DoContext (field, rewardFact, contextMenu, todelete);
    if (todelete.IsEmpty ())
      todelete = "Reward";
  }
}

bool RewardSupportDriver::OnCreatePar (wxPGProperty* property, iRewardFactory* reward)
{
  csString type = emode->GetRewardType (reward);
  RewardSupport* editor = GetEditor (type);
  if (editor)
    return editor->OnCreatePar (property, reward);
  return false;
}

bool RewardSupportDriver::OnDeletePar (wxPGProperty* property, iRewardFactory* reward)
{
  csString type = emode->GetRewardType (reward);
  RewardSupport* editor = GetEditor (type);
  if (editor)
    return editor->OnDeletePar (property, reward);
  return false;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "Sector")
      tf->SetSectorParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "Sequence")
      tf->SetSequenceParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "E:Child")
      tf->SetChildEntityParameter (value);
    else if (field == "T:ChildTemplate")
      tf->SetChildTemplateParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value);
    else if (field == "A:Mask")
      tf->SetMaskParameter (value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "Property")
      tf->SetPropertyParameter (value);
    else if (field == "Value")
      tf->SetValueParameter (value);
    else if (field == "Operation")
      tf->SetOperationParameter (value);
    else if (field == "ChangeOnly")
      tf->SetOnChangeOnly (ToBool (value));
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "Leave")
      tf->EnableLeave (ToBool (value));
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
    AppendVectorPar (responseProp, "Offset", "V:",
	tf->GetOffsetX (), tf->GetOffsetY (), tf->GetOffsetZ ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iTriggerFactory* triggerFact)
  {
    csRef<iWatchTriggerFactory> tf = scfQueryInterface<iWatchTriggerFactory> (triggerFact);
    if (field == "E:Entity")
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "E:Target")
      tf->SetTargetEntityParameter (value, tf->GetTargetTag ());
    else if (field == "TargetTag")
      tf->SetTargetEntityParameter (tf->GetTargetEntity (), value);
    else if (field == "CheckTime")
      tf->SetChecktimeParameter (value);
    else if (field == "Radius")
      tf->SetRadiusParameter (value);
    else if (field == "V:Offset.X")
      tf->SetOffsetParameter (value, tf->GetOffsetY (), tf->GetOffsetZ ());
    else if (field == "V:Offset.Y")
      tf->SetOffsetParameter (tf->GetOffsetX (), value, tf->GetOffsetZ ());
    else if (field == "V:Offset.Z")
      tf->SetOffsetParameter (tf->GetOffsetX (), tf->GetOffsetY (), value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
  }
};

//---------------------------------------------------------------------------

TriggerSupportDriver::TriggerSupportDriver (const char* name, EntityMode* emode)
  : GridSupport (name, emode)
{
  idCreateReward = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreateReward));

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

void TriggerSupportDriver::DoContext (const csString& field,
    iQuestTriggerResponseFactory* response, wxMenu* contextMenu, csString& todelete)
{
  contextMenu->AppendSeparator ();
  contextMenu->Append (idCreateReward, wxT ("Create Reward..."));

  iTriggerFactory* triggerFact = response->GetTriggerFactory ();
  csString type = emode->GetTriggerType (triggerFact);
  printf ("Context for field '%s' in trigger '%s'\n", field.GetData (), type.GetData ()); fflush (stdout);
  TriggerSupport* editor = GetEditor (type);
  if (editor)
  {
    editor->DoContext (field, triggerFact, contextMenu, todelete);
    if (todelete.IsEmpty ())
      todelete = "Trigger";
  }
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
      wxPGProperty* selectedProperty, iSeqOpFactory* seqopFact)
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
      wxPGProperty* selectedProperty, iSeqOpFactory* seqopFact)
  {
    csRef<iDebugPrintSeqOpFactory> tf = scfQueryInterface<iDebugPrintSeqOpFactory> (seqopFact);
    if (field == "Message")
    {
      tf->SetMessageParameter (value);
      return REFRESH_NO;
    }
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
    AppendColorPar (seqProp, "Relative Color", "c:",
	tf->GetRelColorRed (),
	tf->GetRelColorGreen (),
	tf->GetRelColorBlue ());
    AppendColorPar (seqProp, "Absolute Color", "c:",
	tf->GetAbsColorRed (),
	tf->GetAbsColorGreen (),
	tf->GetAbsColorBlue ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqopFact)
  {
    csRef<iAmbientMeshSeqOpFactory> tf = scfQueryInterface<iAmbientMeshSeqOpFactory> (seqopFact);
    if (field == "E:Entity")
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "c:Relative Color.Red")
      tf->SetRelColorParameter (value, tf->GetRelColorGreen (), tf->GetRelColorBlue ());
    else if (field == "c:Relative Color.Green")
      tf->SetRelColorParameter (tf->GetRelColorRed (), value, tf->GetRelColorBlue ());
    else if (field == "c:Relative Color.Blue")
      tf->SetRelColorParameter (tf->GetRelColorRed (), tf->GetRelColorGreen (), value);
    else if (field == "c:Absolute Color.Red")
      tf->SetAbsColorParameter (value, tf->GetAbsColorGreen (), tf->GetAbsColorBlue ());
    else if (field == "c:Absolute Color.Green")
      tf->SetAbsColorParameter (tf->GetAbsColorRed (), value, tf->GetAbsColorBlue ());
    else if (field == "c:Absolute Color.Blue")
      tf->SetAbsColorParameter (tf->GetAbsColorRed (), tf->GetAbsColorGreen (), value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
    AppendColorPar (seqProp, "Relative Color", "c:",
	tf->GetRelColorRed (),
	tf->GetRelColorGreen (),
	tf->GetRelColorBlue ());
    AppendColorPar (seqProp, "Absolute Color", "c:",
	tf->GetAbsColorRed (),
	tf->GetAbsColorGreen (),
	tf->GetAbsColorBlue ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqopFact)
  {
    csRef<iLightSeqOpFactory> tf = scfQueryInterface<iLightSeqOpFactory> (seqopFact);
    if (field == "E:Entity")
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "c:Relative Color.Red")
      tf->SetRelColorParameter (value, tf->GetRelColorGreen (), tf->GetRelColorBlue ());
    else if (field == "c:Relative Color.Green")
      tf->SetRelColorParameter (tf->GetRelColorRed (), value, tf->GetRelColorBlue ());
    else if (field == "c:Relative Color.Blue")
      tf->SetRelColorParameter (tf->GetRelColorRed (), tf->GetRelColorGreen (), value);
    else if (field == "c:Absolute Color.Red")
      tf->SetAbsColorParameter (value, tf->GetAbsColorGreen (), tf->GetAbsColorBlue ());
    else if (field == "c:Absolute Color.Green")
      tf->SetAbsColorParameter (tf->GetAbsColorRed (), value, tf->GetAbsColorBlue ());
    else if (field == "c:Absolute Color.Blue")
      tf->SetAbsColorParameter (tf->GetAbsColorRed (), tf->GetAbsColorGreen (), value);
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
    /*wxPGProperty* outputProp = */ AppendStringPar (seqProp, "TODO", "TODO", "<composed>");
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqopFact)
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
    AppendVectorPar (seqProp, "Vector", "V:",
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
      wxPGProperty* selectedProperty, iSeqOpFactory* seqopFact)
  {
    csRef<iTransformSeqOpFactory> tf = scfQueryInterface<iTransformSeqOpFactory> (seqopFact);
    if (field == "E:Entity")
      tf->SetEntityParameter (value, tf->GetTag ());
    else if (field == "Tag")
      tf->SetEntityParameter (tf->GetEntity (), value);
    else if (field == "V:Vector.X")
      tf->SetVectorParameter (value, tf->GetVectorY (), tf->GetVectorZ ());
    else if (field == "V:Vector.Y")
      tf->SetVectorParameter (tf->GetVectorX (), value, tf->GetVectorZ ());
    else if (field == "V:Vector.Z")
      tf->SetVectorParameter (tf->GetVectorX (), tf->GetVectorY (), value);
    else if (field == "Angle")
      tf->SetRotationParameter (tf->GetRotationAxis (), value);
    else if (field == "RotAxis")
    {
      if (value == "none") tf->SetRotationParameter (-1, tf->GetRotationAngle ());
      else if (value == "x") tf->SetRotationParameter (0, tf->GetRotationAngle ());
      else if (value == "y") tf->SetRotationParameter (1, tf->GetRotationAngle ());
      else if (value == "z") tf->SetRotationParameter (2, tf->GetRotationAngle ());
    }
    else if (field == "Reversed")
      tf->SetReversed (ToBool (value));
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
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
    AppendVectorPar (seqProp, "Vector", "V:",
	tf->GetVectorX (), tf->GetVectorY (), tf->GetVectorZ ());
    AppendBoolPar (seqProp, "Relative", "Relative", tf->IsRelative ());
  }
  virtual RefreshType Update (const csString& field, const csString& value,
      wxPGProperty* selectedProperty, iSeqOpFactory* seqopFact)
  {
    csRef<iPropertySeqOpFactory> tf = scfQueryInterface<iPropertySeqOpFactory> (seqopFact);
    if (field == "E:Entity")
      tf->SetEntityParameter (value);
    else if (field == "PC")
      tf->SetPCParameter (value, tf->GetPCTag ());
    else if (field == "Tag")
      tf->SetPCParameter (tf->GetPC (), value);
    else if (field == "Property")
      tf->SetPropertyParameter (value);
    else if (field == "Float")
      tf->SetFloatParameter (value);
    else if (field == "Long")
      tf->SetLongParameter (value);
    else if (field == "V:Vector.X")
      tf->SetVector3Parameter (value, tf->GetVectorY (), tf->GetVectorZ ());
    else if (field == "V:Vector.Y")
      tf->SetVector3Parameter (tf->GetVectorX (), value, tf->GetVectorZ ());
    else if (field == "V:Vector.Z")
      tf->SetVector3Parameter (tf->GetVectorX (), tf->GetVectorY (), value);
    else if (field == "Relative")
      tf->SetRelative (ToBool (value));
    else
      return REFRESH_NOCHANGE;
    return REFRESH_NO;
  }
};

//---------------------------------------------------------------------------

SequenceSupportDriver::SequenceSupportDriver (const char* name, EntityMode* emode) :
  GridSupport (name, emode)
{
  idCreateSeqOp = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreateSeqOp));
  idMoveTop = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnSeqOpTop));
  idMoveBottom = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnSeqOpBottom));
  idMoveUp = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnSeqOpUp));
  idMoveDown = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnSeqOpDown));

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

void SequenceSupportDriver::FillSequence (wxPGProperty* seqProp, iCelSequenceFactory* state)
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
  csRef<iCelSequenceFactoryIterator> seqIt = questFact->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seqFact = seqIt->Next ();
    s.Format ("Sequence:%s", seqFact->GetName ());
    ss.Format ("Sequence (%s)", seqFact->GetName ());
    wxPGProperty* seqProp = detailGrid->AppendIn (questProp,
      new wxPropertyCategory (wxString::FromUTF8 (ss), wxString::FromUTF8 (s)));
    FillSequence (seqProp, seqFact);
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

void SequenceSupportDriver::DoContext (const csString& field,
    iCelSequenceFactory* seqFact, size_t index, wxMenu* contextMenu, csString& todelete)
{
  contextMenu->AppendSeparator ();
  contextMenu->Append (idCreateSeqOp, wxT ("Create Operation..."));
  contextMenu->Append (idMoveTop, wxT ("Move First"));
  contextMenu->Append (idMoveUp, wxT ("Move Up"));
  contextMenu->Append (idMoveDown, wxT ("Move Down"));
  contextMenu->Append (idMoveBottom, wxT ("Move Last"));

  iSeqOpFactory* seqOpFact = seqFact->GetSeqOpFactory (index);
  csString type = emode->GetSeqOpType (seqOpFact);
  printf ("Context for field '%s' in seqop '%s'\n", field.GetData (), type.GetData ()); fflush (stdout);
  SequenceSupport* editor = GetEditor (type);
  if (editor)
  {
    editor->DoContext (field, seqOpFact, contextMenu, todelete);
    if (todelete.IsEmpty ())
      todelete = "Sequence Operation";
  }
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

int QuestEditorSupportMain::GetResponseIndexForProperty (wxPGProperty* prop)
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

iCelSequenceFactory* QuestEditorSupportMain::GetSequenceForProperty (wxPGProperty* property)
{
  wxPGProperty* seqProperty = FindSequenceProperty (property);
  if (seqProperty)
  {
    csString seqPropName = GetPropertyName (seqProperty);
    csString seqName = seqPropName.Slice (9);
    iQuestFactory* questFact = emode->GetSelectedQuest ();
    return questFact->GetSequence (seqName);
  }
  return 0;
}

iQuestStateFactory* QuestEditorSupportMain::GetStateForProperty (wxPGProperty* property)
{
  wxPGProperty* stateProperty = FindStateProperty (property);
  if (stateProperty)
  {
    csString statePropName = GetPropertyName (stateProperty);
    csString stateName = statePropName.Slice (6);
    iQuestFactory* questFact = emode->GetSelectedQuest ();
    return questFact->GetState (stateName);
  }
  return 0;
}

size_t QuestEditorSupportMain::GetSeqOpForProperty (wxPGProperty* property)
{
  int operationIndex;
  csString propName = GetPropertyName (property);
  csScanStr (propName.GetData () + 10, "%d", &operationIndex);
  return operationIndex;
}

csRef<iRewardFactoryArray> QuestEditorSupportMain::GetRewardForProperty (
    wxPGProperty* property, size_t& index)
{
  int responseIndex = GetResponseIndexForProperty (property);;
  iQuestStateFactory* state = GetStateForProperty (property);
  if (!state) return 0;

  csString propName = GetPropertyName (property);
  if (!propName.StartsWith ("Reward:")) return 0;

  int rewardIndex;
  csScanStr (propName.GetData () + 7, "%d", &rewardIndex);
  index = size_t (rewardIndex);

  if (responseIndex == ONINIT_INDEX)
    return state->GetInitRewardFactories ();
  else if (responseIndex == ONEXIT_INDEX)
    return state->GetExitRewardFactories ();
  else
  {
    csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
    iQuestTriggerResponseFactory* response = responses->Get (responseIndex);
    return response->GetRewardFactories ();
  }
}

QuestEditorSupportMain::QuestEditorSupportMain (EntityMode* emode) :
  GridSupport ("main", emode)
{
  idNewState = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnNewState));
  idNewSequence = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnNewSequence));
  idDelete = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnDelete));
  idCreateTrigger = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreateTrigger));
  idCreateRewardOnInit = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreateRewardOnInit));
  idCreateRewardOnExit = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreateRewardOnExit));

  triggerEditor.AttachNew (new TriggerSupportDriver ("main", emode));
  rewardEditor.AttachNew (new RewardSupportDriver ("main", emode));
  sequenceEditor.AttachNew (new SequenceSupportDriver ("main", emode));
}

static csString ReplaceDot (const char* s)
{
  csString str = s;
  str.ReplaceAll (".", "");
  return str;
}

void QuestEditorSupportMain::FillResponses (wxPGProperty* stateProp, iQuestStateFactory* state)
{
  csString s;
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    iQuestTriggerResponseFactory* response = responses->Get (i);
    s.Format ("Response:%d:%s", int (i), ReplaceDot (state->GetName ()).GetData ());
    wxPGProperty* responseProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("Response"), wxString::FromUTF8 (s)));
    iTriggerFactory* triggerFact = response->GetTriggerFactory ();
    triggerEditor->Fill (responseProp, i, triggerFact);
    csRef<iRewardFactoryArray> rewards = response->GetRewardFactories ();
    rewardEditor->FillRewards (responseProp, rewards);
  }

}

void QuestEditorSupportMain::FillOnInit (wxPGProperty* stateProp, iQuestStateFactory* state)
{
  csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
  if (initRewards->GetSize () > 0)
  {
    csString s;
    s.Format ("OnInit:%s\n", ReplaceDot (state->GetName ()).GetData ());
    wxPGProperty* oninitProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("OnInit"), wxString::FromUTF8 (s)));
    rewardEditor->FillRewards (oninitProp, initRewards);
  }
}

void QuestEditorSupportMain::FillOnExit (wxPGProperty* stateProp, iQuestStateFactory* state)
{
  csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
  if (exitRewards->GetSize () > 0)
  {
    csString s;
    s.Format ("OnExit:%s\n", ReplaceDot (state->GetName ()).GetData ());
    wxPGProperty* onexitProp = detailGrid->AppendIn (stateProp,
      new wxPropertyCategory (wxT ("OnExit"), wxString::FromUTF8 (s)));
    rewardEditor->FillRewards (onexitProp, exitRewards);
  }
}

void QuestEditorSupportMain::FillState (wxPGProperty* stateProp, iQuestStateFactory* state)
{
  FillOnInit (stateProp, state);
  FillResponses (stateProp, state);
  FillOnExit (stateProp, state);
}

void QuestEditorSupportMain::Fill (wxPGProperty* questProp, iQuestFactory* questFact)
{
  csString s, ss;
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  while (it->HasNext ())
  {
    iQuestStateFactory* state = it->Next ();
    s.Format ("State:%s", state->GetName ());
    ss.Format ("State (%s)", state->GetName ());
    wxPGProperty* stateProp = detailGrid->AppendIn (questProp,
      new wxPropertyCategory (wxString::FromUTF8 (ss), wxString::FromUTF8 (s)));
    FillState (stateProp, state);
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
  csString selectedPropName = GetPropertyName (selectedProperty);
  sequence = GetSequenceForProperty (selectedProperty);
  int responseIndex = GetResponseIndexForProperty (selectedProperty);
  state = GetStateForProperty (selectedProperty);
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

void QuestEditorSupportMain::DoContext (iQuestStateFactory* state,
    const csString& selectedPropName, int responseIndex, wxMenu* contextMenu)
{
  contextMenu->AppendSeparator ();
  contextMenu->Append (idCreateTrigger, wxT ("Create trigger..."));
  contextMenu->Append (idCreateRewardOnInit, wxT ("Create on-init reward..."));
  contextMenu->Append (idCreateRewardOnExit, wxT ("Create on-exit reward..."));

  size_t idx = selectedPropName.FindFirst ('.');
  csString field;
  if (idx != csArrayItemNotFound)
    field = selectedPropName.Slice (idx+1);

  if (selectedPropName.StartsWith ("Trigger"))
  {
    csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
    iQuestTriggerResponseFactory* response = responses->Get (responseIndex);
    triggerEditor->DoContext (field, response, contextMenu, todelete);
  }
  else if (selectedPropName.StartsWith ("Reward:"))
  {
    int rewardIndex;
    csScanStr (selectedPropName.GetData () + 7, "%d", &rewardIndex);
    csRef<iRewardFactoryArray> rewards;
    if (responseIndex == ONINIT_INDEX)
      rewards = state->GetInitRewardFactories ();
    else if (responseIndex == ONEXIT_INDEX)
      rewards = state->GetExitRewardFactories ();
    else
    {
      csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
      iQuestTriggerResponseFactory* response = responses->Get (responseIndex);
      rewards = response->GetRewardFactories ();
    }
    rewardEditor->DoContext (field, rewards, size_t (rewardIndex),
	contextMenu, todelete);
  }
  if (todelete.IsEmpty ())
    todelete = "State";
}

void QuestEditorSupportMain::DoContext (iCelSequenceFactory* seq,
    const csString& selectedPropName, wxMenu* contextMenu)
{
  size_t idx = selectedPropName.FindFirst ('.');

  csString field;
  if (idx != csArrayItemNotFound)
    field = selectedPropName.Slice (idx+1);

  int operationIndex;
  csScanStr (selectedPropName.GetData () + 10, "%d", &operationIndex);
  sequenceEditor->DoContext (field, seq, size_t (operationIndex), contextMenu, todelete);
  if (todelete.IsEmpty ())
    todelete = "Sequence";
}

void QuestEditorSupportMain::DoContext (wxPGProperty* property, wxMenu* contextMenu)
{
  contextMenu->Append (idNewState, wxT ("New State..."));
  contextMenu->Append (idNewSequence, wxT ("New Sequence..."));

  todelete.Empty ();

  csString selectedPropName = GetPropertyName (property);
  iCelSequenceFactory* sequence = GetSequenceForProperty (property);
  int responseIndex = GetResponseIndexForProperty (property);;
  iQuestStateFactory* state = GetStateForProperty (property);
  if (state)
  {
    printf ("Quest/PG context %s state=%s response=%d!\n", selectedPropName.GetData (),
        state ? state->GetName () : "-", responseIndex); fflush (stdout);
    DoContext (state, selectedPropName, responseIndex, contextMenu);
  }
  else if (sequence)
  {
    printf ("Quest/PG context %s sequence=%s!\n", selectedPropName.GetData (),
        sequence ? sequence->GetName () : "-"); fflush (stdout);
    DoContext (sequence, selectedPropName, contextMenu);
  }
  if (!todelete.IsEmpty ())
  {
    csString label = "Delete ";
    label += todelete;
    contextMenu->AppendSeparator ();
    contextMenu->Append (idDelete, wxString::FromUTF8 (label));
  }
}

bool QuestEditorSupportMain::OnDeleteFromContext (wxPGProperty* contextProperty,
    iQuestFactory* questFact)
{
  iQuestStateFactory* state = GetStateForProperty (contextProperty);
  iCelSequenceFactory* sequence = GetSequenceForProperty (contextProperty);
  if (todelete == "State")
    questFact->RemoveState (state->GetName ());
  else if (todelete == "Sequence")
    questFact->RemoveSequence (sequence->GetName ());
  else if (todelete == "Reward")
  {
    size_t rewardIndex;
    csRef<iRewardFactoryArray> rewards = GetRewardForProperty (contextProperty, rewardIndex);
    rewards->DeleteIndex (rewardIndex);
  }
  else if (todelete == "Trigger")
  {
    int responseIndex = GetResponseIndexForProperty (contextProperty);;
    csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
    responses->DeleteIndex (responseIndex);
  }
  else if (todelete == "Sequence Operation")
  {
    csString propName = GetPropertyName (contextProperty);
    int operationIndex;
    csScanStr (propName.GetData () + 10, "%d", &operationIndex);
    sequence->RemoveSeqOpFactory (operationIndex);
  }
  else
    return false;
  return true;
}

bool QuestEditorSupportMain::OnCreatePar (wxPGProperty* contextProperty)
{
  size_t index;
  csRef<iRewardFactoryArray> rewards = GetRewardForProperty (contextProperty, index);
  iRewardFactory* reward = rewards->Get (index);
  return rewardEditor->OnCreatePar (contextProperty, reward);
}

bool QuestEditorSupportMain::OnDeletePar (wxPGProperty* contextProperty)
{
  size_t index;
  csRef<iRewardFactoryArray> rewards = GetRewardForProperty (contextProperty, index);
  iRewardFactory* reward = rewards->Get (index);
  return rewardEditor->OnDeletePar (contextProperty, reward);
}

wxPGProperty* QuestEditorSupportMain::Find (iQuestStateFactory* state)
{
  csString s;
  s.Format ("State:%s", state->GetName ());
  wxPGProperty* stateProp = detailGrid->GetPropertyByName (wxString::FromUTF8 (s));
  return stateProp;
}

wxPGProperty* QuestEditorSupportMain::Find (iCelSequenceFactory* sequence)
{
  csString s;
  s.Format ("Sequence:%s", sequence->GetName ());
  wxPGProperty* seqProp = detailGrid->GetPropertyByName (wxString::FromUTF8 (s));
  return seqProp;
}

