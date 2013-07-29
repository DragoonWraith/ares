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

#include <crystalspace.h>
#include "imap/objectcomment.h"
#include "edcommon/listctrltools.h"
#include "edcommon/inspect.h"
#include "edcommon/uitools.h"
#include "entitymode.h"
#include "templatepanel.h"
#include "pcpanel.h"
#include "triggerpanel.h"
#include "rewardpanel.h"
#include "sequencepanel.h"
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/imodelrepository.h"
#include "iassetmanager.h"

#include "celtool/stdparams.h"
#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/questmanager.h"
#include "tools/parameters.h"
#include "tools/entitytplloader.h"
#include "propclass/chars.h"

#include <wx/xrc/xmlres.h>
#include <wx/listbox.h>
#include "cseditor/wx/propgrid/propdev.h"

//---------------------------------------------------------------------------

#define PG_ID (wxID_HIGHEST+1)


BEGIN_EVENT_TABLE(EntityMode::Panel, wxPanel)
  EVT_LIST_ITEM_SELECTED (XRCID("template_List"), EntityMode::Panel::OnTemplateSelect)
  EVT_LIST_ITEM_SELECTED (XRCID("quest_List"), EntityMode::Panel::OnQuestSelect)
  EVT_PG_CHANGING (PG_ID, EntityMode::Panel::OnPropertyGridChanging)
  EVT_PG_CHANGED (PG_ID, EntityMode::Panel::OnPropertyGridChanged)
  EVT_BUTTON (PG_ID, EntityMode::Panel::OnPropertyGridButton)
  EVT_CONTEXT_MENU (EntityMode::Panel::OnContextMenu)
END_EVENT_TABLE()

SCF_IMPLEMENT_FACTORY (EntityMode)

static csStringID ID_Copy = csInvalidStringID;
static csStringID ID_Paste = csInvalidStringID;
static csStringID ID_Delete = csInvalidStringID;

//---------------------------------------------------------------------------

class GraphNodeCallback : public iGraphNodeCallback
{
private:
  EntityMode* emode;

public:
  GraphNodeCallback (EntityMode* emode) : emode (emode) { }
  virtual ~GraphNodeCallback () { }
  virtual void ActivateNode (const char* nodeName)
  {
    emode->ActivateNode (nodeName);
  }
};

//---------------------------------------------------------------------------

class RenameQuestAction : public Ares::Action
{
private:
  EntityMode* entityMode;

public:
  RenameQuestAction (EntityMode* entityMode) : entityMode (entityMode) { }
  virtual ~RenameQuestAction () { }
  virtual const char* GetName () const { return "Rename Quest..."; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    if (value)
    {
      csString questName = value->GetStringArrayValue ()->Get (QUEST_COL_NAME);
      entityMode->OnRenameQuest (questName);
    }
    return true;
  }
  virtual bool IsActive (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    return value != 0;
  }
};

//---------------------------------------------------------------------------

class DeleteQuestAction : public Ares::Action
{
private:
  EntityMode* entityMode;

public:
  DeleteQuestAction (EntityMode* entityMode) : entityMode (entityMode) { }
  virtual ~DeleteQuestAction () { }
  virtual const char* GetName () const { return "Delete Quest..."; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    if (value)
    {
      csString questName = value->GetStringArrayValue ()->Get (QUEST_COL_NAME);
      entityMode->OnQuestDel (questName);
    }
    return true;
  }
  virtual bool IsActive (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    return value != 0;
  }
};

//---------------------------------------------------------------------------

class AddQuestAction : public Ares::Action
{
private:
  EntityMode* entityMode;

public:
  AddQuestAction (EntityMode* entityMode) : entityMode (entityMode) { }
  virtual ~AddQuestAction () { }
  virtual const char* GetName () const { return "Add Quest..."; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    entityMode->AskNewQuest ();
    return true;
  }
};

//---------------------------------------------------------------------------

class RenameTemplateAction : public Ares::Action
{
private:
  EntityMode* entityMode;

public:
  RenameTemplateAction (EntityMode* entityMode) : entityMode (entityMode) { }
  virtual ~RenameTemplateAction () { }
  virtual const char* GetName () const { return "Rename Template..."; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    if (value)
    {
      csString tplName = value->GetStringArrayValue ()->Get (TEMPLATE_COL_NAME);
      entityMode->OnRenameTemplate (tplName);
    }
    return true;
  }
  virtual bool IsActive (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    return value != 0;
  }
};

//---------------------------------------------------------------------------

class DeleteTemplateAction : public Ares::Action
{
private:
  EntityMode* entityMode;

public:
  DeleteTemplateAction (EntityMode* entityMode) : entityMode (entityMode) { }
  virtual ~DeleteTemplateAction () { }
  virtual const char* GetName () const { return "Delete Template..."; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    if (value)
    {
      csString templateName = value->GetStringArrayValue ()->Get (TEMPLATE_COL_NAME);
      entityMode->OnTemplateDel (templateName);
    }
    return true;
  }
  virtual bool IsActive (Ares::View* view, wxWindow* component)
  {
    Ares::Value* value = view->GetSelectedValue (component);
    return value != 0;
  }
};

//---------------------------------------------------------------------------

class AddTemplateAction : public Ares::Action
{
private:
  EntityMode* entityMode;

public:
  AddTemplateAction (EntityMode* entityMode) : entityMode (entityMode) { }
  virtual ~AddTemplateAction () { }
  virtual const char* GetName () const { return "Add Template..."; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    entityMode->AskNewTemplate ();
    return true;
  }
};

//---------------------------------------------------------------------------

EntityMode::EntityMode (iBase* parent) : scfImplementationType (this, parent)
{
  name = "Entity";
  panel = 0;
  contextLastProperty = 0;
}

bool EntityMode::Initialize (iObjectRegistry* object_reg)
{
  if (!EditingMode::Initialize (object_reg)) return false;

  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  iGraphics2D* g2d = g3d->GetDriver2D ();
  font = g2d->GetFontServer ()->LoadFont ("DejaVuSans", 10);
  fontBold = g2d->GetFontServer ()->LoadFont ("DejaVuSansBold", 10);
  fontLarge = g2d->GetFontServer ()->LoadFont ("DejaVuSans", 13);

  ID_Copy = pl->FetchStringID ("Copy");
  ID_Paste = pl->FetchStringID ("Paste");
  ID_Delete = pl->FetchStringID ("Delete");

  questMgr = csQueryRegistryOrLoad<iQuestManager> (object_reg,
      "cel.manager.quests");

  wxPGInitResourceModule ();

  return true;
}

void EntityMode::SetTopLevelParent (wxWindow* toplevel)
{
}

// ------------------------------------------------------------------------

PcEditorSupport::PcEditorSupport (const char* name, EntityMode* emode) : name (name), emode (emode)
{
  pl = emode->GetPL ();
}


class PcEditorSupportQuest : public PcEditorSupport
{
private:
  int idNewPar;
  int idDelPar;

public:
  PcEditorSupportQuest (EntityMode* emode) : PcEditorSupport ("pclogic.quest", emode)
  {
    iUIManager* ui = emode->GetApplication ()->GetUI ();
    idNewPar = ui->AllocContextMenuID ();
    emode->panel->Connect (idNewPar, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (EntityMode::Panel::PcQuest_OnNewParameter), 0, emode->panel);
    idDelPar = ui->AllocContextMenuID ();
    emode->panel->Connect (idDelPar, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (EntityMode::Panel::PcQuest_OnDelParameter), 0, emode->panel);
  }

  virtual ~PcEditorSupportQuest () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    csString questName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
    emode->AppendButtonPar (pcProp, "Quest", "Q:", questName);
    size_t nqIdx = pctpl->FindProperty (pl->FetchStringID ("NewQuest"));
    if (nqIdx != csArrayItemNotFound)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> it = pctpl->GetProperty (nqIdx, id, data);
      while (it->HasNext ())
      {
	csStringID nextID;
	iParameter* nextPar = it->Next (nextID);
	csString name = pl->FetchString (nextID);
	if (name == "name") continue;
	csString val = nextPar->GetOriginalExpression ();
	emode->AppendPar (pcProp, "Par", name, nextPar->GetPossibleType (), val);
      }
    }
  }

  virtual bool Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName)
  {
    csString questName = emode->GetPropertyValueAsString (pcPropName, "Q:Quest");
    csString oldQuestName = emode->GetQuestName (pctpl);
    if (questName != oldQuestName)
    {
      pctpl->RemoveAllProperties ();
      InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", "name");
      csRef<iParameter> par = emode->GetPM ()->GetParameter (questName, CEL_DATA_STRING);
      InspectTools::AddActionParameter (pl, pctpl, "NewQuest", "name", par);
      return true;
    }

    if (selectedPropName.StartsWith ("Par:") && !selectedPropName.EndsWith (".Value")
	&& !selectedPropName.EndsWith (".Type") && !selectedPropName.EndsWith (".Name"))
    {
      csString oldParName = selectedPropName.GetData () + 4;
      csString newParName = emode->GetPropertyValueAsString (pcPropName, selectedPropName+".Name");
      csString newTypeS = emode->GetPropertyValueAsString (pcPropName, selectedPropName+".Type");
      csString newValue = emode->GetPropertyValueAsString (pcPropName, selectedPropName+".Value");
      printf ("oldParName=%s new=%s/%s/%s\n", oldParName.GetData (), newParName.GetData (),
	  newTypeS.GetData (), newValue.GetData ()); fflush (stdout);

      if (oldParName != newParName)
	InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", oldParName);
      InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", newParName);
      celDataType newType = InspectTools::StringToType (newTypeS);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (newValue, newType);
      InspectTools::AddActionParameter (pl, pctpl, "NewQuest", newParName, par);
      return true;
    }

    return false;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value)
  {
    if (selectedPropName.StartsWith ("Par:") && selectedPropName.EndsWith (".Name"))
    {
      csString oldParName = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      iUIManager* ui = emode->GetApplication ()->GetUI ();
      if (value.IsEmpty ())
      {
	ui->Error ("Empty name is not allowed!");
	return false;
      }
      else if (oldParName != value)
      {
	iParameter* par = InspectTools::GetActionParameterValue (pl, pctpl,
	    "NewQuest", value);
	if (par)
	{
	  ui->Error ("There is already a parameter with this name!");
	  return false;
	}
      }
    }
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->Append (idNewPar, wxT ("New Quest Parameter..."));
    if (selectedPropName.StartsWith ("Par:"))
      contextMenu->Append (idDelPar, wxT ("Delete Quest Parameter"));
  }
};

class PcEditorSupportProperties : public PcEditorSupport
{
private:
  int idNewProp;
  int idDelProp;

public:
  PcEditorSupportProperties (EntityMode* emode) : PcEditorSupport ("pctools.properties", emode)
  {
    iUIManager* ui = emode->GetApplication ()->GetUI ();
    idNewProp = ui->AllocContextMenuID ();
    emode->panel->Connect (idNewProp, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (EntityMode::Panel::PcProp_OnNewProperty), 0, emode->panel);
    idDelProp = ui->AllocContextMenuID ();
    emode->panel->Connect (idDelProp, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (EntityMode::Panel::PcProp_OnDelProperty), 0, emode->panel);
  }
  virtual ~PcEditorSupportProperties () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString value;
      celParameterTools::ToString (data, value);
      csString name = pl->FetchString (id);
      emode->AppendPar (pcProp, "Prop", name, InspectTools::ResolveType (data), value);
    }
  }

  virtual bool Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName)
  {
    if (selectedPropName.StartsWith ("Prop:") && !selectedPropName.EndsWith (".Value")
	&& !selectedPropName.EndsWith (".Type") && !selectedPropName.EndsWith (".Name"))
    {
      csString oldParName = selectedPropName.GetData () + 5;
      csString newParName = emode->GetPropertyValueAsString (pcPropName, selectedPropName+".Name");
      csString newTypeS = emode->GetPropertyValueAsString (pcPropName, selectedPropName+".Type");
      csString newValue = emode->GetPropertyValueAsString (pcPropName, selectedPropName+".Value");
      printf ("oldPropName=%s new=%s/%s/%s\n", oldParName.GetData (), newParName.GetData (),
	  newTypeS.GetData (), newValue.GetData ()); fflush (stdout);

      if (oldParName != newParName)
	pctpl->RemoveProperty (pl->FetchStringID (oldParName));
      celDataType newType = InspectTools::StringToType (newTypeS);
      InspectTools::SetProperty (pl, pctpl, newType, newParName, newValue);
      return true;
    }

    return false;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value)
  {
    if (selectedPropName.StartsWith ("Prop:") && selectedPropName.EndsWith (".Name"))
    {
      iUIManager* ui = emode->GetApplication ()->GetUI ();
      csString oldParName = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      if (value.IsEmpty ())
      {
	ui->Error ("Empty name is not allowed!");
	return false;
      }
      else if (oldParName != value)
      {
	if (pctpl->FindProperty (pl->FetchStringID (value)) != csArrayItemNotFound)
	{
	  ui->Error ("There is already a property with this name!");
	  return false;
	}
      }
    }
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->Append (idNewProp, wxT ("New Property..."));
    if (selectedPropName.StartsWith ("Prop:"))
      contextMenu->Append (idDelProp, wxT ("Delete Property"));
  }
};

