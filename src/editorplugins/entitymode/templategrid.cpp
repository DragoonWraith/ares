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
#include "templategrid.h"
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
  Value* chosen = ui->AskDialog ("Select a template", 400, objects,
      "Template,M", TEMPLATE_COL_NAME, TEMPLATE_COL_MODIFIED);
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
  Value* chosen = ui->AskDialog ("Select a class", 700, classes, "Class,Description", CLASS_COL_NAME,
	    CLASS_COL_DESCRIPTION);
  if (chosen)
  {
    csString name = chosen->GetStringArrayValue ()->Get (CLASS_COL_NAME);
    value = wxString::FromUTF8 (name);
    return true;
  }
  return false;
}

// ------------------------------------------------------------------------

class PcEditorSupportMessenger : public PcEditorSupport
{
private:
  int idNewSlot;
  int idDelSlot;
  int idNewType;
  int idDelType;
  wxArrayString anchorArray;

public:
  PcEditorSupportMessenger (EntityMode* emode) : PcEditorSupport ("pctools.messenger", emode)
  {
    idNewSlot = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcMsg_OnNewSlot));
    idDelSlot = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcMsg_OnDelSlot));
    idNewType = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcMsg_OnNewType));
    idDelType = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcMsg_OnDelType));

    anchorArray.Add (wxT ("c"));
    anchorArray.Add (wxT ("nw"));
    anchorArray.Add (wxT ("n"));
    anchorArray.Add (wxT ("ne"));
    anchorArray.Add (wxT ("e"));
    anchorArray.Add (wxT ("se"));
    anchorArray.Add (wxT ("s"));
    anchorArray.Add (wxT ("sw"));
    anchorArray.Add (wxT ("s"));
  }

  virtual ~PcEditorSupportMessenger () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "DefineSlot")
      {
        csString slotName = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "name");
        csString position = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "position");
        bool queue = InspectTools::GetActionParameterValueBool (pl, pctpl, idx, "queue");
        csString screenanchor = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "screenanchor");
        csString boxanchor = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "boxanchor");
        csString sizex = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "sizex");
        csString sizey = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "sizey");
        csString maxsizex = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "maxsizex");
        csString maxsizey = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "maxsizey");
        csString borderwidth = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "borderwidth");
        csString roundness = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "roundness");
        csString maxmessages = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "maxmessages");
        csString boxfadetime = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "boxfadetime");

	csString s;
	s.Format ("Slot:%d", int (idx));
	wxPGProperty* outputProp = AppendStringPar (pcProp, "Slot", s, "<composed>");

	AppendStringPar (outputProp, "Name", "Name", slotName);
	AppendStringPar (outputProp, "Position", "Position", position);
	AppendBoolPar (outputProp, "Queue", "Queue", queue);
	AppendStringPar (outputProp, "Size X", "SizeX", sizex);
	AppendStringPar (outputProp, "Size Y", "SizeY", sizey);
	AppendStringPar (outputProp, "Max Size X", "MaxSizeX", maxsizex);
	AppendStringPar (outputProp, "Max Size Y", "MaxSizeY", maxsizey);
        AppendEnumPar (outputProp, "Screen Anchor", "ScreenAnchor", anchorArray, wxArrayInt(),
	  anchorArray.Index (wxString::FromUTF8 (screenanchor)));
        AppendEnumPar (outputProp, "Box Anchor", "BoxAnchor", anchorArray, wxArrayInt(),
	  anchorArray.Index (wxString::FromUTF8 (boxanchor)));
	AppendStringPar (outputProp, "Border Width", "BorderWidth", borderwidth);
	AppendStringPar (outputProp, "Roundness", "Roundness", roundness);
	AppendStringPar (outputProp, "Max Messages", "MaxMessages", maxmessages);
	AppendStringPar (outputProp, "Box Fade Time", "BoxFadeTime", boxfadetime);
      }
    }

    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "DefineType")
      {
        csString type = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "type");
        csString slot = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "slot");
        csString timeout = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "timeout");
        csString fadetime = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "fadetime");
        bool click = InspectTools::GetActionParameterValueBool (pl, pctpl, idx, "click");
        bool log = InspectTools::GetActionParameterValueBool (pl, pctpl, idx, "log");

	csString s;
	s.Format ("Type:%d", int (idx));
	wxPGProperty* outputProp = AppendStringPar (pcProp, "Type", s, "<composed>");

	AppendStringPar (outputProp, "Type", "Type", type);
	AppendStringPar (outputProp, "Slot", "Slot", slot);
	AppendStringPar (outputProp, "Timeout", "Timeout", timeout);
	AppendStringPar (outputProp, "Fadetime", "Fadetime", fadetime);
	AppendBoolPar (outputProp, "Click", "Click", click);
	AppendBoolPar (outputProp, "Log", "Log", log);
      }
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);

    if (selectedPropName.StartsWith ("Slot:"))
    {
      size_t dot = selectedPropName.FindLast ('.');
      int idx;
      if (dot == csArrayItemNotFound) return REFRESH_NOCHANGE;
      csScanStr (selectedPropName.Slice (0, dot).GetData () + strlen ("Slot:"), "%d", &idx);
      printf ("UpdateMsg:selectedPropName=%s\n", selectedPropName.GetData ()); fflush (stdout);
      if (selectedPropName.EndsWith (".Name"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "name", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Position"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "position", CEL_DATA_VECTOR2, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Queue"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "queue", CEL_DATA_BOOL, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".ScreenAnchor"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "screenanchor", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".BoxAnchor"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "boxanchor", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".SizeX"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "sizex", CEL_DATA_LONG, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".SizeY"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "sizey", CEL_DATA_LONG, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".MaxSizeX"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "maxsizex", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".MaxSizeY"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "maxsizey", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".BorderWidth"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "borderwidth", CEL_DATA_FLOAT, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Roundness"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "roundness", CEL_DATA_LONG, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".MaxMessages"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "maxmessages", CEL_DATA_LONG, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".BoxFadeTime"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "boxfadetime", CEL_DATA_LONG, value);
        return REFRESH_NO;
      }
    }

    if (selectedPropName.StartsWith ("Type:"))
    {
      size_t dot = selectedPropName.FindLast ('.');
      int idx;
      if (dot == csArrayItemNotFound) return REFRESH_NOCHANGE;
      csScanStr (selectedPropName.Slice (0, dot).GetData () + strlen ("Type:"), "%d", &idx);
      printf ("UpdateMsg:selectedPropName=%s\n", selectedPropName.GetData ()); fflush (stdout);
      if (selectedPropName.EndsWith (".Type"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "type", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Slot"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "slot", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Timeout"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "timeout", CEL_DATA_FLOAT, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Fadetime"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "fadetime", CEL_DATA_FLOAT, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Click"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "click", CEL_DATA_BOOL, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Log"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "log", CEL_DATA_BOOL, value);
        return REFRESH_NO;
      }
    }

    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewSlot, wxT ("New Slot..."));
    if (selectedPropName.StartsWith ("Slot:"))
    {
      contextMenu->Append (idDelSlot, wxT ("Delete Slot"));
    }
    contextMenu->Append (idNewType, wxT ("New Type..."));
    if (selectedPropName.StartsWith ("Type:"))
    {
      contextMenu->Append (idDelType, wxT ("Delete Type"));
    }
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportDynworld : public PcEditorSupport
{
public:
  PcEditorSupportDynworld (EntityMode* emode) : PcEditorSupport ("pcworld.dynamic", emode)
  {
  }

  virtual ~PcEditorSupportDynworld () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    bool valid;
    bool physics = InspectTools::GetPropertyValueBool (pl, pctpl, "physics", &valid);
    AppendBoolPar (pcProp, "Physics", "Physics", valid ? physics : true);
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);
    if (selectedPropName == "Physics")
    {
      InspectTools::SetProperty (pl, pctpl, CEL_DATA_BOOL, "physics", value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportSpawn : public PcEditorSupport
{
private:
  int idNewTemplate;
  int idDelTemplate;

public:
  PcEditorSupportSpawn (EntityMode* emode) : PcEditorSupport ("pclogic.spawn", emode)
  {
    idNewTemplate = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcSpawn_OnNewTemplate));
    idDelTemplate = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcSpawn_OnDelTemplate));
  }

  virtual ~PcEditorSupportSpawn () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
  // @@@ TODO AddSpawnPosition
    bool repeat = InspectTools::GetActionParameterValueBool (pl, pctpl, "SetTiming", "repeat");
    AppendBoolPar (pcProp, "Repeat", "Repeat", repeat);
    bool random = InspectTools::GetActionParameterValueBool (pl, pctpl, "SetTiming", "random");
    AppendBoolPar (pcProp, "Random", "Random", random);
    bool valid;
    long mindelay = InspectTools::GetActionParameterValueLong (pl, pctpl, "SetTiming", "mindelay", &valid);
    AppendIntPar (pcProp, "MinDelay", "MinDelay", valid ? mindelay : 0);
    long maxdelay = InspectTools::GetActionParameterValueLong (pl, pctpl, "SetTiming", "maxdelay", &valid);
    AppendIntPar (pcProp, "MaxDelay", "MaxDelay", valid ? maxdelay : 0);

    bool spawnunique = InspectTools::GetPropertyValueBool (pl, pctpl, "spawnunique");
    AppendBoolPar (pcProp, "SpawnUnique", "SpawnUnique", spawnunique);
    bool namecounter = InspectTools::GetPropertyValueBool (pl, pctpl, "namecounter");
    AppendBoolPar (pcProp, "NameCounter", "NameCounter", namecounter);

    long inhibit = InspectTools::GetActionParameterValueLong (pl, pctpl, "Inhibit", "count", &valid);
    AppendIntPar (pcProp, "Inhibit", "Inhibit", valid ? inhibit : 0); // @@@ 0?

    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddEntityTemplateType")
      {
        csString tplName = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "template");

	csString s;
	s.Format ("Template:%d", int (idx));
	wxPGProperty* outputProp = AppendStringPar (pcProp, "Output", s, "<composed>");

	AppendButtonPar (outputProp, "Template", "Template", BUTTON_TEMPLATE, tplName);
	// @@@ Todo spawn supports more parameters for the template.
      }
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);

    if (selectedPropName == "Repeat")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetTiming", "repeat", CEL_DATA_BOOL, value);
      return REFRESH_NO;
    }
    if (selectedPropName == "Random")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetTiming", "random", CEL_DATA_BOOL, value);
      return REFRESH_NO;
    }
    if (selectedPropName == "MinDelay")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetTiming", "mindelay", CEL_DATA_LONG, value);
      return REFRESH_NO;
    }
    if (selectedPropName == "MaxDelay")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetTiming", "maxdelay", CEL_DATA_LONG, value);
      return REFRESH_NO;
    }
    if (selectedPropName == "SpawnUnique")
    {
      InspectTools::SetProperty (pl, pctpl, CEL_DATA_BOOL, "spawnunique", value);
      return REFRESH_NO;
    }
    if (selectedPropName == "NameCounter")
    {
      InspectTools::SetProperty (pl, pctpl, CEL_DATA_BOOL, "namecounter", value);
      return REFRESH_NO;
    }
    if (selectedPropName == "Inhibit")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "Inhibit", "count", CEL_DATA_LONG, value);
      return REFRESH_NO;
    }
  
    if (selectedPropName.StartsWith ("Template:"))
    {
      size_t dot = selectedPropName.FindLast ('.');
      int idx;
      if (dot == csArrayItemNotFound) return REFRESH_NOCHANGE;
      csScanStr (selectedPropName.Slice (0, dot).GetData () + strlen ("Template:"), "%d", &idx);
      printf ("UpdateSpawn:selectedPropName=%s index=%d\n", selectedPropName.GetData (), idx); fflush (stdout);
      if (selectedPropName.EndsWith (".Template"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "template", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
    }

    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewTemplate, wxT ("New Template..."));
    if (selectedPropName.StartsWith ("Template:"))
    {
      contextMenu->Append (idDelTemplate, wxT ("Delete Template"));
    }
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportOldCamera : public PcEditorSupport
{
private:
  wxArrayString modesArray;

public:
  PcEditorSupportOldCamera (EntityMode* emode) : PcEditorSupport ("pccamera.old", emode)
  {
    modesArray.Add (wxT ("freelook"));
    modesArray.Add (wxT ("firstperson"));
    modesArray.Add (wxT ("thirdperson"));
    modesArray.Add (wxT ("m64_thirdperson"));
    modesArray.Add (wxT ("lara_thirdperson"));
  }

  virtual ~PcEditorSupportOldCamera () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    csString inputMask = InspectTools::GetActionParameterValueString (pl, pctpl, "SetCamera", "modename");
    AppendEnumPar (pcProp, "Mode", "Mode", modesArray, wxArrayInt (),
	  modesArray.Index (wxString::FromUTF8 (inputMask)));
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);

    if (selectedPropName == "Mode")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetCamera", "modename", CEL_DATA_STRING, value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportInventory : public PcEditorSupport
{
private:
  int idNewTemplate;
  int idDelTemplate;

public:
  PcEditorSupportInventory (EntityMode* emode) : PcEditorSupport ("pctools.inventory", emode)
  {
    idNewTemplate = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcInv_OnNewTemplate));
    idDelTemplate = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcInv_OnDelTemplate));
  }

  virtual ~PcEditorSupportInventory () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddTemplate")
      {
        csString tplName = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "name");
        csString amount = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "amount");

	csString s;
	s.Format ("Template:%d", int (idx));
	wxPGProperty* outputProp = AppendStringPar (pcProp, "Template", s, "<composed>");

	AppendButtonPar (outputProp, "Template", "Template", BUTTON_TEMPLATE, tplName);
	AppendStringPar (outputProp, "Amount", "Amount", amount);
      }
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);

    if (selectedPropName.StartsWith ("Template:"))
    {
      size_t dot = selectedPropName.FindLast ('.');
      int idx;
      if (dot == csArrayItemNotFound) return REFRESH_NOCHANGE;
      csScanStr (selectedPropName.Slice (0, dot).GetData () + strlen ("Template:"), "%d", &idx);
      printf ("UpdateInv:selectedPropName=%s\n", selectedPropName.GetData ()); fflush (stdout);
      if (selectedPropName.EndsWith (".Template"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "name", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Amount"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "amount", CEL_DATA_LONG, value);
        return REFRESH_NO;
      }
    }

    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewTemplate, wxT ("New Template..."));
    if (selectedPropName.StartsWith ("Template:"))
    {
      contextMenu->Append (idDelTemplate, wxT ("Delete Template"));
    }
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportWire : public PcEditorSupport
{
private:
  int idNewPar;
  int idDelPar;
  int idNewOutput;
  int idDelOutput;

public:
  PcEditorSupportWire (EntityMode* emode) : PcEditorSupport ("pclogic.wire", emode)
  {
    idNewPar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcWire_OnNewParameter));
    idDelPar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcWire_OnDelParameter));
    idNewOutput = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcWire_OnNewOutput));
    idDelOutput = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcWire_OnDelOutput));
  }

  virtual ~PcEditorSupportWire () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    csString inputMask = InspectTools::GetActionParameterValueString (pl, pctpl, "AddInput", "mask");
    AppendButtonPar (pcProp, "Input", "Input", BUTTON_ACTION, inputMask);

    csStringID msgID = pl->FetchStringID ("msgid");
    csStringID entityID = pl->FetchStringID ("entity");

    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddOutput")
      {
	csArray<csStringID> ids;
	csArray<iParameter*> pars;
        csString msg;
        csString entity;
        while (params->HasNext ())
        {
          csStringID parid;
          iParameter* par = params->Next (parid);
          if (parid == msgID) msg = par->GetOriginalExpression ();
          else if (parid == entityID) entity = par->GetOriginalExpression ();
          else
          {
	    ids.Push (parid);
	    pars.Push (par);
          }
        }

	csString s;
	s.Format ("Output:%d", int (idx));
	wxPGProperty* outputProp = AppendStringPar (pcProp, "Output", s, "<composed>");

	AppendButtonPar (outputProp, "Message", "Message", BUTTON_ACTION, msg);
	AppendButtonPar (outputProp, "Entity", "Entity", BUTTON_ENTITY, entity);

	for (size_t i = 0 ; i < ids.GetSize () ; i++)
	{
	  csString name = pl->FetchString (ids.Get (i));
	  iParameter* par = pars.Get (i);
	  AppendPar (outputProp, "Par", name, par->GetPossibleType (), par->GetOriginalExpression ());
	}
      }
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);

    if (selectedPropName == "Input")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "AddInput", "mask", CEL_DATA_STRING, value);
      return REFRESH_NO;
    }
  
    if (selectedPropName.StartsWith ("Output:"))
    {
      size_t dot = selectedPropName.FindLast ('.');
      int idx;
      if (dot == csArrayItemNotFound) return REFRESH_NOCHANGE;
      csScanStr (selectedPropName.Slice (0, dot).GetData () + strlen ("Output:"), "%d", &idx);
      printf ("UpdateWire:selectedPropName=%s\n", selectedPropName.GetData ()); fflush (stdout);
      if (selectedPropName.EndsWith (".Entity"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "entity", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Message"))
      {
        InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), "msgid", CEL_DATA_STRING, value);
        return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Name"))
      {
        size_t dot1 = selectedPropName.FindFirst ('.');
	csString oldParName = selectedPropName.Slice (dot1+5, dot-dot1-5);
        if (oldParName != value)
	{
	  csRef<iParameter> par = InspectTools::GetActionParameterValue (pl, pctpl, size_t (idx),
	      oldParName);
	  InspectTools::DeleteActionParameter (pl, pctpl, size_t (idx), oldParName);
          InspectTools::AddActionParameter (pl, pctpl, size_t (idx), value, par);
	  return REFRESH_NO;
	}
      }
      else if (selectedPropName.EndsWith (".Value"))
      {
        size_t dot1 = selectedPropName.FindFirst ('.');
	csString parName = selectedPropName.Slice (dot1+5, dot-dot1-5);
	csRef<iParameter> par = InspectTools::GetActionParameterValue (pl, pctpl, size_t (idx),
	    parName);
	par = pm->GetParameter (value, par->GetPossibleType ());
	InspectTools::AddActionParameter (pl, pctpl, size_t (idx), parName, par);
	return REFRESH_NO;
      }
      else if (selectedPropName.EndsWith (".Type"))
      {
        size_t dot1 = selectedPropName.FindFirst ('.');
	csString parName = selectedPropName.Slice (dot1+5, dot-dot1-5);
	csRef<iParameter> par = InspectTools::GetActionParameterValue (pl, pctpl, size_t (idx),
	    parName);
	par = pm->GetParameter (par->GetOriginalExpression (), InspectTools::StringToType (value));
	InspectTools::AddActionParameter (pl, pctpl, size_t (idx), parName, par);
	return REFRESH_NO;
      }
    }

    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewOutput, wxT ("New Output..."));
    if (selectedPropName.StartsWith ("Output:"))
    {
      contextMenu->Append (idDelOutput, wxT ("Delete Output"));
      contextMenu->Append (idNewPar, wxT ("New Wire Parameter..."));
      if (selectedPropName.Find (".Par:") != csArrayItemNotFound)
        contextMenu->Append (idDelPar, wxT ("Delete Wire Parameter"));
    }
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportBag : public PcEditorSupport
{
private:
  int idNewString;
  int idDelString;

public:
  PcEditorSupportBag (EntityMode* emode) : PcEditorSupport ("pctools.bag", emode)
  {
    idNewString = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcBag_OnNewString));
    idDelString = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcBag_OnDelString));
  }

  virtual ~PcEditorSupportBag () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    for (size_t idx = 0 ; idx < pctpl->GetPropertyCount () ; idx++)
    {
      csStringID id;
      celData data;
      csRef<iCelParameterIterator> params = pctpl->GetProperty (idx, id, data);
      csString name = pl->FetchString (id);
      if (name == "AddString")
      {
	csString str = InspectTools::GetActionParameterValueExpression (pl, pctpl, idx, "string");
	csString s;
	s.Format ("String:%d", int (idx));
	AppendStringPar (pcProp, "String", s, str);
      }
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);
    if (selectedPropName.StartsWith ("String:"))
    {
      int idx;
      csScanStr (selectedPropName.GetData () + 7, "%d", &idx);
      InspectTools::AddActionParameter (pl, pm, pctpl, idx, "string", CEL_DATA_STRING, value);
      return REFRESH_NO;
    }

    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewString, wxT ("New String..."));
    if (selectedPropName.StartsWith ("String:"))
      contextMenu->Append (idDelString, wxT ("Delete String"));
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportQuest : public PcEditorSupport
{
private:
  int idNewPar;
  int idDelPar;
  int idSuggestPar;

public:
  PcEditorSupportQuest (EntityMode* emode) : PcEditorSupport ("pclogic.quest", emode)
  {
    idNewPar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcQuest_OnNewParameter));
    idDelPar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcQuest_OnDelParameter));
    idSuggestPar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcQuest_OnSuggestParameters));
  }

  virtual ~PcEditorSupportQuest () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    iQuestFactory* questFact = emode->GetQuestFactory (pctpl);
    csString questName = questFact ? questFact->GetName () : "";
    AppendButtonPar (pcProp, "Quest", "Quest", BUTTON_QUEST, questName);

    csString defaultState = InspectTools::GetPropertyValueString (pl, pctpl, "state");
    wxArrayString states;
    states.Add (wxT ("-"));
    int idx = 1, defState = 0;
    if (questFact)
    {
      csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
      while (it->HasNext ())
      {
        iQuestStateFactory* stateFact = it->Next ();
        states.Add (wxString::FromUTF8 (stateFact->GetName ()));
        if (defaultState == stateFact->GetName ())
	  defState = idx;
        idx++;
      }
    }
    AppendEnumPar (pcProp, "State", "State", states, wxArrayInt (), defState);

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
	AppendPar (pcProp, "Par", name,
	    nextPar->GetPossibleType (), nextPar->GetOriginalExpression ());
      }
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString questName = GetPropertyValueAsString (pcPropName, "Quest");
    csString oldQuestName = emode->GetQuestName (pctpl);
    if (questName != oldQuestName)
    {
      pctpl->RemoveAllProperties ();
      InspectTools::AddActionParameter (pl, pm, pctpl, "NewQuest", "name", CEL_DATA_STRING, questName);
      return REFRESH_PC;
    }

    if (selectedPropName == "State")
    {
      csString value = (const char*)selectedProperty->GetValueAsString ().mb_str (wxConvUTF8);
      InspectTools::SetProperty (pl, pctpl, CEL_DATA_STRING, "state", value);
      return REFRESH_NO;
    }

    if (selectedPropName.StartsWith ("Par:") && !selectedPropName.EndsWith (".Value")
	&& !selectedPropName.EndsWith (".Type") && !selectedPropName.EndsWith (".Name"))
    {
      csString oldParName = selectedPropName.GetData () + 4;
      csString newParName = GetPropertyValueAsString (pcPropName, selectedPropName+".Name");
      csString newTypeS = GetPropertyValueAsString (pcPropName, selectedPropName+".Type");
      csString newValue = GetPropertyValueAsString (pcPropName, selectedPropName+".Value");

      if (oldParName != newParName)
	InspectTools::DeleteActionParameter (pl, pctpl, "NewQuest", oldParName);
      celDataType newType = InspectTools::StringToType (newTypeS);
      InspectTools::AddActionParameter (pl, pm, pctpl, "NewQuest", newParName, newType, newValue);
      return REFRESH_NO;
    }

    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    if (selectedPropName.StartsWith ("Par:") && selectedPropName.EndsWith (".Name"))
    {
      csString oldParName = GetPropertyValueAsString (pcPropName, selectedPropName);
      if (value.IsEmpty ())
	return ui->Error ("Empty name is not allowed!");
      else if (oldParName != value)
      {
	iParameter* par = InspectTools::GetActionParameterValue (pl, pctpl, "NewQuest", value);
	if (par)
	  return ui->Error ("There is already a parameter with this name!");
      }
    }
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewPar, wxT ("New Quest Parameter..."));
    if (selectedPropName.StartsWith ("Par:"))
      contextMenu->Append (idDelPar, wxT ("Delete Quest Parameter"));
    contextMenu->Append (idSuggestPar, wxT ("Suggest Quest Parameters"));
  }
};

// ------------------------------------------------------------------------

class PcEditorSupportProperties : public PcEditorSupport
{
private:
  int idNewProp;
  int idDelProp;

public:
  PcEditorSupportProperties (EntityMode* emode) : PcEditorSupport ("pctools.properties", emode)
  {
    idNewProp = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcProp_OnNewProperty));
    idDelProp = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::PcProp_OnDelProperty));
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
      AppendPar (pcProp, "Prop", name, InspectTools::ResolveType (data), value);
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    if (selectedPropName.StartsWith ("Prop:") && !selectedPropName.EndsWith (".Value")
	&& !selectedPropName.EndsWith (".Type") && !selectedPropName.EndsWith (".Name"))
    {
      csString oldParName = selectedPropName.GetData () + 5;
      csString newParName = GetPropertyValueAsString (pcPropName, selectedPropName+".Name");
      csString newTypeS = GetPropertyValueAsString (pcPropName, selectedPropName+".Type");
      csString newValue = GetPropertyValueAsString (pcPropName, selectedPropName+".Value");
      printf ("oldPropName=%s new=%s/%s/%s\n", oldParName.GetData (), newParName.GetData (),
	  newTypeS.GetData (), newValue.GetData ()); fflush (stdout);

      if (oldParName != newParName)
	pctpl->RemoveProperty (pl->FetchStringID (oldParName));
      celDataType newType = InspectTools::StringToType (newTypeS);
      InspectTools::SetProperty (pl, pctpl, newType, newParName, newValue);
      return REFRESH_NO;
    }

    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    if (selectedPropName.StartsWith ("Prop:") && selectedPropName.EndsWith (".Name"))
    {
      csString oldParName = GetPropertyValueAsString (pcPropName, selectedPropName);
      if (value.IsEmpty ())
	return ui->Error ("Empty name is not allowed!");
      else if (oldParName != value)
      {
	if (pctpl->FindProperty (pl->FetchStringID (value)) != csArrayItemNotFound)
	  return ui->Error ("There is already a property with this name!");
      }
    }
    return true;
  }

  virtual void DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (idNewProp, wxT ("New Property..."));
    if (selectedPropName.StartsWith ("Prop:"))
      contextMenu->Append (idDelProp, wxT ("Delete Property"));
  }
};