class PcEditorSupportTrigger : public PcEditorSupport
{
public:
  PcEditorSupportTrigger (EntityMode* emode) : PcEditorSupport ("pclogic.trigger", emode) { }
  virtual ~PcEditorSupportTrigger () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    bool valid;
    long delay = InspectTools::GetPropertyValueLong (pl, pctpl, "delay", &valid);
    emode->GetDetailGrid ()->AppendIn (pcProp, new wxIntProperty (wxT ("Delay"), wxT ("Delay"),
	  valid ? delay : 200));
    long jitter = InspectTools::GetPropertyValueLong (pl, pctpl, "jitter", &valid);
    emode->GetDetailGrid ()->AppendIn (pcProp, new wxIntProperty (wxT ("Jitter"), wxT ("Jitter"),
	  valid ? jitter : 10));

    csString monitor = InspectTools::GetPropertyValueString (pl, pctpl, "monitor", &valid);
    emode->AppendButtonPar (pcProp, "Monitor", "E:", monitor);

    csString clazz = InspectTools::GetPropertyValueString (pl, pctpl, "class", &valid);
    emode->AppendButtonPar (pcProp, "Class", "C:", clazz);

    wxPGProperty* typeProp = emode->GetDetailGrid ()->AppendIn (pcProp,
	new wxEnumProperty (wxT ("Type"), wxT ("TrigType"), emode->trigtypesArray, wxArrayInt()));
    if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerSphere")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Sphere"));
      iParameter* par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerSphere", "radius");
      emode->GetDetailGrid ()->AppendIn (typeProp, new wxStringProperty (wxT ("Radius"), wxPG_LABEL,
	  par ? wxString::FromUTF8 (par->GetOriginalExpression ()) : wxT ("")));
      par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerSphere", "position");
      emode->GetDetailGrid ()->AppendIn (typeProp, new wxStringProperty (wxT ("Position"), wxPG_LABEL,
	  par ? wxString::FromUTF8 (par->GetOriginalExpression ()) : wxT ("")));
    }
    else if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerBox")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Box"));
      iParameter* par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerBox", "minbox");
      emode->GetDetailGrid ()->AppendIn (typeProp, new wxStringProperty (wxT ("MinBox"), wxPG_LABEL,
	  par ? wxString::FromUTF8 (par->GetOriginalExpression ()) : wxT ("")));
      par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerBox", "maxbox");
      emode->GetDetailGrid ()->AppendIn (typeProp, new wxStringProperty (wxT ("MaxBox"), wxPG_LABEL,
	  par ? wxString::FromUTF8 (par->GetOriginalExpression ()) : wxT ("")));
    }
    else if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerBeam")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Beam"));
      iParameter* par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerBeam", "start");
      emode->GetDetailGrid ()->AppendIn (typeProp, new wxStringProperty (wxT ("Start"), wxPG_LABEL,
	  par ? wxString::FromUTF8 (par->GetOriginalExpression ()) : wxT ("")));
      par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerBeam", "end");
      emode->GetDetailGrid ()->AppendIn (typeProp, new wxStringProperty (wxT ("End"), wxPG_LABEL,
	  par ? wxString::FromUTF8 (par->GetOriginalExpression ()) : wxT ("")));
    }
    else if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerAboveMesh")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Above"));
      iParameter* par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerAboveMesh", "entity");
      emode->AppendButtonPar (typeProp, "Entity", "E:", par ? par->GetOriginalExpression () : "");
      par = InspectTools::GetActionParameterValue (pl, pctpl, "SetupTriggerAboveMesh", "maxdistance");
      emode->GetDetailGrid ()->AppendIn (typeProp, new wxStringProperty (wxT ("MaxDistance"), wxPG_LABEL,
	  par ? wxString::FromUTF8 (par->GetOriginalExpression ()) : wxT ("")));
    }
    else
    {
      printf ("Huh! Unknown trigger type!\n");
    }
  }

  virtual bool Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName)
  {
    if (selectedPropName == "Delay")
    {
      long delay = emode->GetPropertyValueAsInt (pcPropName, selectedPropName);
      pctpl->SetProperty (pl->FetchStringID ("delay"), delay);
      return true;
    }
    else if (selectedPropName == "Jitter")
    {
      long jitter = emode->GetPropertyValueAsInt (pcPropName, selectedPropName);
      pctpl->SetProperty (pl->FetchStringID ("jitter"), jitter);
      return true;
    }
    else if (selectedPropName == "Monitor")
    {
      csString monitor = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      pctpl->SetProperty (pl->FetchStringID ("monitor"), monitor.GetData ());
      return true;
    }
    else if (selectedPropName == "Class")
    {
      csString clazz = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      pctpl->SetProperty (pl->FetchStringID ("class"), clazz.GetData ());
      return true;
    }
    else if (selectedPropName == "TrigType")
    {
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerBox"));
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerSphere"));
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerBeam"));
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerAboveMesh"));
      csString type = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      printf ("type=%s\n", type.GetData ()); fflush (stdout);
      if (type == "Sphere")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerSphere", CEL_DATA_NONE);
	return true;
      }
      else if (type == "Box")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerBox", CEL_DATA_NONE);
	return true;
      }
      else if (type == "Beam")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerBeam", CEL_DATA_NONE);
	return true;
      }
      else if (type == "Above")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerAboveMesh", CEL_DATA_NONE);
	return true;
      }
    }
    else if (selectedPropName == "TrigType.Radius")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerSphere", "radius");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_FLOAT);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerSphere", "radius", par);
      return true;
    }
    else if (selectedPropName == "TrigType.Position")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerSphere", "position");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_VECTOR3);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerSphere", "position", par);
      return true;
    }
    else if (selectedPropName == "TrigType.MinBox")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerBox", "minbox");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_VECTOR3);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerBox", "minbox", par);
      return true;
    }
    else if (selectedPropName == "TrigType.MaxBox")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerBox", "maxbox");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_VECTOR3);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerBox", "maxbox", par);
      return true;
    }
    else if (selectedPropName == "TrigType.Start")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerBeam", "start");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_VECTOR3);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerBeam", "start", par);
      return true;
    }
    else if (selectedPropName == "TrigType.End")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerBeam", "end");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_VECTOR3);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerBeam", "end", par);
      return true;
    }
    else if (selectedPropName == "TrigType.Entity")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerAboveMesh", "entity");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_STRING);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerAboveMesh", "entity", par);
      return true;
    }
    else if (selectedPropName == "TrigType.MaxDistance")
    {
      InspectTools::DeleteActionParameter (pl, pctpl, "SetupTriggerAboveMesh", "maxdistance");
      csString value = emode->GetPropertyValueAsString (pcPropName, selectedPropName);
      csRef<iParameter> par = emode->GetPM ()->GetParameter (value, CEL_DATA_FLOAT);
      InspectTools::AddActionParameter (pl, pctpl, "SetupTriggerAboveMesh", "maxdistance", par);
      return true;
    }
    return false;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value)
  {
    return true;
  }
};


// ------------------------------------------------------------------------

static wxPGProperty* FindPCProperty (wxPGProperty* prop)
{
  while (prop)
  {
    csString propName = (const char*)prop->GetName ().mb_str (wxConvUTF8);
    if (propName.StartsWith ("PC:")) return prop;
    prop = prop->GetParent ();
  }
  return prop;
}


void EntityMode::OnPropertyGridButton (wxCommandEvent& event)
{
  using namespace Ares;

  wxPGProperty* selectedProperty = detailGrid->GetSelection ();
  if (selectedProperty)
  {
    csString propName = (const char*)selectedProperty->GetName ().mb_str (wxConvUTF8);
    iUIManager* ui = view3d->GetApplication ()->GetUI ();

    size_t dot = propName.FindFirst ('.');
    if (dot != csArrayItemNotFound)
      propName = propName.Slice (dot+1);

    Value* chosen = 0;
    int col;
    if (propName.StartsWith ("E:"))
    {
      csRef<Value> objects = view3d->GetModelRepository ()->GetObjectsWithEntityValue ();
      chosen = ui->AskDialog (
	  "Select an entity", objects, "Entity,Template,Dynfact,Logic",
	    DYNOBJ_COL_ENTITY, DYNOBJ_COL_TEMPLATE, DYNOBJ_COL_FACTORY, DYNOBJ_COL_LOGIC);
      col = DYNOBJ_COL_ENTITY;
    }
    else if (propName.StartsWith ("Q:"))
    {
      csRef<Value> objects = view3d->GetModelRepository ()->GetQuestsValue ();
      chosen = ui->AskDialog ("Select a quest", objects, "Name,M", QUEST_COL_NAME,
	    QUEST_COL_MODIFIED);
      col = QUEST_COL_NAME;
    }
    else if (propName.StartsWith ("C:"))
    {
      Value* objects = view3d->GetModelRepository ()->GetClassesValue ();
      chosen = ui->AskDialog ("Select a class", objects, "Class,Description", CLASS_COL_NAME,
	    CLASS_COL_DESCRIPTION);
      col = CLASS_COL_NAME;
    }
    if (chosen)
    {
      csString n = chosen->GetStringArrayValue ()->Get (col);
      selectedProperty->SetValue (wxString::FromUTF8 (n));
      OnPropertyGridChanged (selectedProperty);
    }
  }
}

iCelPropertyClassTemplate* EntityMode::GetPCForProperty (wxPGProperty* property,
    csString& pcPropName, csString& selectedPropName)
{
  selectedPropName = (const char*)property->GetName ().mb_str (wxConvUTF8);
  wxPGProperty* pcProperty = FindPCProperty (property);
  if (pcProperty)
  {
    pcPropName = (const char*)pcProperty->GetName ().mb_str (wxConvUTF8);
    int idx;
    csScanStr (pcPropName.GetData () + 3, "%d", &idx);
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    return tpl->GetPropertyClassTemplate (idx);
  }
  return 0;
}

void EntityMode::OnContextMenu (wxContextMenuEvent& event)
{
  printf ("OnContextMenu\n"); fflush (stdout);
  wxWindow* gridWindow = wxStaticCast (detailGrid, wxWindow);
  wxWindow* component = wxStaticCast (event.GetEventObject (), wxWindow);
  while (gridWindow != component && component)
    component = component->GetParent ();
  if (component == gridWindow)
  {
    wxPropertyGridHitTestResult rc = detailGrid->HitTest (detailGrid->ScreenToClient (event.GetPosition ()));
    contextLastProperty = rc.GetProperty ();

    if (contextLastProperty)
    {
      csString selectedPropName, pcPropName;
      iCelPropertyClassTemplate* pctpl = GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);

      wxMenu contextMenu;
      contextMenu.Append (idNewChar, wxT ("New characteristic..."));
      if (selectedPropName.StartsWith ("Char:"))
        contextMenu.Append (idDelChar, wxT ("Delete characteristic"));
      if (pctpl)
      {
        PcEditorSupport* editor = editors.Get (pctpl->GetName (), 0);
        if (editor)
        {
          editor->DoContext (pctpl, pcPropName, selectedPropName, &contextMenu);
        }
      }

      panel->PopupMenu (&contextMenu);
    }
  }
}

bool EntityMode::ValidateTemplateParentsFromGrid (wxPropertyGridEvent& event)
{
  wxArrayString templates = event.GetValue ().GetArrayString ();
  for (size_t i = 0 ; i < templates.GetCount () ; i++)
  {
    csString name = (const char*)templates.Item (i).mb_str (wxConvUTF8);
    iCelEntityTemplate* parent = pl->FindEntityTemplate (name);
    if (!parent)
    {
      iUIManager* ui = app->GetUI ();
      ui->Error ("Can't find template '%s'!", name.GetData ());
      return false;
    }
  }
  return true;
}

void EntityMode::OnPropertyGridChanging (wxPropertyGridEvent& event)
{
  wxPGProperty* selectedProperty = event.GetProperty ();
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (selectedProperty, pcPropName, selectedPropName);
  printf ("PG changing %s/%s!\n", selectedPropName.GetData (), pcPropName.GetData ()); fflush (stdout);
  if (pctpl)
  {
    csString value = (const char*)event.GetValue ().GetString ().mb_str (wxConvUTF8);
    printf ("Prop name: %s\n", selectedPropName.GetData ()); fflush (stdout);
    if (!ValidateGridChange (pctpl, pcPropName, selectedPropName, value))
      event.Veto ();
  }
  else if (selectedPropName == "Parents")
  {
    if (!ValidateTemplateParentsFromGrid (event))
      event.Veto ();
  }
  else if (selectedPropName == "Classes")
  {
  }
  else if (selectedPropName.StartsWith ("Char:"))
  {
    csString value = (const char*)event.GetValue ().GetString ().mb_str (wxConvUTF8);
    if (selectedPropName.EndsWith (".Value"))
    {
      char* endptr;
      strtod (value, &endptr);
      if (*endptr)
	event.Veto ();
    }
    else if (selectedPropName.EndsWith (".Name"))
    {
      iUIManager* ui = view3d->GetApplication ()->GetUI ();
      csString oldName = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);
      if (value.IsEmpty ())
      {
        ui->Error ("Empty name is not allowed!");
        return;
      }
      else if (oldName != value)
      {
	iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
	if (tpl->GetCharacteristics ()->HasCharacteristic (value))
	{
          ui->Error ("There is already a characteristic with this name!");
	  event.Veto ();
	}
      }
    }
  }
}

void EntityMode::OnPropertyGridChanged (wxPropertyGridEvent& event)
{
  wxPGProperty* selectedProperty = event.GetProperty ();
  OnPropertyGridChanged (selectedProperty);
}