// ------------------------------------------------------------------------

static bool ToBool (const char* value)
{
  csString lvalue = csString (value).Downcase ();
  return lvalue == "1" || lvalue == "true" || lvalue == "yes" || lvalue == "on";
}


class PcEditorSupportTrigger : public PcEditorSupport
{
private:
  wxArrayString trigtypesArray;

public:
  PcEditorSupportTrigger (EntityMode* emode) : PcEditorSupport ("pclogic.trigger", emode)
  {
    trigtypesArray.Add (wxT ("..."));
    trigtypesArray.Add (wxT ("Sphere"));
    trigtypesArray.Add (wxT ("Box"));
    trigtypesArray.Add (wxT ("Beam"));
    trigtypesArray.Add (wxT ("Above"));
  }
  virtual ~PcEditorSupportTrigger () { }

  virtual void Fill (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
  {
    bool valid;
    long delay = InspectTools::GetPropertyValueLong (pl, pctpl, "delay", &valid);
    AppendIntPar (pcProp, "Delay", "Delay", valid ? delay : 200);
    long jitter = InspectTools::GetPropertyValueLong (pl, pctpl, "jitter", &valid);
    AppendIntPar (pcProp, "Jitter", "Jitter", valid ? jitter : 10);

    csString monitor = InspectTools::GetPropertyValueString (pl, pctpl, "monitor", &valid);
    AppendButtonPar (pcProp, "Monitor", "Monitor", BUTTON_ENTITY, monitor);

    csString clazz = InspectTools::GetPropertyValueString (pl, pctpl, "class", &valid);
    AppendButtonPar (pcProp, "Class", "Class", BUTTON_CLASS, clazz);

    bool strict = InspectTools::GetPropertyValueBool (pl, pctpl, "strict", &valid);
    AppendBoolPar (pcProp, "Strict", "Strict", strict);
    bool follow = InspectTools::GetPropertyValueBool (pl, pctpl, "follow", &valid);
    AppendBoolPar (pcProp, "Follow", "Follow", follow);

    wxPGProperty* typeProp = AppendEnumPar (pcProp, "Type", "TrigType", trigtypesArray, wxArrayInt ());
    if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerSphere")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Sphere"));
      csString par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerSphere", "radius");
      AppendStringPar (typeProp, "Radius", "Radius", par);
      par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerSphere", "position");
      AppendVectorPar (typeProp, "Position", par);
    }
    else if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerBox")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Box"));
      csString par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerBox", "minbox");
      AppendVectorPar (typeProp, "MinBox", par);
      par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerBox", "maxbox");
      AppendVectorPar (typeProp, "MaxBox", par);
    }
    else if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerBeam")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Beam"));
      csString par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerBeam", "start");
      AppendVectorPar (typeProp, "Start", par);
      par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerBeam", "end");
      AppendVectorPar (typeProp, "End", par);
    }
    else if (pctpl->FindProperty (pl->FetchStringID ("SetupTriggerAboveMesh")) != csArrayItemNotFound)
    {
      typeProp->SetValue (wxT ("Above"));
      csString par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerAboveMesh", "entity");
      AppendButtonPar (typeProp, "Entity", "Entity", BUTTON_ENTITY, par);
      par = InspectTools::GetActionParameterValueExpression (pl, pctpl, "SetupTriggerAboveMesh", "maxdistance");
      AppendStringPar (typeProp, "MaxDistance", "MaxDistance", par);
    }
    else
    {
      typeProp->SetValue (wxT ("..."));
      printf ("Huh! Unknown trigger type!\n");
    }
  }

  virtual RefreshType Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
  {
    csString value = GetPropertyValueAsString (pcPropName, selectedPropName);
    if (selectedPropName == "Delay")
    {
      long delay = GetPropertyValueAsInt (pcPropName, selectedPropName);
      pctpl->SetProperty (pl->FetchStringID ("delay"), delay);
      return REFRESH_NO;
    }
    else if (selectedPropName == "Jitter")
    {
      long jitter = GetPropertyValueAsInt (pcPropName, selectedPropName);
      pctpl->SetProperty (pl->FetchStringID ("jitter"), jitter);
      return REFRESH_NO;
    }
    else if (selectedPropName == "Monitor")
    {
      pctpl->SetProperty (pl->FetchStringID ("monitor"), value.GetData ());
      return REFRESH_NO;
    }
    else if (selectedPropName == "Class")
    {
      pctpl->SetProperty (pl->FetchStringID ("class"), value.GetData ());
      return REFRESH_NO;
    }
    else if (selectedPropName == "Strict")
    {
      pctpl->SetProperty (pl->FetchStringID ("strict"), ToBool (value));
      return REFRESH_NO;
    }
    else if (selectedPropName == "Follow")
    {
      pctpl->SetProperty (pl->FetchStringID ("follow"), ToBool (value));
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType")
    {
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerBox"));
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerSphere"));
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerBeam"));
      pctpl->RemoveProperty (pl->FetchStringID ("SetupTriggerAboveMesh"));
      if (value == "Sphere")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerSphere", CEL_DATA_NONE);
	return REFRESH_PC;
      }
      else if (value == "Box")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerBox", CEL_DATA_NONE);
	return REFRESH_PC;
      }
      else if (value == "Beam")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerBeam", CEL_DATA_NONE);
	return REFRESH_PC;
      }
      else if (value == "Above")
      {
	InspectTools::AddAction (pl, emode->GetPM (), pctpl, "SetupTriggerAboveMesh", CEL_DATA_NONE);
	return REFRESH_PC;
      }
      else
	return REFRESH_PC;
    }
    else if (selectedPropName == "TrigType.Radius")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerSphere", "radius", CEL_DATA_FLOAT, value);
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType.Position")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerSphere", "position", CEL_DATA_VECTOR3, value);
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType.MinBox")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerBox", "minbox", CEL_DATA_VECTOR3, value);
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType.MaxBox")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerBox", "maxbox", CEL_DATA_VECTOR3, value);
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType.Start")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerBeam", "start", CEL_DATA_VECTOR3, value);
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType.End")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerBeam", "end", CEL_DATA_VECTOR3, value);
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType.Entity")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerAboveMesh", "entity", CEL_DATA_STRING, value);
      return REFRESH_NO;
    }
    else if (selectedPropName == "TrigType.MaxDistance")
    {
      InspectTools::AddActionParameter (pl, pm, pctpl, "SetupTriggerAboveMesh", "maxdistance", CEL_DATA_FLOAT, value);
      return REFRESH_NO;
    }
    return REFRESH_NOCHANGE;
  }

  virtual bool Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
  {
    return true;
  }
};