void EntityMode::UpdateCharacteristicFromGrid (wxPGProperty* property, const csString& propName)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  csString oldname = propName.Slice (5);
  csString newname = (const char*)(detailGrid->GetPropertyValueAsString (wxString::FromUTF8 (
	propName + ".Name")).mb_str (wxConvUTF8));
  csString newvalue = (const char*)(detailGrid->GetPropertyValueAsString (wxString::FromUTF8 (
	propName + ".Value")).mb_str (wxConvUTF8));
  if (oldname != newname)
    tpl->GetCharacteristics ()->ClearCharacteristic (oldname);
  tpl->GetCharacteristics ()->ClearCharacteristic (newname);
  float value;
  csScanStr (newvalue, "%f", &value);
  tpl->GetCharacteristics ()->SetCharacteristic (newname, value);
  PCWasEdited (0);
  SelectTemplate (tpl);
}

void EntityMode::UpdateTemplateClassesFromGrid ()
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  wxArrayString classes = detailGrid->GetPropertyValueAsArrayString (wxT ("Classes"));
  csStringArray classesToAdd;
  csStringArray classesToRemove;
  for (size_t i = 0 ; i < classes.GetCount () ; i++)
    classesToAdd.Push (classes.Item (i).mb_str (wxConvUTF8));

  const csSet<csStringID>& tplClasses = tpl->GetClasses ();
  csSet<csStringID>::GlobalIterator it = tplClasses.GetIterator ();
  while (it.HasNext ())
  {
    csStringID classID = it.Next ();
    csString name = pl->FetchString (classID);
    if (classes.Index (wxString::FromUTF8 (name)) == wxNOT_FOUND)
      classesToRemove.Push (name);
    else
      classesToAdd.Delete (name);
  }
  if (classesToRemove.GetSize () > 0 || classesToAdd.GetSize () > 0)
  {
    for (size_t i = 0 ; i < classesToAdd.GetSize () ; i++)
    {
      csStringID id = pl->FetchStringID (classesToAdd.Get (i));
      tpl->AddClass (id);
    }
    for (size_t i = 0 ; i < classesToRemove.GetSize () ; i++)
    {
      csStringID id = pl->FetchStringID (classesToRemove.Get (i));
      tpl->RemoveClass (id);
    }
    PCWasEdited (0);
    SelectTemplate (tpl);
  }
}

void EntityMode::UpdateTemplateParentsFromGrid ()
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  wxArrayString templates = detailGrid->GetPropertyValueAsArrayString (wxT ("Parents"));
  csStringArray templatesToAdd;
  csStringArray templatesToRemove;
  for (size_t i = 0 ; i < templates.GetCount () ; i++)
    templatesToAdd.Push (templates.Item (i).mb_str (wxConvUTF8));
  csRef<iCelEntityTemplateIterator> it = tpl->GetParents ();
  while (it->HasNext ())
  {
    csString name = it->Next ()->GetName ();
    if (templates.Index (wxString::FromUTF8 (name)) == wxNOT_FOUND)
      templatesToRemove.Push (name);
    else
      templatesToAdd.Delete (name);
  }
  if (templatesToRemove.GetSize () > 0 || templatesToAdd.GetSize () > 0)
  {
    for (size_t i = 0 ; i < templatesToAdd.GetSize () ; i++)
    {
      iCelEntityTemplate* parent = pl->FindEntityTemplate (templatesToAdd.Get (i));
      tpl->AddParent (parent);
    }
    for (size_t i = 0 ; i < templatesToRemove.GetSize () ; i++)
    {
      iCelEntityTemplate* parent = pl->FindEntityTemplate (templatesToRemove.Get (i));
      tpl->RemoveParent (parent);
    }
    PCWasEdited (0);
    SelectTemplate (tpl);
  }
}

void EntityMode::OnPropertyGridChanged (wxPGProperty* selectedProperty)
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (selectedProperty, pcPropName, selectedPropName);
  printf ("PG changed %s/%s!\n", selectedPropName.GetData (), pcPropName.GetData ()); fflush (stdout);
  if (pctpl)
  {
    printf ("Prop name: %s\n", pcPropName.GetData ()); fflush (stdout);
    if (UpdatePCFromGrid (pctpl, pcPropName, selectedPropName))
    {
      PCWasEdited (pctpl);
    }
  }
  else if (selectedPropName == "Parents")
  {
    UpdateTemplateParentsFromGrid ();
  }
  else if (selectedPropName == "Classes")
  {
    UpdateTemplateClassesFromGrid ();
  }
  else if (selectedPropName.StartsWith ("Char:") && !selectedPropName.EndsWith (".Value")
	&& !selectedPropName.EndsWith (".Name"))
  {
    UpdateCharacteristicFromGrid (selectedProperty, selectedPropName);
  }
}

void EntityMode::RegisterEditor (PcEditorSupport* editor)
{
  editors.Put (editor->GetName (), editor);
  editor->DecRef ();
}

void EntityMode::BuildDetailGrid ()
{
  wxPanel* detailPanel = XRCCTRL (*panel, "detail_Panel", wxPanel);

  detailGrid = new wxPropertyGrid (detailPanel);
  detailGrid->SetId (PG_ID);
  detailPanel->GetSizer ()->Add (detailGrid, 1, wxEXPAND | wxALL);
  //detailGrid->SetColumnCount (3);

  RegisterEditor (new PcEditorSupportQuest (this));
  RegisterEditor (new PcEditorSupportProperties (this));
  RegisterEditor (new PcEditorSupportTrigger (this));

  typesArray.Add (wxT ("string"));  typesArrayIdx.Add (CEL_DATA_STRING);
  typesArray.Add (wxT ("float"));   typesArrayIdx.Add (CEL_DATA_FLOAT);
  typesArray.Add (wxT ("long"));    typesArrayIdx.Add (CEL_DATA_LONG);
  typesArray.Add (wxT ("bool"));    typesArrayIdx.Add (CEL_DATA_BOOL);
  typesArray.Add (wxT ("vector2")); typesArrayIdx.Add (CEL_DATA_VECTOR2);
  typesArray.Add (wxT ("vector3")); typesArrayIdx.Add (CEL_DATA_VECTOR3);
  typesArray.Add (wxT ("color"));   typesArrayIdx.Add (CEL_DATA_COLOR);

  pctypesArray.Add (wxT ("pcobject.mesh"));
  pctypesArray.Add (wxT ("pctools.properties"));
  pctypesArray.Add (wxT ("pctools.inventory"));
  pctypesArray.Add (wxT ("pclogic.quest"));
  pctypesArray.Add (wxT ("pclogic.spawn"));
  pctypesArray.Add (wxT ("pclogic.trigger"));
  pctypesArray.Add (wxT ("pclogic.wire"));
  pctypesArray.Add (wxT ("pctools.messenger"));
  pctypesArray.Add (wxT ("pcinput.standard"));
  pctypesArray.Add (wxT ("pcphysics.object"));
  pctypesArray.Add (wxT ("pcphysics.system"));
  pctypesArray.Add (wxT ("pccamera.old"));
  pctypesArray.Add (wxT ("pcmove.actor.dynamic"));
  pctypesArray.Add (wxT ("pcmove.actor.standard"));
  pctypesArray.Add (wxT ("pcmove.actor.wasd"));
  pctypesArray.Add (wxT ("pcworld.dynamic"));
  pctypesArray.Add (wxT ("ares.gamecontrol"));

  trigtypesArray.Add (wxT ("Sphere"));
  trigtypesArray.Add (wxT ("Box"));
  trigtypesArray.Add (wxT ("Beam"));
  trigtypesArray.Add (wxT ("Above"));
}

void EntityMode::AppendPar (
    wxPGProperty* parent, const char* partype,
    const char* name, celDataType type, const char* value)
{
  csString s;
  s.Format ("%s:%s", partype, name);
  wxPGProperty* parProp = detailGrid->AppendIn (parent,
      new wxStringProperty (wxString::FromUTF8 (partype),
	wxString::FromUTF8 (s), wxT ("<composed>")));
  detailGrid->AppendIn (parProp, new wxStringProperty (wxT ("Name"), wxT ("Name"),
	wxString::FromUTF8 (name)));
  if (type != CEL_DATA_NONE)
    detailGrid->AppendIn (parProp, new wxEnumProperty (wxT ("Type"), wxPG_LABEL,
	typesArray, typesArrayIdx, type));
  detailGrid->AppendIn (parProp, new wxStringProperty (wxT ("Value"), wxT ("Value"),
	wxString::FromUTF8 (value)));
  detailGrid->Collapse (parProp);
}

void EntityMode::AppendButtonPar (
    wxPGProperty* parent, const char* partype, const char* type, const char* name)
{
  wxStringProperty* prop = new wxStringProperty (
      wxString::FromUTF8 (partype),
      wxString::FromUTF8 (csString (type) + partype),
      wxString::FromUTF8 (name));
  detailGrid->AppendIn (parent, prop);
  detailGrid->SetPropertyEditor (prop, wxPGEditor_TextCtrlAndButton);
}

// -----------------------------------------------------------------------
// Templates Property
// -----------------------------------------------------------------------

//WX_PG_DECLARE_ARRAYSTRING_PROPERTY_WITH_DECL(wxTemplatesProperty, class wxEMPTY_PARAMETER_VALUE)
class wxEMPTY_PARAMETER_VALUE wxPG_PROPCLASS(wxTemplatesProperty) : public wxPG_PROPCLASS(wxArrayStringProperty)
{
  WX_PG_DECLARE_PROPERTY_CLASS(wxPG_PROPCLASS(wxTemplatesProperty))
  EntityMode* emode;

public:
  wxPG_PROPCLASS(wxTemplatesProperty)( const wxString& label = wxPG_LABEL,
      const wxString& name = wxPG_LABEL, const wxArrayString& value = wxArrayString());
  virtual ~wxPG_PROPCLASS(wxTemplatesProperty)();
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


WX_PG_IMPLEMENT_ARRAYSTRING_PROPERTY(wxTemplatesProperty, wxT (','), wxT ("Browse"))

bool wxTemplatesProperty::OnCustomStringEdit( wxWindow* parent, wxString& value )
{
  using namespace Ares;
  csRef<Value> objects = emode->Get3DView ()->GetModelRepository ()->GetTemplatesValue ();
  iUIManager* ui = emode->GetApplication ()->GetUI ();
  Value* chosen = ui->AskDialog ("Select a template", objects, "Template,M", TEMPLATE_COL_NAME,
	    TEMPLATE_COL_MODIFIED);
  if (chosen)
  {
    csString name = chosen->GetStringArrayValue ()->Get (TEMPLATE_COL_NAME);
    value = wxString::FromUTF8 (name);
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------
// Classes Property
// -----------------------------------------------------------------------

class wxEMPTY_PARAMETER_VALUE wxPG_PROPCLASS(wxClassesProperty) : public wxPG_PROPCLASS(wxArrayStringProperty)
{
  WX_PG_DECLARE_PROPERTY_CLASS(wxPG_PROPCLASS(wxClassesProperty))
  EntityMode* emode;

public:
  wxPG_PROPCLASS(wxClassesProperty)( const wxString& label = wxPG_LABEL,
      const wxString& name = wxPG_LABEL, const wxArrayString& value = wxArrayString());
  virtual ~wxPG_PROPCLASS(wxClassesProperty)();
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


WX_PG_IMPLEMENT_ARRAYSTRING_PROPERTY(wxClassesProperty, wxT (','), wxT ("Browse"))

bool wxClassesProperty::OnCustomStringEdit( wxWindow* parent, wxString& value )
{
  using namespace Ares;
  Value* classes = emode->Get3DView ()->GetModelRepository ()->GetClassesValue ();
  iUIManager* ui = emode->GetApplication ()->GetUI ();
  Value* chosen = ui->AskDialog ("Select a class", classes, "Class,Description", CLASS_COL_NAME,
	    CLASS_COL_DESCRIPTION);
  if (chosen)
  {
    csString name = chosen->GetStringArrayValue ()->Get (CLASS_COL_NAME);
    value = wxString::FromUTF8 (name);
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------

void EntityMode::AppendTemplatesPar (
    wxPGProperty* parentProp, iCelEntityTemplateIterator* it, const char* partype)
{
  wxArrayString parentArray;
  while (it->HasNext ())
  {
    iCelEntityTemplate* parent = it->Next ();
    parentArray.Add (wxString::FromUTF8 (parent->GetName ()));
  }
  wxTemplatesProperty* tempProp = new wxTemplatesProperty (
	wxString::FromUTF8 (partype),
	wxPG_LABEL,
	parentArray);
  tempProp->SetEntityMode (this);
  detailGrid->AppendIn (parentProp, tempProp);
}

void EntityMode::AppendClassesPar (
    wxPGProperty* parentProp, csSet<csStringID>::GlobalIterator* it, const char* partype)
{
  wxArrayString classesArray;
  while (it->HasNext ())
  {
    csStringID classID = it->Next ();
    csString className = pl->FetchString (classID);
    classesArray.Add (wxString::FromUTF8 (className));
  }
  wxClassesProperty* classProp = new wxClassesProperty (
	wxString::FromUTF8 (partype),
	wxPG_LABEL,
	classesArray);
  classProp->SetEntityMode (this);
  detailGrid->AppendIn (parentProp, classProp);
}

void EntityMode::AppendCharacteristics (wxPGProperty* parentProp, iCelEntityTemplate* tpl)
{
    csRef<iCharacteristicsIterator> it = tpl->GetCharacteristics ()->GetAllCharacteristics ();
    while (it->HasNext ())
    {
      float f;
      csString name = it->Next (f);
      csString value;
      value.Format ("%g", f);
      AppendPar (parentProp, "Char", name, CEL_DATA_NONE, value);
    }
}

void EntityMode::FillDetailGrid (iCelEntityTemplate* tpl)
{
  detailGrid->Freeze ();
  detailGrid->Clear ();

  if (!tpl)
  {
    detailGrid->Thaw ();
    return;
  }

  csString s;
  s.Format ("Template (%s)", tpl->GetName ());
  wxPGProperty* templateProp = detailGrid->Append (new wxPropertyCategory (wxString::FromUTF8 (s)));

  csRef<iCelEntityTemplateIterator> parentIt = tpl->GetParents ();
  AppendTemplatesPar (templateProp, parentIt, "Parents");

  const csSet<csStringID>& classes = tpl->GetClasses ();
  csSet<csStringID>::GlobalIterator classIt = classes.GetIterator ();
  AppendClassesPar (templateProp, &classIt, "Classes");

  AppendCharacteristics (templateProp, tpl);

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
    s.Format ("PC:%d", int (i));
    wxPGProperty* pcProp = detailGrid->AppendIn (templateProp,
      new wxPropertyCategory (wxT ("PC"), wxString::FromUTF8 (s)));

    detailGrid->AppendIn (pcProp, new wxEnumProperty (wxT ("Type"), wxPG_LABEL,
	pctypesArray, wxArrayInt(), pctypesArray.Index (wxString::FromUTF8 (pctpl->GetName ()))));
    detailGrid->AppendIn (pcProp, new wxStringProperty (wxT ("Tag"), wxPG_LABEL,
	wxString::FromUTF8 (pctpl->GetTag ())));

    PcEditorSupport* editor = editors.Get (pctpl->GetName (), 0);
    if (editor) editor->Fill (pcProp, pctpl);
  }
  detailGrid->FitColumns ();
  detailGrid->Thaw ();
}

csString EntityMode::GetPropertyValueAsString (const csString& property, const char* sub)
{
  wxString wxValue = detailGrid->GetPropertyValueAsString (wxString::FromUTF8 (
	property + "." + sub));
  csString value = (const char*)wxValue.mb_str (wxConvUTF8);
  return value;
}

int EntityMode::GetPropertyValueAsInt (const csString& property, const char* sub)
{
  int value = detailGrid->GetPropertyValueAsInt (wxString::FromUTF8 (
	property + "." + sub));
  return value;
}

bool EntityMode::ValidateGridChange (iCelPropertyClassTemplate* pctpl,
    const csString& pcPropName, const csString& selectedPropName, const csString& value)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (selectedPropName == "Tag")
  {
    iCelPropertyClassTemplate* pc = tpl->FindPropertyClassTemplate (pctpl->GetName (), value);
    if (pc && pc != pctpl)
    {
      iUIManager* ui = view3d->GetApplication ()->GetUI ();
      ui->Error ("Property class with this name and tag already exists!");
      return false;
    }
  }

  csString type = GetPropertyValueAsString (pcPropName, "Type");
  PcEditorSupport* editor = editors.Get (pctpl->GetName (), 0);
  if (editor)
    return editor->Validate (pctpl, pcPropName, selectedPropName, value);

  return true;
}

bool EntityMode::UpdatePCFromGrid (iCelPropertyClassTemplate* pctpl, const csString& propname,
    const csString& selectedPropName)
{
  csString tag = GetPropertyValueAsString (propname, "Tag");
  if (tag != pctpl->GetTag ())
  {
    if (tag.IsEmpty ())
      pctpl->SetTag (0);
    else
      pctpl->SetTag (tag);
    return true;
  }

  csString type = GetPropertyValueAsString (propname, "Type");
  if (type != pctpl->GetName ())
  {
    pctpl->SetName (type);
    pctpl->RemoveAllProperties ();
    return true;
  }

  PcEditorSupport* editor = editors.Get (type, 0);
  if (editor)
    return editor->Update (pctpl, propname, selectedPropName);

  return false;
}

void EntityMode::OnDeleteCharacteristic ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);
  size_t dot = selectedPropName.FindFirst ('.');
  csString name;
  if (dot == csArrayItemNotFound)
    name = selectedPropName.Slice (5);
  else
    name = selectedPropName.Slice (5, dot-5);
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  tpl->GetCharacteristics ()->ClearCharacteristic (name);
  PCWasEdited (0);
  SelectTemplate (tpl);
}

void EntityMode::OnNewCharacteristic ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Characteristic Parameter");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("Name");
  dialog->AddRow ();
  dialog->AddText ("Value");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString name = result.Get ("Name", (const char*)0);
  csString value = result.Get ("Value", (const char*)0);

  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (name.IsEmpty ())
  {
    ui->Error ("Empty name is not allowed!");
    return;
  }
  else if (tpl->GetCharacteristics ()->HasCharacteristic (name))
  {
    ui->Error ("There is already a characteristic with this name!");
    return;
  }
  float v;
  csScanStr (value, "%f", &v);
  tpl->GetCharacteristics ()->SetCharacteristic (name, v);
  PCWasEdited (0);
  SelectTemplate (tpl);
}

void EntityMode::PcProp_OnDelProperty ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);
  size_t dot = selectedPropName.FindFirst ('.');
  csString name;
  if (dot == csArrayItemNotFound)
    name = selectedPropName.Slice (5);
  else
    name = selectedPropName.Slice (5, dot-5);
  pctpl->RemoveProperty (pl->FetchStringID (name));
  PCWasEdited (pctpl);
}

void EntityMode::PcProp_OnNewProperty ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Quest Parameter");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("Name");
  dialog->AddChoice ("Type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", (const char*)0);
  dialog->AddRow ();
  dialog->AddMultiText ("Value");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString name = result.Get ("Name", (const char*)0);
  csString value = result.Get ("Value", (const char*)0);
  csString typeS = result.Get ("Type", (const char*)0);

  if (name.IsEmpty ())
  {
    ui->Error ("Empty name is not allowed!");
    return;
  }
  else if (pctpl->FindProperty (pl->FetchStringID (name)) != csArrayItemNotFound)
  {
    ui->Error ("There is already a parameter with this name!");
    return;
  }

  celDataType type = InspectTools::StringToType (typeS);
  InspectTools::SetProperty (pl, pctpl, type, name, value);
  PCWasEdited (pctpl);
}

void EntityMode::PcQuest_OnDelParameter ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);
  size_t dot = selectedPropName.FindFirst ('.');
  csString name;
  if (dot == csArrayItemNotFound)
    name = selectedPropName.Slice (4);
  else
    name = selectedPropName.Slice (4, dot-4);
  InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", name);
  PCWasEdited (pctpl);
}

void EntityMode::PcQuest_OnNewParameter ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Quest Parameter");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("Name");
  dialog->AddChoice ("Type", "string", "float", "long", "bool",
      "vector2", "vector3", "color", (const char*)0);
  dialog->AddRow ();
  dialog->AddMultiText ("Value");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString name = result.Get ("Name", (const char*)0);
  csString value = result.Get ("Value", (const char*)0);
  csString typeS = result.Get ("Type", (const char*)0);

  if (name.IsEmpty ())
  {
    ui->Error ("Empty name is not allowed!");
    return;
  }
  else if (InspectTools::GetActionParameterValue (pl, pctpl, "NewQuest", name))
  {
    ui->Error ("There is already a parameter with this name!");
    return;
  }

  celDataType type = InspectTools::StringToType (typeS);
  csRef<iParameter> par = GetPM ()->GetParameter (value, type);
  InspectTools::AddActionParameter (pl, pctpl, "NewQuest", name, par);
  PCWasEdited (pctpl);
}

// ------------------------------------------------------------------------

void EntityMode::BuildMainPanel (wxWindow* parent)
{
  if (panel)
  {
    view.Reset ();
    parent->GetSizer ()->Detach (panel);
    delete parent;
  }
  panel = new Panel (this);
  view.SetParent (panel);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("EntityModePanel"));

  wxPanel* childPanel = XRCCTRL (*panel, "childPanel", wxPanel);

  pcPanel = new PropertyClassPanel (childPanel, view3d->GetApplication ()->GetUI (), this);
  pcPanel->Hide ();

  triggerPanel = new TriggerPanel (childPanel, view3d->GetApplication ()->GetUI (), this);
  triggerPanel->Hide ();

  rewardPanel = new RewardPanel (childPanel, view3d->GetApplication ()->GetUI (), this);
  rewardPanel->Hide ();

  sequencePanel = new SequencePanel (childPanel, view3d->GetApplication ()->GetUI (), this);
  sequencePanel->Hide ();

  tplPanel = new EntityTemplatePanel (childPanel, view3d->GetApplication ()->GetUI (), this);
  tplPanel->Hide ();

  BuildDetailGrid ();

  graphView = markerMgr->CreateGraphView ();
  graphView->Clear ();
  csRef<GraphNodeCallback> cb;
  cb.AttachNew (new GraphNodeCallback (this));
  graphView->AddNodeActivationCallback (cb);

  graphView->SetVisible (false);

  view.DefineHeadingIndexed ("template_List", "Template,M",
      TEMPLATE_COL_NAME, TEMPLATE_COL_MODIFIED);
  view.Bind (view3d->GetModelRepository ()->GetTemplatesValue (), "template_List");
  wxListCtrl* list = XRCCTRL (*panel, "template_List", wxListCtrl);
  view.AddAction (list, NEWREF(Ares::Action, new AddTemplateAction (this)));
  view.AddAction (list, NEWREF(Ares::Action, new DeleteTemplateAction (this)));
  view.AddAction (list, NEWREF(Ares::Action, new RenameTemplateAction (this)));

  view.DefineHeadingIndexed ("quest_List", "Quest,M", QUEST_COL_NAME, QUEST_COL_MODIFIED);
  questsValue = view3d->GetModelRepository ()->GetQuestsValue ();
  view.Bind (questsValue, "quest_List");
  list = XRCCTRL (*panel, "quest_List", wxListCtrl);
  view.AddAction (list, NEWREF(Ares::Action, new AddQuestAction (this)));
  view.AddAction (list, NEWREF(Ares::Action, new DeleteQuestAction (this)));
  view.AddAction (list, NEWREF(Ares::Action, new RenameQuestAction (this)));

  InitColors ();
  editQuestMode = 0;
}

EntityMode::~EntityMode ()
{
  markerMgr->DestroyGraphView (graphView);
}

iParameterManager* EntityMode::GetPM () const
{
  return view3d->GetPM ();
}

iAresEditor* EntityMode::GetApplication () const
{
  return view3d->GetApplication ();
}

iMarkerColor* EntityMode::NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1, bool fill)
{
  iMarkerColor* col = markerMgr->CreateMarkerColor (name);
  col->SetRGBColor (SELECTION_NONE, r0, g0, b0, 1);
  col->SetRGBColor (SELECTION_SELECTED, r1, g1, b1, 1);
  col->SetRGBColor (SELECTION_ACTIVE, r1, g1, b1, 1);
  col->SetPenWidth (SELECTION_NONE, 1.2f);
  col->SetPenWidth (SELECTION_SELECTED, 1.2f);
  col->SetPenWidth (SELECTION_ACTIVE, 1.2f);
  col->EnableFill (SELECTION_NONE, fill);
  col->EnableFill (SELECTION_SELECTED, fill);
  col->EnableFill (SELECTION_ACTIVE, fill);
  return col;
}

iMarkerColor* EntityMode::NewColor (const char* name,
    float r0, float g0, float b0, float r1, float g1, float b1,
    float r2, float g2, float b2, bool fill)
{
  iMarkerColor* col = markerMgr->CreateMarkerColor (name);
  col->SetRGBColor (SELECTION_NONE, r0, g0, b0, 1);
  col->SetRGBColor (SELECTION_SELECTED, r1, g1, b1, 1);
  col->SetRGBColor (SELECTION_ACTIVE, r2, g2, b2, 1);
  col->SetPenWidth (SELECTION_NONE, 1.2f);
  col->SetPenWidth (SELECTION_SELECTED, 1.2f);
  col->SetPenWidth (SELECTION_ACTIVE, 1.2f);
  col->EnableFill (SELECTION_NONE, fill);
  col->EnableFill (SELECTION_SELECTED, fill);
  col->EnableFill (SELECTION_ACTIVE, fill);
  return col;
}