// ------------------------------------------------------------------------

bool PcEditorSupportTemplate::ValidateTemplateParentsFromGrid (const wxPropertyGridEvent& event)
{
  wxArrayString templates = event.GetValue ().GetArrayString ();
  for (size_t i = 0 ; i < templates.GetCount () ; i++)
  {
    csString name = (const char*)templates.Item (i).mb_str (wxConvUTF8);
    iCelEntityTemplate* parent = pl->FindEntityTemplate (name);
    if (!parent)
      return ui->Error ("Can't find template '%s'!", name.GetData ());
  }
  return true;
}

void PcEditorSupportTemplate::AppendCharacteristics (wxPGProperty* parentProp,
      iCelEntityTemplate* tpl)
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

void PcEditorSupportTemplate::AppendTemplatesPar (
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
  tempProp->SetEntityMode (emode);
  detailGrid->AppendIn (parentProp, tempProp);
}

void PcEditorSupportTemplate::AppendClassesPar (
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
  classProp->SetEntityMode (emode);
  detailGrid->AppendIn (parentProp, classProp);
}

RefreshType PcEditorSupportTemplate::UpdateCharacteristicFromGrid (wxPGProperty* property, const csString& propName)
{
  iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();
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
  return REFRESH_NO;
}

RefreshType PcEditorSupportTemplate::UpdateTemplateClassesFromGrid ()
{
  iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();
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
    return REFRESH_NO;
  }
  return REFRESH_NOCHANGE;
}