void EntityMode::InitColors ()
{
  iMarkerColor* textColor = NewColor ("viewWhite", .8, .8, .8, 1, 1, 1, false);
  iMarkerColor* textSelColor = NewColor ("viewBlack", 0, 0, 0, 0, 0, 0, false);

  iMarkerColor* thickLinkColor = markerMgr->CreateMarkerColor ("thickLink");
  thickLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thickLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thickLinkColor->SetPenWidth (SELECTION_NONE, 1.2f);
  thickLinkColor->SetPenWidth (SELECTION_SELECTED, 2.0f);
  thickLinkColor->SetPenWidth (SELECTION_ACTIVE, 2.0f);
  iMarkerColor* thinLinkColor = markerMgr->CreateMarkerColor ("thinLink");
  thinLinkColor->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  thinLinkColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  thinLinkColor->SetPenWidth (SELECTION_NONE, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_SELECTED, 0.8f);
  thinLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.8f);
  iMarkerColor* arrowLinkColor = markerMgr->CreateMarkerColor ("arrowLink");
  arrowLinkColor->SetRGBColor (SELECTION_NONE, .6, .6, .6, .5);
  arrowLinkColor->SetRGBColor (SELECTION_SELECTED, .7, .7, .7, .5);
  arrowLinkColor->SetRGBColor (SELECTION_ACTIVE, .7, .7, .7, .5);
  arrowLinkColor->SetPenWidth (SELECTION_NONE, 0.3f);
  arrowLinkColor->SetPenWidth (SELECTION_SELECTED, 0.3f);
  arrowLinkColor->SetPenWidth (SELECTION_ACTIVE, 0.3f);

  styleTemplate = markerMgr->CreateGraphNodeStyle ();
  styleTemplate->SetBorderColor (NewColor ("templateColorFG", .0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleTemplate->SetBackgroundColor (NewColor ("templateColorBG", .1, .4, .5, .2, .6, .7, true));
  styleTemplate->SetTextColor (textColor);
  styleTemplate->SetTextFont (fontLarge);

  stylePC = markerMgr->CreateGraphNodeStyle ();
  stylePC->SetBorderColor (NewColor ("pcColorFG", 0, 0, .7, 0, 0, 1, 1, 1, 1, false));
  stylePC->SetBackgroundColor (NewColor ("pcColorBG", .1, .4, .5, .2, .6, .7, true));
  stylePC->SetTextColor (textColor);
  stylePC->SetTextFont (font);

  iMarkerColor* colStateFG = NewColor ("stateColorFG", 0, .7, 0, 0, 1, 0, 1, 1, 1, false);
  iMarkerColor* colStateBG = NewColor ("stateColorBG", .1, .4, .5, .2, .6, .7, true);
  styleState = markerMgr->CreateGraphNodeStyle ();
  styleState->SetBorderColor (colStateFG);
  styleState->SetBackgroundColor (colStateBG);
  styleState->SetTextColor (textColor);
  styleState->SetTextFont (font);
  styleStateDefault = markerMgr->CreateGraphNodeStyle ();
  styleStateDefault->SetBorderColor (colStateFG);
  styleStateDefault->SetBackgroundColor (colStateBG);
  styleStateDefault->SetTextColor (textSelColor);
  styleStateDefault->SetTextFont (fontBold);

  iMarkerColor* colSeqFG = NewColor ("seqColorFG", 0, 0, 0, 0, 0, 0, 1, 1, 1, false);
  iMarkerColor* colSeqBG = NewColor ("seqColorBG", .8, 0, 0, 1, 0, 0, true);
  styleSequence = markerMgr->CreateGraphNodeStyle ();
  styleSequence->SetBorderColor (colSeqFG);
  styleSequence->SetBackgroundColor (colSeqBG);
  styleSequence->SetTextColor (textColor);
  styleSequence->SetTextFont (font);

  styleResponse = markerMgr->CreateGraphNodeStyle ();
  styleResponse->SetBorderColor (NewColor ("respColorFG", 0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleResponse->SetBackgroundColor (NewColor ("respColorBG", .3, .6, .7, .4, .7, .8, true));
  styleResponse->SetRoundness (5);
  styleResponse->SetTextColor (NewColor ("respColorTxt", 0, 0, 0, 0, 0, 0, false));
  styleResponse->SetTextFont (font);

  styleReward = markerMgr->CreateGraphNodeStyle ();
  styleReward->SetBorderColor (NewColor ("rewColorFG", 0, .7, .7, 0, 1, 1, 1, 1, 1, false));
  styleReward->SetBackgroundColor (NewColor ("rewColorBG", .3, .6, .7, .4, .7, .8, true));
  styleReward->SetRoundness (1);
  styleReward->SetTextColor (textColor);
  styleReward->SetTextFont (font);
  styleReward->SetConnectorStyle (CONNECTOR_RIGHT);

  graphView->SetDefaultNodeStyle (stylePC);

  styleThickLink = markerMgr->CreateGraphLinkStyle ();
  styleThickLink->SetColor (thickLinkColor);
  styleThinLink = markerMgr->CreateGraphLinkStyle ();
  styleThinLink->SetColor (thinLinkColor);
  styleArrowLink = markerMgr->CreateGraphLinkStyle ();
  styleArrowLink->SetColor (arrowLinkColor);
  styleArrowLink->SetArrow (true);
  styleArrowLink->SetSoft (true);
  styleArrowLink->SetLinkStrength (0.0);

  graphView->SetDefaultLinkStyle (styleThickLink);
}

void EntityMode::Start ()
{
  EditingMode::Start ();
  view3d->GetApplication ()->HideCameraWindow ();
  view3d->GetModelRepository ()->GetTemplatesValue ()->Refresh ();
  graphView->SetVisible (true);
  pcPanel->Hide ();
  triggerPanel->Hide ();
  rewardPanel->Hide ();
  sequencePanel->Hide ();
  tplPanel->Hide ();
  contextMenuNode = "";
  questsValue->Refresh ();
}

void EntityMode::Stop ()
{
  EditingMode::Stop ();
  graphView->SetVisible (false);
}

const char* EntityMode::GetTriggerType (iTriggerFactory* trigger)
{
  {
    csRef<iTimeoutTriggerFactory> s = scfQueryInterface<iTimeoutTriggerFactory> (trigger);
    if (s) return "Timeout";
  }
  {
    csRef<iEnterSectorTriggerFactory> s = scfQueryInterface<iEnterSectorTriggerFactory> (trigger);
    if (s) return "EnterSect";
  }
  {
    csRef<iSequenceFinishTriggerFactory> s = scfQueryInterface<iSequenceFinishTriggerFactory> (trigger);
    if (s) return "SeqFinish";
  }
  {
    csRef<iPropertyChangeTriggerFactory> s = scfQueryInterface<iPropertyChangeTriggerFactory> (trigger);
    if (s) return "PropChange";
  }
  {
    csRef<iTriggerTriggerFactory> s = scfQueryInterface<iTriggerTriggerFactory> (trigger);
    if (s) return "Trigger";
  }
  {
    csRef<iWatchTriggerFactory> s = scfQueryInterface<iWatchTriggerFactory> (trigger);
    if (s) return "Watch";
  }
  {
    csRef<iOperationTriggerFactory> s = scfQueryInterface<iOperationTriggerFactory> (trigger);
    if (s) return "Operation";
  }
  {
    csRef<iInventoryTriggerFactory> s = scfQueryInterface<iInventoryTriggerFactory> (trigger);
    if (s) return "Inventory";
  }
  {
    csRef<iMessageTriggerFactory> s = scfQueryInterface<iMessageTriggerFactory> (trigger);
    if (s) return "Message";
  }
  {
    csRef<iMeshSelectTriggerFactory> s = scfQueryInterface<iMeshSelectTriggerFactory> (trigger);
    if (s) return "MeshSel";
  }
  return "?";
}

const char* EntityMode::GetRewardType (iRewardFactory* reward)
{
  {
    csRef<iNewStateQuestRewardFactory> s = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    if (s) return "NewState";
  }
  {
    csRef<iDebugPrintRewardFactory> s = scfQueryInterface<iDebugPrintRewardFactory> (reward);
    if (s) return "DbPrint";
  }
  {
    csRef<iInventoryRewardFactory> s = scfQueryInterface<iInventoryRewardFactory> (reward);
    if (s) return "Inventory";
  }
  {
    csRef<iSequenceRewardFactory> s = scfQueryInterface<iSequenceRewardFactory> (reward);
    if (s) return "Sequence";
  }
  {
    csRef<iCsSequenceRewardFactory> s = scfQueryInterface<iCsSequenceRewardFactory> (reward);
    if (s) return "CsSequence";
  }
  {
    csRef<iSequenceFinishRewardFactory> s = scfQueryInterface<iSequenceFinishRewardFactory> (reward);
    if (s) return "SeqFinish";
  }
  {
    csRef<iChangePropertyRewardFactory> s = scfQueryInterface<iChangePropertyRewardFactory> (reward);
    if (s) return "ChangeProp";
  }
  {
    csRef<iCreateEntityRewardFactory> s = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    if (s) return "CreateEnt";
  }
  {
    csRef<iDestroyEntityRewardFactory> s = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
    if (s) return "DestroyEnt";
  }
  {
    csRef<iChangeClassRewardFactory> s = scfQueryInterface<iChangeClassRewardFactory> (reward);
    if (s) return "ChangeClass";
  }
  {
    csRef<iActionRewardFactory> s = scfQueryInterface<iActionRewardFactory> (reward);
    if (s) return "Action";
  }
  {
    csRef<iMessageRewardFactory> s = scfQueryInterface<iMessageRewardFactory> (reward);
    if (s) return "Message";
  }
  return "?";
}

void EntityMode::BuildRewardGraph (iRewardFactoryArray* rewards,
    const char* parentKey, const char* pcKey)
{
  for (size_t j = 0 ; j < rewards->GetSize () ; j++)
  {
    iRewardFactory* reward = rewards->Get (j);
    csString rewKey; rewKey.Format ("r:%zu,%s", j, parentKey);
    csString rewLabel; rewLabel.Format ("%zu:%s", j+1, GetRewardType (reward));
    graphView->CreateSubNode (parentKey, rewKey, rewLabel, styleReward);
    //graphView->LinkNode (parentKey, rewKey, styleThinLink);

    csRef<iNewStateQuestRewardFactory> newState = scfQueryInterface<iNewStateQuestRewardFactory> (reward);
    if (newState)
    {
      csString entity = newState->GetEntityParameter ();
      csString cls = newState->GetClassParameter ();
      if (entity.IsEmpty () && cls.IsEmpty ())
      {
        // @@@ No support for expressions here!
        csString stateKey; stateKey.Format ("S:%s,%s", newState->GetStateParameter (), pcKey);
        graphView->LinkNode (rewKey, stateKey, styleArrowLink);
      }
    }
  }
}

csString EntityMode::GetRewardsLabel (iRewardFactoryArray* rewards)
{
  csString label;
  for (size_t j = 0 ; j < rewards->GetSize () ; j++)
  {
    label += csString ("\n    ");// + GetRewardType (reward);
  }
  return label;
}

void EntityMode::BuildStateGraph (iQuestStateFactory* state,
    const char* stateKey, const char* pcKey)
{
  csRef<iQuestTriggerResponseFactoryArray> responses = state->GetTriggerResponseFactories ();
  for (size_t i = 0 ; i < responses->GetSize () ; i++)
  {
    iQuestTriggerResponseFactory* response = responses->Get (i);
    csString responseKey; responseKey.Format ("t:%zu,%s", i, stateKey);
    csString triggerLabel = GetTriggerType (response->GetTriggerFactory ());
    triggerLabel += GetRewardsLabel (response->GetRewardFactories ());
    graphView->CreateNode (responseKey, triggerLabel, styleResponse);
    graphView->LinkNode (stateKey, responseKey);
    csRef<iRewardFactoryArray> rewards = response->GetRewardFactories ();
    BuildRewardGraph (rewards, responseKey, pcKey);
  }

  csRef<iRewardFactoryArray> initRewards = state->GetInitRewardFactories ();
  if (initRewards->GetSize () > 0)
  {
    csString newKeyKey; newKeyKey.Format ("i:,%s", stateKey);
    csString label = "Oninit:";
    label += GetRewardsLabel (initRewards);
    graphView->CreateNode (newKeyKey, label, styleResponse);
    graphView->LinkNode (stateKey, newKeyKey);
    BuildRewardGraph (initRewards, newKeyKey, pcKey);
  }
  csRef<iRewardFactoryArray> exitRewards = state->GetExitRewardFactories ();
  if (exitRewards->GetSize () > 0)
  {
    csString newKeyKey; newKeyKey.Format ("e:,%s", stateKey);
    csString label = "Onexit:";
    label += GetRewardsLabel (exitRewards);
    graphView->CreateNode (newKeyKey, label, styleResponse);
    graphView->LinkNode (stateKey, newKeyKey);
    BuildRewardGraph (exitRewards, newKeyKey, pcKey);
  }
}

csString EntityMode::GetQuestName (iCelPropertyClassTemplate* pctpl)
{
  if (editQuestMode)
    return editQuestMode->GetName ();
  else
    return InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
}

void EntityMode::BuildQuestGraph (iQuestFactory* questFact, const char* pcKey,
    bool fullquest, const csString& defaultState)
{
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    csString stateKey; stateKey.Format ("S:%s,%s", stateFact->GetName (), pcKey);
    graphView->CreateNode (stateKey, stateFact->GetName (),
	defaultState == stateFact->GetName () ? styleStateDefault : styleState);
    graphView->LinkNode (pcKey, stateKey);
    if (fullquest)
      BuildStateGraph (stateFact, stateKey, pcKey);
  }
  csRef<iCelSequenceFactoryIterator> seqIt = questFact->GetSequences ();
  while (seqIt->HasNext ())
  {
    iCelSequenceFactory* seqFact = seqIt->Next ();
    csString seqKey; seqKey.Format ("s:%s,%s", seqFact->GetName (), pcKey);
    graphView->CreateNode (seqKey, seqFact->GetName (), styleSequence);
    graphView->LinkNode (pcKey, seqKey);
  }
}

void EntityMode::BuildQuestGraph (iCelPropertyClassTemplate* pctpl,
    const char* pcKey)
{
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  csString defaultState = InspectTools::GetPropertyValueString (pl, pctpl, "state");

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  // @@@ Error check
  if (questFact) BuildQuestGraph (questFact, pcKey, false, defaultState);
}

csString EntityMode::GetExtraPCInfo (iCelPropertyClassTemplate* pctpl)
{
  csString pcName = pctpl->GetName ();
  if (pcName == "pclogic.quest")
  {
    return GetQuestName (pctpl);
  }
  return "";
}

void EntityMode::GetPCKeyLabel (iCelPropertyClassTemplate* pctpl, csString& pcKey, csString& pcLabel)
{
  csString pcShortName;
  csString pcName = pctpl->GetName ();
  size_t lastDot = pcName.FindLast ('.');
  if (lastDot != csArrayItemNotFound)
    pcShortName = pcName.Slice (lastDot+1);

  pcKey.Format ("P:%s", pcName.GetData ());
  pcLabel = pcShortName;
  if (pctpl->GetTag () != 0)
  {
    pcKey.AppendFmt (":%s", pctpl->GetTag ());
    pcLabel.AppendFmt (" (%s)", pctpl->GetTag ());
  }

  csString extraInfo = GetExtraPCInfo (pctpl);
  if (!extraInfo.IsEmpty ()) { pcLabel += '\n'; pcLabel += extraInfo; }
}

void EntityMode::BuildTemplateGraph (const char* templateName)
{
  currentTemplate = templateName;

  graphView->StartRefresh ();

  graphView->SetVisible (false);
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (templateName);
  if (!tpl) { graphView->FinishRefresh (); return; }

  csString tplKey; tplKey.Format ("T:%s", templateName);
  graphView->CreateNode (tplKey, templateName, styleTemplate);

  for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);

    // Extract the last part of the name (everything after the last '.').
    csString pcKey, pcLabel;
    GetPCKeyLabel (pctpl, pcKey, pcLabel);
    graphView->CreateNode (pcKey, pcLabel, stylePC);
    graphView->LinkNode (tplKey, pcKey);
    csString pcName = pctpl->GetName ();
    if (pcName == "pclogic.quest")
      BuildQuestGraph (pctpl, pcKey);
  }
  graphView->FinishRefresh ();
  graphView->SetVisible (true);
}

void EntityMode::Refresh ()
{
  questsValue->Refresh ();
  ActivateNode (0);
  RefreshView ();
}

void EntityMode::RefreshView (iCelPropertyClassTemplate* pctpl)
{
  if (!started) return;
  if (editQuestMode)
  {
    graphView->StartRefresh ();
    graphView->SetVisible (false);

    csString pcKey = "P:pclogic.quest";
    csString pcLabel = "quest\n";
    pcLabel += editQuestMode->GetName ();
    graphView->CreateNode (pcKey, pcLabel, stylePC);

    csString defaultState;	// Empty: we have no default state here.
    BuildQuestGraph (editQuestMode, pcKey, true, defaultState);
    graphView->FinishRefresh ();
    graphView->SetVisible (true);
    app->SetObjectForComment ("quest", editQuestMode->QueryObject ());
    FillDetailGrid (0);
  }
  else
  {
    BuildTemplateGraph (currentTemplate);
    if (pctpl) SelectPC (pctpl);
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    FillDetailGrid (tpl);
  }
}

void EntityMode::SelectPC (iCelPropertyClassTemplate* pctpl)
{
  csString pcKey, pcLabel;
  GetPCKeyLabel (pctpl, pcKey, pcLabel);
  //if (pcKey != activeNode)
    graphView->ActivateNode (pcKey);
}

iQuestFactory* EntityMode::GetSelectedQuest (const char* key)
{
  if (editQuestMode) return editQuestMode;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (key);
  csString questName = GetQuestName (pctpl);
  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  return questFact;
}

bool EntityMode::IsOnInit (const char* key)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'i') return true;
  }
  return 0;
}

bool EntityMode::IsOnExit (const char* key)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'e') return true;
  }
  return 0;
}

iCelSequenceFactory* EntityMode::GetSelectedSequence (const char* key)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 's')
    {
      csStringArray tokens (op, ":");
      csString sequenceName = tokens[1];
      iQuestFactory* questFact = GetSelectedQuest (key);
      return questFact->GetSequence (sequenceName);
    }
  }
  return 0;
}

csRef<iRewardFactoryArray> EntityMode::GetSelectedReward (const char* key,
    size_t& idx)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'r')
    {
      csRef<iRewardFactoryArray> array;
      csStringArray tokens (op, ":");
      csString triggerNum = tokens[1];
      int index;
      csScanStr (triggerNum, "%d", &index);
      idx = index;
      if (IsOnInit (key))
      {
        iQuestStateFactory* state = GetSelectedState (key);
	return state->GetInitRewardFactories ();
      }
      else if (IsOnExit (key))
      {
        iQuestStateFactory* state = GetSelectedState (key);
	return state->GetExitRewardFactories ();
      }
      else
      {
        iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (key);
        if (!resp) return 0;
        return resp->GetRewardFactories ();
      }
    }
  }
  return 0;
}

iQuestTriggerResponseFactory* EntityMode::GetSelectedTriggerResponse (const char* key)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 't')
    {
      csStringArray tokens (op, ":");
      csString triggerNum = tokens[1];
      int num;
      csScanStr (triggerNum, "%d", &num);
      iQuestStateFactory* state = GetSelectedState (key);
      if (!state) return 0;
      return state->GetTriggerResponseFactories ()->Get (num);
    }
  }
  return 0;
}

csString EntityMode::GetSelectedStateName (const char* key)
{
  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'S')
    {
      csStringArray tokens (op, ":");
      csString stateName = tokens[1];
      return stateName;
    }
  }
  return "";
}

iQuestStateFactory* EntityMode::GetSelectedState (const char* key)
{
  csString n = GetSelectedStateName (key);
  if (!n) return 0;
  iQuestFactory* questFact = GetSelectedQuest (key);
  if (!questFact) return 0;
  return questFact->GetState (n);
}


iCelPropertyClassTemplate* EntityMode::GetPCTemplate (const char* key)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  if (!tpl) return 0;

  csStringArray ops (key, ",");
  for (size_t i = 0 ; i < ops.GetSize () ; i++)
  {
    csString op = ops.Get (i);
    if (op.operator[] (0) == 'P')
    {
      csStringArray tokens (op, ":");
      csString pcName = tokens[1];
      csString tagName;
      if (tokens.GetSize () >= 3) tagName = tokens[2];
      return tpl->FindPropertyClassTemplate (pcName, tagName);
    }
  }
  return 0;
}

void EntityMode::OnQuestSelect ()
{
  if (!started) return;
  wxListCtrl* list = XRCCTRL (*panel, "template_List", wxListCtrl);
  ListCtrlTools::ClearSelection (list);
  list = XRCCTRL (*panel, "quest_List", wxListCtrl);
  Ares::Value* v = view.GetSelectedValue (list);
  if (!v) return;
  csString questName = v->GetStringArrayValue ()->Get (QUEST_COL_NAME);
  editQuestMode = questMgr->GetQuestFactory (questName);
  currentTemplate = "";
  RefreshView ();
}

void EntityMode::OnTemplateSelect ()
{
  if (!started) return;
  wxListCtrl* list = XRCCTRL (*panel, "quest_List", wxListCtrl);
  ListCtrlTools::ClearSelection (list);
  list = XRCCTRL (*panel, "template_List", wxListCtrl);
  Ares::Value* v = view.GetSelectedValue (list);
  if (!v) return;
  csString templateName = v->GetStringArrayValue ()->Get (TEMPLATE_COL_NAME);
  if (editQuestMode || currentTemplate != templateName)
  {
    editQuestMode = 0;
    currentTemplate = templateName;
    RefreshView ();
    ActivateNode (0);
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    app->SetObjectForComment ("template", tpl->QueryObject ());
  }
}

void EntityMode::RegisterModification (iCelEntityTemplate* tpl)
{
  if (!tpl)
    tpl = pl->FindEntityTemplate (currentTemplate);
  view3d->GetApplication ()->RegisterModification (tpl->QueryObject ());
  view3d->GetModelRepository ()->GetTemplatesValue ()->Refresh ();
  RefreshView ();
}

void EntityMode::RegisterModification (iQuestFactory* quest)
{
  GetApplication ()->RegisterModification (quest->QueryObject ());
  questsValue->Refresh ();
}

static size_t FindNotebookPage (wxNotebook* notebook, const char* name)
{
  wxString iname = wxString::FromUTF8 (name);
  for (size_t i = 0 ; i < notebook->GetPageCount () ; i++)
  {
    wxString pageName = notebook->GetPageText (i);
    if (pageName == iname) return i;
  }
  return csArrayItemNotFound;
}

void EntityMode::SelectResource (iObject* resource)
{
  csRef<iCelEntityTemplate> tpl = scfQueryInterface<iCelEntityTemplate> (resource);
  if (tpl)
  {
    wxNotebook* notebook = XRCCTRL (*panel, "type_Notebook", wxNotebook);
    size_t pageIdx = FindNotebookPage (notebook, "Templates");
    if (pageIdx != csArrayItemNotFound)
      notebook->ChangeSelection (pageIdx);
    SelectTemplate (tpl);
    return;
  }
  csRef<iQuestFactory> quest = scfQueryInterface<iQuestFactory> (resource);
  if (quest)
  {
    wxNotebook* notebook = XRCCTRL (*panel, "type_Notebook", wxNotebook);
    size_t pageIdx = FindNotebookPage (notebook, "Quests");
    if (pageIdx != csArrayItemNotFound)
      notebook->ChangeSelection (pageIdx);
    SelectQuest (quest);
    return;
  }
}

void EntityMode::SelectQuest (iQuestFactory* questFact)
{
  currentTemplate = "";
  editQuestMode = questFact;
  BuildTemplateGraph (currentTemplate);
  questsValue->Refresh ();
  csRef<Ares::ValueIterator> it = questsValue->GetIterator ();
  size_t i = 0;
  while (it->HasNext ())
  {
    Ares::Value* c = it->NextChild ();
    csString questName = c->GetStringArrayValue ()->Get (QUEST_COL_NAME);
    if (questName == questFact->GetName ()) break;
    i++;
  }
  wxListCtrl* list = XRCCTRL (*panel, "quest_List", wxListCtrl);
  ListCtrlTools::SelectRow (list, (int)i, false);
  ActivateNode (0);
  app->SetObjectForComment ("quest", questFact->QueryObject ());
  RefreshView ();
}

void EntityMode::SelectTemplate (iCelEntityTemplate* tpl)
{
  currentTemplate = tpl->GetName ();
  editQuestMode = 0;
  BuildTemplateGraph (currentTemplate);
  view3d->GetModelRepository ()->GetTemplatesValue ()->Refresh ();
  size_t i = view3d->GetModelRepository ()->GetTemplateIndexFromTemplates (tpl);
  wxListCtrl* list = XRCCTRL (*panel, "template_List", wxListCtrl);
  ListCtrlTools::SelectRow (list, (int)i, false);
  ActivateNode (0);
  app->SetObjectForComment ("template", tpl->QueryObject ());
}

void EntityMode::AskNewQuest ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New Quest", "Name:");
  if (name && !name->IsEmpty ())
  {
    iQuestFactory* questFact = questMgr->GetQuestFactory (name->GetData ());
    if (questFact)
      ui->Error ("A quest with this name already exists!");
    else
    {
      questFact = questMgr->CreateQuestFactory (name->GetData ());
      RegisterModification (questFact);
      SelectQuest (questFact);
    }
  }
}

void EntityMode::OnRenameTemplate (const char* tplName)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (tplName);
  if (!tpl) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New template name", "New name:", tplName);
  if (name && !name->IsEmpty ())
  {
    if (csString (tplName) == name->GetData ())
      return;
    if (pl->FindEntityTemplate (name->GetData ()))
    {
      ui->Error ("A template with this name already exists!");
      return;
    }
    tpl->QueryObject ()->SetName (name->GetData ());
    view3d->GetApplication ()->RegisterModification (tpl->QueryObject ());
    view3d->GetModelRepository ()->GetTemplatesValue ()->Refresh ();
    view3d->GetModelRepository ()->GetObjectsValue ()->Refresh ();
    SelectTemplate (tpl);
    RefreshView ();
  }
}

void EntityMode::OnRenameQuest (const char* questName)
{
  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New quest name", "New name:", questName);
  if (name && !name->IsEmpty ())
  {
    if (csString (questName) == name->GetData ())
      return;
    if (questMgr->GetQuestFactory (name->GetData ()))
    {
      ui->Error ("A quest with this name already exists!");
      return;
    }
    questFact->QueryObject ()->SetName (name->GetData ());
    RegisterModification (questFact);

    csString pcLogicQuest = "pclogic.quest";
    csRef<iCelEntityTemplateIterator> tplIt = pl->GetEntityTemplates ();
    while (tplIt->HasNext ())
    {
      iCelEntityTemplate* tpl = tplIt->Next ();
      for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
      {
        iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
        if (pcLogicQuest == pctpl->GetName ())
        {
	  csString n = InspectTools::GetActionParameterValueString (pl, pctpl, "NewQuest", "name");
	  if (n == questName)
	  {
	    csRef<iParameter> par = view3d->GetPM ()->GetParameter (name->GetData (), CEL_DATA_STRING);
	    InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", "name");
	    InspectTools::AddActionParameter (pl, pctpl, "NewQuest", "name", par);
	    RegisterModification (tpl);
	  }
        }
      }
    }

    questsValue->Refresh ();
    SelectQuest (questFact);
    RefreshView ();
  }
}

void EntityMode::OnQuestDel (const char* questName)
{
  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csString pcLogicQuest = "pclogic.quest";

  int cnt = 0;
  csRef<iCelEntityTemplateIterator> tplIt = pl->GetEntityTemplates ();
  while (tplIt->HasNext ())
  {
    iCelEntityTemplate* tpl = tplIt->Next ();
    for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
    {
      iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (i);
      if (pcLogicQuest == pctpl->GetName ())
      {
	csString n = InspectTools::GetActionParameterValueString (pl, pctpl, "NewQuest", "name");
	if (n == questName) cnt++;
      }
    }
  }

  bool yes;
  if (cnt > 0)
    yes = ui->Ask ("There are %d usages of this quest. Are you sure you want to remove the '%s' quest?",
	cnt, questName);
  else
    yes = ui->Ask ("Are you sure you want to remove the '%s' quest?", questName);
  if (yes)
  {
    view3d->GetApplication ()->GetAssetManager ()->RegisterRemoval (questFact->QueryObject ());
    questMgr->RemoveQuestFactory (questName);
    questsValue->Refresh ();
    editQuestMode = 0;
    ActivateNode (0);
    RefreshView ();
  }
}