RefreshType PcEditorSupportTemplate::UpdateTemplateParentsFromGrid ()
{
  iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();
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
    return REFRESH_NO;
  }
  return REFRESH_NOCHANGE;
}

void PcEditorSupportTemplate::FillPC (wxPGProperty* pcProp, iCelPropertyClassTemplate* pctpl)
{
  AppendEnumPar (pcProp, "Type", "Type", pctypesArray, wxArrayInt(),
      pctypesArray.Index (wxString::FromUTF8 (pctpl->GetName ())));
  AppendStringPar (pcProp, "Tag", "Tag", pctpl->GetTag ());

  PcEditorSupport* editor = GetEditor (pctpl->GetName ());
  if (editor) editor->Fill (pcProp, pctpl);
}

PcEditorSupportTemplate::PcEditorSupportTemplate (EntityMode* emode) :
  PcEditorSupport ("template", emode)
{
  pctypesArray.Add (wxT ("pcobject.mesh"));
  pctypesArray.Add (wxT ("pctools.properties"));
  pctypesArray.Add (wxT ("pctools.inventory"));
  pctypesArray.Add (wxT ("pctools.bag"));
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
  pctypesArray.Add (wxT ("pcmove.linear"));
  pctypesArray.Add (wxT ("pcworld.dynamic"));
  pctypesArray.Add (wxT ("ares.gamecontrol"));

  idNewChar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnNewCharacteristic));
  idDelChar = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnDeleteCharacteristic));
  idCreatePC = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnCreatePC));
  idDelPC = RegisterContextMenu (wxCommandEventHandler (EntityMode::Panel::OnDeletePC));

  RegisterEditor (new PcEditorSupportQuest (emode));
  RegisterEditor (new PcEditorSupportBag (emode));
  RegisterEditor (new PcEditorSupportWire (emode));
  RegisterEditor (new PcEditorSupportOldCamera (emode));
  RegisterEditor (new PcEditorSupportDynworld (emode));
  RegisterEditor (new PcEditorSupportSpawn (emode));
  RegisterEditor (new PcEditorSupportInventory (emode));
  RegisterEditor (new PcEditorSupportMessenger (emode));
  RegisterEditor (new PcEditorSupportProperties (emode));
  RegisterEditor (new PcEditorSupportTrigger (emode));
}

void PcEditorSupportTemplate::Fill (wxPGProperty* templateProp, iCelPropertyClassTemplate* pctpl)
{
  csString s;
  iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();

  if (pctpl)
  {
    // Special case to refresh only the PC in the grid.
    FillPC (templateProp, pctpl);
    return;
  }
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
    FillPC (pcProp, pctpl);
  }
}

RefreshType PcEditorSupportTemplate::Update (wxPGProperty* selectedProperty,
    iCelPropertyClassTemplate*& pctpl)
{
  csString selectedPropName, pcPropName;
  pctpl = GetPCForProperty (selectedProperty, pcPropName, selectedPropName);
  printf ("PC/PG changed %s/%s!\n", selectedPropName.GetData (), pcPropName.GetData ()); fflush (stdout);
  return Update (pctpl, pcPropName, selectedPropName, selectedProperty);
}

RefreshType PcEditorSupportTemplate::Update (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxPGProperty* selectedProperty)
{
  if (pctpl)
  {
    csString tag = GetPropertyValueAsString (pcPropName, "Tag");
    if (tag != pctpl->GetTag ())
    {
      if (tag.IsEmpty ())
	pctpl->SetTag (0);
      else
	pctpl->SetTag (tag);
      return REFRESH_NO;
    }

    csString type = GetPropertyValueAsString (pcPropName, "Type");
    if (type != pctpl->GetName ())
    {
      pctpl->SetName (type);
      pctpl->RemoveAllProperties ();
      return REFRESH_PC;
    }

    PcEditorSupport* editor = GetEditor (type);
    if (editor)
      return editor->Update (pctpl, pcPropName, selectedPropName, selectedProperty);
  }
  else if (selectedPropName == "Parents")
  {
    return UpdateTemplateParentsFromGrid ();
  }
  else if (selectedPropName == "Classes")
  {
    return UpdateTemplateClassesFromGrid ();
  }
  else if (selectedPropName.StartsWith ("Char:") && !selectedPropName.EndsWith (".Value")
	&& !selectedPropName.EndsWith (".Name"))
  {
    return UpdateCharacteristicFromGrid (selectedProperty, selectedPropName);
  }

  return REFRESH_NOCHANGE;
}