void EntityMode::AskNewTemplate ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New Template", "Name:");
  if (name && !name->IsEmpty ())
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (name->GetData ());
    if (tpl)
      ui->Error ("A template with this name already exists!");
    else
    {
      tpl = pl->CreateEntityTemplate (name->GetData ());
      RegisterModification (tpl);
      SelectTemplate (tpl);
    }
  }
}

void EntityMode::OnTemplateDel (const char* tplName)
{
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (tplName);
  if (!tpl) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  iPcDynamicWorld* dynworld = view3d->GetDynamicWorld ();

  // Count the usages.
  int cntDynobj = 0, cntParent = 0, cntFact = 0;
  csRef<iDynamicCellIterator> cellIt = dynworld->GetCells ();
  while (cellIt->HasNext ())
  {
    iDynamicCell* cell = cellIt->NextCell ();
    for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    {
      iDynamicObject* dynobj = cell->GetObject (i);
      if (dynobj->GetEntityTemplate () == tpl) cntDynobj++;
    }
  }
  csRef<iCelEntityTemplateIterator> tplIt = pl->GetEntityTemplates ();
  while (tplIt->HasNext ())
  {
    iCelEntityTemplate* t = tplIt->Next ();
    csRef<iCelEntityTemplateIterator> parentsIt = t->GetParents ();
    while (parentsIt->HasNext ())
    {
      iCelEntityTemplate* p = parentsIt->Next ();
      if (p == tpl) cntParent++;
    }
  }
  if (dynworld->FindFactory (tpl->GetName ())) cntFact++;
  int cnt = cntDynobj + cntParent + cntFact;


  bool yes;
  if (cnt > 0)
    yes = ui->Ask ("There are %d usages of this template (%d times as parent and %d times in an object). Are you sure you want to remove the '%s' template?",
	cnt, cntParent, cntDynobj, tpl->GetName ());
  else
    yes = ui->Ask ("Are you sure you want to remove the '%s' template?", tpl->GetName ());
  if (yes)
  {
    view3d->GetApplication ()->GetAssetManager ()->RegisterRemoval (tpl->QueryObject ());
    view3d->GetApplication ()->UpdateTitle ();
    if (cnt > 0)
    {
      cellIt = dynworld->GetCells ();
      while (cellIt->HasNext ())
      {
	iDynamicCell* cell = cellIt->NextCell ();
	for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
	{
	  iDynamicObject* dynobj = cell->GetObject (i);
	  if (dynobj->GetEntityTemplate () == tpl)
	    dynobj->SetEntity (dynobj->GetEntityName (), dynobj->GetFactory ()->GetName (),
		dynobj->GetEntityParameters ());
	}
      }
      tplIt = pl->GetEntityTemplates ();
      while (tplIt->HasNext ())
      {
        iCelEntityTemplate* t = tplIt->Next ();
	t->RemoveParent (tpl);
      }
    }
    pl->RemoveEntityTemplate (tpl);
    view3d->GetModelRepository ()->GetTemplatesValue ()->Refresh ();
    view3d->GetModelRepository ()->GetObjectsValue ()->Refresh ();
    ActivateNode (0);
  }
}

void EntityMode::OnDelete ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  DeleteItem (contextMenuNode);
}

void EntityMode::DeleteItem (const char* item)
{
  const char type = item[0];
  if (type == 'T')
  {
    // Delete template.
    OnTemplateDel (currentTemplate);
  }
  else if (type == 'P')
  {
    // Delete property class.
    if (editQuestMode)
    {
      OnQuestDel (editQuestMode->GetName ());
    }
    else
    {
      iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
      iCelPropertyClassTemplate* pctpl = GetPCTemplate (item);
      tpl->RemovePropertyClassTemplate (pctpl);
      RegisterModification (tpl);
      editQuestMode = 0;
      RefreshView ();
    }
  }
  else if (type == 'S')
  {
    // Delete state.
    csString state = GetSelectedStateName (item);
    iQuestFactory* questFact = GetSelectedQuest (item);
    questFact->RemoveState (state);
    RegisterModification (questFact);
    RefreshView ();
  }
  else if (type == 't')
  {
    // Delete trigger.
    csString state = GetSelectedStateName (item);
    iQuestFactory* questFact = GetSelectedQuest (item);
    iQuestStateFactory* questState = questFact->GetState (state);
    csRef<iQuestTriggerResponseFactoryArray> responses = questState->GetTriggerResponseFactories ();
    iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (item);
    responses->Delete (resp);
    RegisterModification (questFact);
    RefreshView ();
  }
  else if (type == 'r')
  {
    // Delete reward.
    size_t idx;
    csRef<iRewardFactoryArray> array = GetSelectedReward (item, idx);
    if (!array) return;
    array->DeleteIndex (idx);
    iQuestFactory* questFact = GetSelectedQuest (item);
    RegisterModification (questFact);
    RefreshView ();
  }
  else if (type == 's')
  {
    // Delete sequence.
    iCelSequenceFactory* sequence = GetSelectedSequence (item);
    iQuestFactory* questFact = GetSelectedQuest (item);
    questFact->RemoveSequence (sequence->GetName ());
    RegisterModification (questFact);
    RefreshView ();
  }
}

void EntityMode::OnCreatePC ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New PropertyClass");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddChoice ("name", "pcobject.mesh", "pctools.properties",
      "pctools.inventory", "pclogic.quest", "pclogic.spawn", "pclogic.trigger",
      "pclogic.wire", "pctools.messenger",
      "pcinput.standard", "pcphysics.object", "pcphysics.system", "pccamera.old",
      "pcmove.actor.dynamic", "pcmove.actor.standard", "pcmove.actor.wasd",
      "pcworld.dynamic", "ares.gamecontrol",
      (const char*)0);
  dialog->AddRow ();
  dialog->AddLabel ("Tag:");
  dialog->AddText ("tag");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    csString tag = fields.Get ("tag", "");
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    iCelPropertyClassTemplate* pc = tpl->FindPropertyClassTemplate (name, tag);
    if (pc)
      ui->Error ("Property class with this name and tag already exists!");
    else
    {
      pc = tpl->CreatePropertyClassTemplate ();
      pc->SetName (name);
      if (tag && *tag)
        pc->SetTag (tag);

      view3d->GetApplication ()->RegisterModification (tpl->QueryObject ());
      RefreshView (pc);
    }

  }
}

void EntityMode::PCWasEdited (iCelPropertyClassTemplate* pctpl)
{
  RefreshView (pctpl);
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  view3d->GetApplication ()->RegisterModification (tpl->QueryObject ());
}

void EntityMode::ClearCopy ()
{
  entityCopy.name = "";
  entityCopy.doc = 0;
  entityCopy.node = 0;
  pcCopy.name = "";
  pcCopy.doc = 0;
  pcCopy.node = 0;
  questCopy.name = "";
  questCopy.doc = 0;
  questCopy.node = 0;
}

bool EntityMode::HasPaste ()
{
  if (!entityCopy.name.IsEmpty ()) return true;
  if (!pcCopy.name.IsEmpty ()) return true;
  if (questCopy.node) return true;
  return false;
}

QuestCopy EntityMode::Copy (iQuestFactory* questFact)
{
  QuestCopy copy;
  copy.name = questFact->GetName ();

  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (object_reg);
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  copy.doc = docsys->CreateDocument ();
  csRef<iDocumentNode> root = copy.doc->CreateRoot ();
  copy.node = root->CreateNodeBefore (CS_NODE_ELEMENT);
  copy.node->SetValue ("quest");
  questFact->Save (copy.node);

  return copy;
}

EntityCopy EntityMode::Copy (iCelEntityTemplate* tpl)
{
  EntityCopy copy;
  copy.name = tpl->GetName ();

  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (object_reg);
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  csRef<iEntityTemplateLoader> tplldr = csQueryRegistryOrLoad<iEntityTemplateLoader> (
      object_reg, "cel.addons.celentitytpl");

  copy.doc = docsys->CreateDocument ();
  csRef<iDocumentNode> root = copy.doc->CreateRoot ();
  copy.node = root->CreateNodeBefore (CS_NODE_ELEMENT);
  copy.node->SetValue ("template");
  tplldr->Save (tpl, copy.node);

  return copy;
}

PropertyClassCopy EntityMode::Copy (iCelPropertyClassTemplate* pctpl)
{
  PropertyClassCopy copy;
  copy.name = pctpl->GetName ();
  copy.tag = pctpl->GetTag ();

  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (object_reg);
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  csRef<iEntityTemplateLoader> tplldr = csQueryRegistryOrLoad<iEntityTemplateLoader> (
      object_reg, "cel.addons.celentitytpl");

  copy.doc = docsys->CreateDocument ();
  csRef<iDocumentNode> root = copy.doc->CreateRoot ();
  copy.node = root->CreateNodeBefore (CS_NODE_ELEMENT);
  copy.node->SetValue ("propclass");
  tplldr->Save (pctpl, copy.node);

  return copy;
}

void EntityMode::CopySelected ()
{
  csString a = GetActiveNode ();
  if (a.IsEmpty ()) return;
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  const char type = a.operator[] (0);
  if (type == 'P' && editQuestMode)
  {
    ClearCopy ();
    questCopy = Copy (editQuestMode);
  }
  else if (type == 'P')
  {
    iCelPropertyClassTemplate* pctpl = GetPCTemplate (a);
    if (!pctpl) return;
    ClearCopy ();
    pcCopy = Copy (pctpl);
  }
  else if (type == 'T')
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    ClearCopy ();
    entityCopy = Copy (tpl);
  }
  else
  {
    ui->Message ("Not implemented yet!");
  }
  app->SetMenuState ();
}


iCelPropertyClassTemplate* PropertyClassCopy::Create (EntityMode* em,
    iCelEntityTemplate* tpl, const char* overridetag)
{
  csRef<iEntityTemplateLoader> tplldr = csQueryRegistryOrLoad<iEntityTemplateLoader> (
      em->GetObjectRegistry (), "cel.addons.celentitytpl");

  if (overridetag)
    node->SetAttribute ("tag", overridetag);
  iCelPropertyClassTemplate* pctpl = tplldr->Load (tpl, node, 0);
  return pctpl;
}

iCelEntityTemplate* EntityCopy::Create (EntityMode* em, const char* overridename)
{
  csRef<iEntityTemplateLoader> tplldr = csQueryRegistryOrLoad<iEntityTemplateLoader> (
      em->GetObjectRegistry (), "cel.addons.celentitytpl");

  node->SetAttribute ("entityname", overridename);
  iCelEntityTemplate* tpl = tplldr->Load (node, 0);

  return tpl;
}

iQuestFactory* QuestCopy::Create (iQuestManager* questMgr, const char* overridename)
{
  iQuestFactory* questFact = questMgr->CreateQuestFactory (overridename);
  questFact->Load (node);
  return questFact;
}

void EntityMode::Paste ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();

  if (questCopy.node)
  {
    csString title = "Enter new name for quest factory";
    csRef<iString> name = ui->AskDialog (title, "Name:", questCopy.name);
    if (!name) return;
    iQuestFactory* questFact = questMgr->GetQuestFactory (name->GetData ());
    if (questFact)
    {
      ui->Error ("A quest factory with this name already exists!");
      return;
    }
    questFact = questCopy.Create (questMgr, name->GetData ());
    RegisterModification (questFact);
    SelectQuest (questFact);
  }
  else if (!pcCopy.name.IsEmpty ())
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    if (!tpl) return;
    csString title;
    title.Format ("Enter new tag for property class '%s'", pcCopy.name.GetData ());
    csRef<iString> tag = ui->AskDialog (title, "Tag:", pcCopy.tag);
    if (!tag) return;
    iCelPropertyClassTemplate* pc = tpl->FindPropertyClassTemplate (pcCopy.name, tag->GetData ());
    if (pc)
    {
      ui->Error ("Property class with this name and tag already exists!");
      return;
    }
    pcCopy.Create (this, tpl, tag->GetData ());
    RegisterModification (tpl);
    RefreshView ();
  }
  else if (!entityCopy.name.IsEmpty ())
  {
    csString title = "Enter new name for entity template";
    csRef<iString> name = ui->AskDialog (title, "Name:", entityCopy.name);
    if (!name) return;
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (name->GetData ());
    if (tpl)
    {
      ui->Error ("A template with this name already exists!");
      return;
    }
    tpl = entityCopy.Create (this, name->GetData ());
    RegisterModification (tpl);
    SelectTemplate (tpl);
  }
}

void EntityMode::DeleteSelected ()
{
  csString a = GetActiveNode ();
  if (a.IsEmpty ()) return;
  DeleteItem (a);
  ActivateNode (0);
}

csString EntityMode::GetActiveNode ()
{
  if (activeNode) return activeNode;
  csString page = UITools::GetValue (panel, "type_Notebook");
  if (page == "Templates")
  {
    wxListCtrl* list = XRCCTRL (*panel, "template_List", wxListCtrl);
    Ares::Value* v = view.GetSelectedValue (list);
    if (!v) return "";
    csString templateName = v->GetStringArrayValue ()->Get (TEMPLATE_COL_NAME);
    return csString ("T:") + templateName;
  }
  else if (page == "Quests")
  {
    wxListCtrl* list = XRCCTRL (*panel, "quest_List", wxListCtrl);
    Ares::Value* v = view.GetSelectedValue (list);
    if (!v) return "";
    return "P:pclogic.quest";
  }
  else return "";
}