bool PcEditorSupportTemplate::Validate (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName,
      const csString& value, const wxPropertyGridEvent& event)
{
  iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();
  if (pctpl)
  {
    if (selectedPropName == "Tag")
    {
      iCelPropertyClassTemplate* pc = tpl->FindPropertyClassTemplate (pctpl->GetName (), value);
      if (pc && pc != pctpl)
	return ui->Error ("Property class with this name and tag already exists!");
    }

    csString type = GetPropertyValueAsString (pcPropName, "Type");
    PcEditorSupport* editor = GetEditor (pctpl->GetName ());
    if (editor)
      return editor->Validate (pctpl, pcPropName, selectedPropName, value, event);
  }
  else if (selectedPropName == "Parents")
  {
    if (!ValidateTemplateParentsFromGrid (event))
      return false;
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
	return false;
    }
    else if (selectedPropName.EndsWith (".Name"))
    {
      csString oldName = (const char*)event.GetProperty ()->GetValueAsString ().mb_str (wxConvUTF8);
      if (value.IsEmpty ())
	return ui->Error ("Empty name is not allowed!");
      else if (oldName != value && tpl->GetCharacteristics ()->HasCharacteristic (value))
	return ui->Error ("There is already a characteristic with this name!");
    }
  }

  return true;
}

void PcEditorSupportTemplate::DoContext (iCelPropertyClassTemplate* pctpl,
      const csString& pcPropName, const csString& selectedPropName, wxMenu* contextMenu)
{
  contextMenu->Append (idNewChar, wxT ("New characteristic..."));
  if (selectedPropName.StartsWith ("Char:"))
    contextMenu->Append (idDelChar, wxT ("Delete characteristic"));
  contextMenu->Append (idCreatePC, wxT ("Create Property Class..."));
  if (pctpl)
  {
    contextMenu->Append (idDelPC, wxT ("Delete Property Class"));
    PcEditorSupport* editor = GetEditor (pctpl->GetName ());
    if (editor)
      editor->DoContext (pctpl, pcPropName, selectedPropName, contextMenu);
  }
}

void PcEditorSupportTemplate::DoContext (wxPGProperty* property, wxMenu* contextMenu)
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (property,
	pcPropName, selectedPropName);
  DoContext (pctpl, pcPropName, selectedPropName, contextMenu);
}

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

iCelPropertyClassTemplate* PcEditorSupportTemplate::GetPCForProperty (wxPGProperty* property,
    csString& pcPropName, csString& selectedPropName)
{
  selectedPropName = GetPropertyName (property);
  wxPGProperty* pcProperty = FindPCProperty (property);
  if (pcProperty)
  {
    pcPropName = GetPropertyName (pcProperty);
    int idx;
    csScanStr (pcPropName.GetData () + 3, "%d", &idx);
    iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();
    return tpl->GetPropertyClassTemplate (idx);
  }
  return 0;
}

void PcEditorSupportTemplate::PcProp_OnDelProperty ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (),
      pcPropName, selectedPropName);
  size_t dot = selectedPropName.FindFirst ('.');
  csString name;
  if (dot == csArrayItemNotFound)
    name = selectedPropName.Slice (5);
  else
    name = selectedPropName.Slice (5, dot-5);
  pctpl->RemoveProperty (pl->FetchStringID (name));
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcProp_OnNewProperty ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (),
      pcPropName, selectedPropName);
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Quest Parameter",
      "LName:;TName;CType,string,float,long,bool,vector2,vector3,color\nMValue");
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
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::OnDeleteCharacteristic ()
{
  csString selectedPropName, pcPropName;
  GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);
  size_t dot = selectedPropName.FindFirst ('.');
  csString name;
  if (dot == csArrayItemNotFound)
    name = selectedPropName.Slice (5);
  else
    name = selectedPropName.Slice (5, dot-5);
  iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();
  tpl->GetCharacteristics ()->ClearCharacteristic (name);
  emode->PCWasEdited (0, REFRESH_TEMPLATE);
  emode->SelectTemplate (tpl);
}

void PcEditorSupportTemplate::OnNewCharacteristic ()
{
  csString selectedPropName, pcPropName;
  GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Characteristic Parameter", "LName:;TName\nTValue");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString name = result.Get ("Name", (const char*)0);
  csString value = result.Get ("Value", (const char*)0);

  iCelEntityTemplate* tpl = emode->GetCurrentTemplate ();
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
  emode->PCWasEdited (0, REFRESH_TEMPLATE);
  emode->SelectTemplate (tpl);
}

void PcEditorSupportTemplate::PcQuest_OnSuggestParameters ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);
  iQuestFactory* questFact = emode->GetQuestFactory (pctpl);
  if (!questFact) return;

  csHash<ParameterDomain,csStringID> suggestions = InspectTools::GetQuestParameters (pl, questFact);
  csHash<ParameterDomain,csStringID>::GlobalIterator it = suggestions.GetIterator ();
  while (it.HasNext ())
  {
    csStringID parID;
    ParameterDomain type = it.Next (parID);
    csString name = pl->FetchString (parID);
    csRef<iParameter> par = pm->GetParameter ("", type.type);
    if (!par) break;	// @@@ Report error?
    InspectTools::AddActionParameter (pl, pctpl, "NewQuest", name, par);
  }
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::OnDeleteActionParameter ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  size_t colon = selectedPropName.FindFirst (':');
  size_t dot = selectedPropName.FindLast ('.');
  size_t dot1 = selectedPropName.FindFirst ('.');
  size_t colon2 = selectedPropName.FindFirst (':', dot1+1);
  int lenparname = colon2 - dot1 + 1;
  int idx;
  csScanStr (selectedPropName.Slice (0, dot).GetData () + (colon+1), "%d", &idx);
  csString parName;
  if (dot1 == dot)
    parName = selectedPropName.Slice (dot1+lenparname);
  else
    parName = selectedPropName.Slice (dot1+lenparname, dot-dot1-lenparname);

  InspectTools::DeleteActionParameter (pl, pctpl, size_t (idx), parName);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::OnDeleteProperty ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  size_t colon = selectedPropName.FindFirst (':');
  size_t dot = selectedPropName.FindLast ('.');
  int idx;
  if (dot == csArrayItemNotFound)
    csScanStr (selectedPropName.GetData () + (colon+1), "%d", &idx);
  else
    csScanStr (selectedPropName.Slice (0, dot).GetData () + (colon+1), "%d", &idx);

  pctpl->RemovePropertyByIndex (idx);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcMsg_OnNewSlot ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  csRef<iUIDialog> dialog = ui->CreateDialog ("New Message Slot", "LName:;TName");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString name = result.Get ("Name", (const char*)0);

  if (name.IsEmpty ())
  {
    ui->Error ("Empty slot name is not allowed!");
    return;
  }

  InspectTools::AddAction (pl, pm, pctpl, "DefineSlot",
      CEL_DATA_STRING, "name", name.GetData (),
      CEL_DATA_NONE);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcMsg_OnNewType ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  csRef<iUIDialog> dialog = ui->CreateDialog ("New Message Type", "LName:;TName");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString name = result.Get ("Name", (const char*)0);

  if (name.IsEmpty ())
  {
    ui->Error ("Empty type name is not allowed!");
    return;
  }

  InspectTools::AddAction (pl, pm, pctpl, "DefineType",
      CEL_DATA_STRING, "type", name.GetData (),
      CEL_DATA_NONE);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcSpawn_OnNewTemplate ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  csRef<iUIDialog> dialog = ui->CreateDialog ("New Spawn Template", "LTemplate:;ETTemplate");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString tpl = result.Get ("Template", (const char*)0);

  if (tpl.IsEmpty ())
  {
    ui->Error ("Empty template is not allowed!");
    return;
  }

  InspectTools::AddAction (pl, pm, pctpl, "AddEntityTemplateType",
      CEL_DATA_STRING, "template", tpl.GetData (),
      CEL_DATA_NONE);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcInv_OnNewTemplate ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  csRef<iUIDialog> dialog = ui->CreateDialog ("New Inventory Template",
      "LTemplate:;ETTemplate\nLAmount:;TAmount");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString tpl = result.Get ("Template", (const char*)0);
  csString amount = result.Get ("Amount", (const char*)0);

  if (tpl.IsEmpty ())
  {
    ui->Error ("Empty template is not allowed!");
    return;
  }

  InspectTools::AddAction (pl, pm, pctpl, "AddTemplate",
      CEL_DATA_STRING, "name", tpl.GetData (),
      CEL_DATA_LONG, "amount", amount.GetData (),
      CEL_DATA_NONE);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcBag_OnNewString ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  csRef<iUIDialog> dialog = ui->CreateDialog ("New Bag String",
      "LString:;TString");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString str = result.Get ("String", (const char*)0);

  InspectTools::AddAction (pl, pm, pctpl, "AddString",
      CEL_DATA_STRING, "string", str.GetData (),
      CEL_DATA_NONE);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcBag_OnDelString ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  int idx;
  csScanStr (selectedPropName.GetData () + 7, "%d", &idx);
  pctpl->RemovePropertyByIndex (idx);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcWire_OnNewOutput ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  csRef<iUIDialog> dialog = ui->CreateDialog ("New Wire Output");
  dialog->AddRow ();
  dialog->AddLabel ("Message:");
  iEditorConfig* config = emode->GetApplication ()->GetConfig ();
  csStringArray messageArray;
  const csArray<KnownMessage>& messages = config->GetMessages ();
  for (size_t i = 0 ; i < messages.GetSize () ; i++)
    messageArray.Push (messages.Get (i).name);
  dialog->AddCombo ("Message", messageArray);
  dialog->AddRow ();
  dialog->AddLabel ("Entity:");
  dialog->AddTypedText (SPT_ENTITY, "Entity");
  if (dialog->Show (0) == 0) return;
  DialogResult result = dialog->GetFieldContents ();
  csString message = result.Get ("Message", (const char*)0);
  csString entity = result.Get ("Entity", (const char*)0);

  if (message.IsEmpty ())
  {
    ui->Error ("Empty message is not allowed!");
    return;
  }

  InspectTools::AddAction (pl, pm, pctpl, "AddOutput",
      CEL_DATA_STRING, "msgid", message.GetData (),
      CEL_DATA_STRING, "entity", entity.GetData (),
      CEL_DATA_NONE);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcWire_OnNewParameter ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);

  size_t dot = selectedPropName.FindLast ('.');
  int idx;
  csScanStr (selectedPropName.Slice (0, dot).GetData () + strlen ("Output:"), "%d", &idx);

  csRef<iUIDialog> dialog = ui->CreateDialog ("New Wire Parameter",
      "LName:;TName;CType,string,float,long,bool,vector2,vector3,color\nMValue");
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
  else if (InspectTools::GetActionParameterValue (pl, pctpl, size_t (idx), name))
  {
    ui->Error ("There is already a parameter with this name!");
    return;
  }

  celDataType type = InspectTools::StringToType (typeS);
  InspectTools::AddActionParameter (pl, pm, pctpl, size_t (idx), name, type, value);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

void PcEditorSupportTemplate::PcQuest_OnNewParameter ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = GetPCForProperty (emode->GetContextLastProperty (), pcPropName, selectedPropName);
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Quest Parameter",
      "LName:;TName;CType,string,float,long,bool,vector2,vector3,color\nMValue");
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
  InspectTools::AddActionParameter (pl, pm, pctpl, "NewQuest", name, type, value);
  emode->PCWasEdited (pctpl, REFRESH_PC);
}