void EntityMode::ActivateNode (const char* nodeName)
{
  tplPanel->Hide ();
  pcPanel->Hide ();
  triggerPanel->Hide ();
  rewardPanel->Hide ();
  sequencePanel->Hide ();
  activeNode = nodeName;
  app->SetMenuState ();
  if (!nodeName) return;
  printf ("ActivateNode %s\n", nodeName); fflush (stdout);
  const char type = activeNode.operator[] (0);
  if (type == 'P')
  {
    if (!editQuestMode)
    {
      iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
      iCelPropertyClassTemplate* pctpl = GetPCTemplate (activeNode);
printf ("activeNode=%s pctpl=%p\n", activeNode.GetData (), pctpl);
      pcPanel->SwitchToPC (tpl, pctpl);
      pcPanel->Show ();
    }
  }
  else if (type == 'T')
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    tplPanel->SwitchToTpl (tpl);
    tplPanel->Show ();
  }
  else if (type == 't')
  {
    iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (activeNode);
    if (!resp || !resp->GetTriggerFactory ()) return;
    iQuestFactory* questFact = GetSelectedQuest (activeNode);
    triggerPanel->SwitchTrigger (questFact, resp);
    triggerPanel->Show ();
  }
  else if (type == 'r')
  {
    size_t idx;
    csRef<iRewardFactoryArray> array = GetSelectedReward (activeNode, idx);
    if (!array) return;
    iRewardFactory* reward = array->Get (idx);
    rewardPanel->SwitchReward (GetSelectedQuest (activeNode), array, idx, reward);
    rewardPanel->Show ();
  }
  else if (type == 's')
  {
    iCelSequenceFactory* sequence = GetSelectedSequence (activeNode);
    if (!sequence) return;
    iQuestFactory* questFact = GetSelectedQuest (activeNode);
    sequencePanel->SwitchSequence (questFact, sequence);
    sequencePanel->Show ();
  }
}

void EntityMode::OnRewardMove (int dir)
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  size_t idx;
  csRef<iRewardFactoryArray> array = GetSelectedReward (GetContextMenuNode (), idx);
  if (!array) return;
  if (dir == -1 && idx <= 0) return;
  if (dir == 1 && idx >= array->GetSize ()-1) return;

  csRef<iRewardFactory> rf = array->Get (idx);
  array->DeleteIndex (idx);
  array->Insert (idx + dir, rf);

  iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
  RegisterModification (questFact);
  graphView->ActivateNode (0);
  ActivateNode (0);
  RefreshView ();
}

void EntityMode::OnCreateReward (int type)
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Reward");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddChoice ("name", "newstate", "debugprint", "action", "changeproperty",
      "createentity", "destroyentity", "changeclass", "inventory", "message", "cssequence",
      "sequence", "sequencefinish", (const char*)0);
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    iRewardType* rewardType = questMgr->GetRewardType ("cel.rewards."+name);
    csRef<iRewardFactory> fact = rewardType->CreateRewardFactory ();
    if (type == 0)
    {
      iQuestTriggerResponseFactory* resp = GetSelectedTriggerResponse (GetContextMenuNode ());
      if (!resp) return;
      resp->AddRewardFactory (fact);
    }
    else
    {
      iQuestStateFactory* state = GetSelectedState (GetContextMenuNode ());
      if (type == 1) state->AddInitRewardFactory (fact);
      else state->AddExitRewardFactory (fact);
    }
    iQuestFactory* questFact = GetSelectedQuest (activeNode);
    RegisterModification (questFact);
    RefreshView ();
  }
}

void EntityMode::OnCreateTrigger ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Trigger");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddChoice ("name", "entersector", "meshentersector", "inventory",
      "meshselect", "message", "operation", "propertychange", "sequencefinish",
      "timeout", "trigger", "watch", (const char*)0);
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    csString state = GetSelectedStateName (GetContextMenuNode ());
    iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
    iQuestStateFactory* questState = questFact->GetState (state);
    iQuestTriggerResponseFactory* resp = questState->CreateTriggerResponseFactory ();
    iTriggerType* triggertype = questMgr->GetTriggerType ("cel.triggers."+name);
    csRef<iTriggerFactory> triggerFact = triggertype->CreateTriggerFactory ();
    resp->SetTriggerFactory (triggerFact);
    RegisterModification (questFact);
    RefreshView ();
  }
}

void EntityMode::OnDefaultState ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());

  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact) return;

  csStringArray tokens (GetContextMenuNode (), ",");
  csString state = tokens[0];
  state = state.Slice (2);
  pctpl->RemoveProperty (pl->FetchStringID ("state"));
  pctpl->SetProperty (pl->FetchStringID ("state"), state.GetData ());

  RegisterModification ();

  RefreshView (pctpl);
}

void EntityMode::OnNewSequence ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New Sequence", "Name:");
  if (!name) return;
  if (name->IsEmpty ()) return;

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact)
    questFact = questMgr->CreateQuestFactory (questName);
  if (questFact->GetSequence (name->GetData ()))
  {
    ui->Error ("Sequence already exists with this name!");
    return;
  }
  questFact->CreateSequence (name->GetData ());
  RegisterModification (questFact);
  RefreshView (pctpl);
}

void EntityMode::OnNewState ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iString> name = ui->AskDialog ("New State", "Name:");
  if (!name) return;
  if (name->IsEmpty ()) return;

  iQuestFactory* questFact = questMgr->GetQuestFactory (questName);
  if (!questFact)
    questFact = questMgr->CreateQuestFactory (questName);
  if (questFact->GetState (name->GetData ()))
  {
    ui->Error ("State already exists with this name!");
    return;
  }
  questFact->CreateState (name->GetData ());
  RegisterModification (questFact);
  RefreshView (pctpl);
}

void EntityMode::OnEditQuest ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return;

  editQuestMode = questMgr->GetQuestFactory (questName);
  if (!editQuestMode)
    editQuestMode = questMgr->CreateQuestFactory (questName);

  currentTemplate = "";

  RefreshView ();
}

bool EntityMode::Command (csStringID id, const csString& args)
{
  if (id == ID_Copy) { CopySelected (); return true; }
  if (id == ID_Paste) { Paste (); return true; }
  if (id == ID_Delete) { DeleteSelected (); return true; }
  return false;
}

bool EntityMode::IsCommandValid (csStringID id, const csString& args,
      iSelection* selection, size_t pastesize)
{
  if (id == ID_Copy)
  {
    csString a = GetActiveNode ();
    if (a.IsEmpty ()) return false;
    const char type = a.operator[] (0);
    return type == 'P' || type == 'T';
  }
  if (id == ID_Paste) return HasPaste ();
  if (id == ID_Delete) return !GetActiveNode ().IsEmpty ();
  return true;
}

csPtr<iString> EntityMode::GetAlternativeLabel (csStringID id,
      iSelection* selection, size_t pastesize)
{
  if (id == ID_Paste)
  {
    if (questCopy.node)
    {
      scfString* label = new scfString ();
      label->Format ("Paste %s\tCtrl+V", questCopy.name.GetData ());
      return label;
    }
    else if (!entityCopy.name.IsEmpty ())
    {
      scfString* label = new scfString ();
      label->Format ("Paste %s\tCtrl+V", entityCopy.name.GetData ());
      return label;
    }
    if (!pcCopy.name.IsEmpty ())
    {
      scfString* label = new scfString ();
      label->Format ("Paste %s\tCtrl+V", pcCopy.name.GetData ());
      return label;
    }
    return new scfString ("Paste\tCtrl+V");
  }
  return 0;
}

void EntityMode::AllocContextHandlers (wxFrame* frame)
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();

  idDelete = ui->AllocContextMenuID ();
  frame->Connect (idDelete, wxEVT_COMMAND_MENU_SELECTED,
	      wxCommandEventHandler (EntityMode::Panel::OnDelete), 0, panel);
  idCreate = ui->AllocContextMenuID ();
  frame->Connect (idCreate, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreatePC), 0, panel);
  idEditQuest = ui->AllocContextMenuID ();
  frame->Connect (idEditQuest, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnEditQuest), 0, panel);
  idNewState = ui->AllocContextMenuID ();
  frame->Connect (idNewState, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnNewState), 0, panel);
  idNewSequence = ui->AllocContextMenuID ();
  frame->Connect (idNewSequence, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnNewSequence), 0, panel);
  idDefaultState = ui->AllocContextMenuID ();
  frame->Connect (idDefaultState, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnDefaultState), 0, panel);
  idCreateTrigger = ui->AllocContextMenuID ();
  frame->Connect (idCreateTrigger, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateTrigger), 0, panel);
  idCreateReward = ui->AllocContextMenuID ();
  frame->Connect (idCreateReward, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateReward), 0, panel);
  idCreateRewardOnInit = ui->AllocContextMenuID ();
  frame->Connect (idCreateRewardOnInit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateRewardOnInit), 0, panel);
  idCreateRewardOnExit = ui->AllocContextMenuID ();
  frame->Connect (idCreateRewardOnExit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnCreateRewardOnExit), 0, panel);
  idRewardUp = ui->AllocContextMenuID ();
  frame->Connect (idRewardUp, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnRewardUp), 0, panel);
  idRewardDown = ui->AllocContextMenuID ();
  frame->Connect (idRewardDown, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnRewardDown), 0, panel);

  idNewChar = ui->AllocContextMenuID ();
  frame->Connect (idNewChar, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnNewCharacteristic), 0, panel);
  idDelChar = ui->AllocContextMenuID ();
  frame->Connect (idDelChar, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (EntityMode::Panel::OnDeleteCharacteristic), 0, panel);
}

csString EntityMode::GetContextMenuNode ()
{
  if (!contextMenuNode) return "";
  if (!graphView->NodeExists (contextMenuNode))
  {
    contextMenuNode = "";
  }
  return contextMenuNode;
}

void EntityMode::AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY)
{
  contextMenuNode = graphView->FindHitNode (mouseX, mouseY);
  if (!contextMenuNode.IsEmpty ())
  {
    wxMenuItem* item;
    iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());

    contextMenu->AppendSeparator ();

    const char type = contextMenuNode.operator[] (0);
    switch (type)
    {
      case 'T':
        contextMenu->Append (idDelete, wxT ("Delete Template"));
        contextMenu->Append (idCreate, wxT ("Create Property Class..."));
	break;
      case 'P':
        if (pctpl)
          contextMenu->Append (idDelete, wxT ("Delete Property Class"));
        else
          contextMenu->Append (idDelete, wxT ("Delete Quest"));
        if (contextMenuNode.StartsWith ("P:pclogic.quest"))
        {
          item = contextMenu->Append (idEditQuest, wxT ("Edit quest"));
          item->Enable (pctpl != 0);
          contextMenu->Append (idNewState, wxT ("New state..."));
          contextMenu->Append (idNewSequence, wxT ("New sequence..."));
        }
	break;
      case 'S':
        contextMenu->Append (idDelete, wxT ("Delete State"));
        item = contextMenu->Append (idDefaultState, wxT ("Set default state"));
        item->Enable (pctpl != 0);
        contextMenu->Append (idCreateTrigger, wxT ("Create trigger..."));
        contextMenu->Append (idCreateRewardOnInit, wxT ("Create on-init reward..."));
        contextMenu->Append (idCreateRewardOnExit, wxT ("Create on-exit reward..."));
	break;
      case 's':
        contextMenu->Append (idDelete, wxT ("Delete Sequence"));
	break;
      case  'r':
	{
          contextMenu->Append (idDelete, wxT ("Delete Reward"));
          size_t idx;
          csRef<iRewardFactoryArray> array = GetSelectedReward (GetContextMenuNode (), idx);
          item = contextMenu->Append (idRewardUp, wxT ("Move Up"));
	  item->Enable (idx > 0);
          item = contextMenu->Append (idRewardDown, wxT ("Move Down"));
	  item->Enable (idx < array->GetSize ()-1);
	}
	break;
      case 'i':
        contextMenu->Append (idCreateRewardOnInit, wxT ("Create on-init reward..."));
	break;
      case 'e':
        contextMenu->Append (idCreateRewardOnExit, wxT ("Create on-exit reward..."));
	break;
      case 't':
        contextMenu->Append (idDelete, wxT ("Delete Trigger"));
        contextMenu->Append (idCreateReward, wxT ("Create reward..."));
	break;
    }
  }
}

void EntityMode::FramePre()
{
}

void EntityMode::Frame3D()
{
  g3d->GetDriver2D ()->Clear (g3d->GetDriver2D ()->FindRGB (100, 110, 120));
}

void EntityMode::Frame2D()
{
}

bool EntityMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  if (code == '1')
  {
    float f = graphView->GetNodeForceFactor ();
    f -= 5.0f;
    graphView->SetNodeForceFactor (f);
    printf ("Node force factor %g\n", f); fflush (stdout);
  }
  if (code == '2')
  {
    float f = graphView->GetNodeForceFactor ();
    f += 5.0f;
    graphView->SetNodeForceFactor (f);
    printf ("Node force factor %g\n", f); fflush (stdout);
  }
  if (code == '3')
  {
    float f = graphView->GetLinkForceFactor ();
    f -= 0.01f;
    graphView->SetLinkForceFactor (f);
    printf ("Link force factor %g\n", f); fflush (stdout);
  }
  if (code == '4')
  {
    float f = graphView->GetLinkForceFactor ();
    f += 0.01f;
    graphView->SetLinkForceFactor (f);
    printf ("Link force factor %g\n", f); fflush (stdout);
  }
  return false;
}

bool EntityMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

bool EntityMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

bool EntityMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return false;
}

