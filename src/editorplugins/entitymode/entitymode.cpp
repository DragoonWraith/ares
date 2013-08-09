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
#include "edcommon/transformtools.h"
#include "edcommon/uitools.h"
#include "entitymode.h"
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/imodelrepository.h"
#include "editor/iconfig.h"
#include "editor/icamerawin.h"
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
#include <wx/notebook.h>
#include "cseditor/wx/propgrid/propdev.h"

//---------------------------------------------------------------------------

#define PG_ID (wxID_HIGHEST+1)


BEGIN_EVENT_TABLE(EntityMode::Panel, wxPanel)
  EVT_LIST_ITEM_SELECTED (XRCID("template_List"), EntityMode::Panel::OnTemplateSelect)
  EVT_LIST_ITEM_SELECTED (XRCID("quest_List"), EntityMode::Panel::OnQuestSelect)
  EVT_PG_CHANGING (PG_ID, EntityMode::Panel::OnPropertyGridChanging)
  EVT_PG_CHANGED (PG_ID, EntityMode::Panel::OnPropertyGridChanged)
#ifdef CS_PLATFORM_WIN32
  EVT_PG_RIGHT_CLICK (PG_ID, EntityMode::Panel::OnPropertyGridRight)
#else
  EVT_CONTEXT_MENU (EntityMode::Panel::OnContextMenu)
#endif
  EVT_BUTTON (PG_ID, EntityMode::Panel::OnPropertyGridButton)
  EVT_IDLE (EntityMode::Panel::OnIdle)
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
  delayedRefreshType = REFRESH_NOCHANGE;
  refreshPctpl = 0;
  refreshStateFact = 0;
  refreshSeqFact = 0;
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

static void Set3Value (EntityMode* emode, wxPGProperty* prop, const char* v1,
    const char* v2, const char* v3)
{
  prop->Item (0)->SetValue (wxString::FromUTF8 (v1));
  prop->Item (1)->SetValue (wxString::FromUTF8 (v2));
  prop->Item (2)->SetValue (wxString::FromUTF8 (v3));
  csString composed;
  composed.Format ("%s; %s; %s", v1, v2, v3);
  prop->SetValue (wxString::FromUTF8 (composed));

  emode->OnPropertyGridChanged (prop->Item (0));
  emode->OnPropertyGridChanged (prop->Item (1));
  emode->OnPropertyGridChanged (prop->Item (2));
}

static void Set3Value (EntityMode* emode, wxPGProperty* prop, const csVector3& v)
{
  csString s1, s2, s3;
  s1.Format ("%g", v.x);
  s2.Format ("%g", v.y);
  s3.Format ("%g", v.z);
  Set3Value (emode, prop, s1, s2, s3);
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
    else if (propName.StartsWith ("A:"))
    {
      Value* objects = view3d->GetModelRepository ()->GetActionsValue ();
      chosen = ui->AskDialog ("Select an action", objects, "Action,Description", ACTION_COL_NAME,
	    ACTION_COL_DESCRIPTION);
      col = ACTION_COL_NAME;
    }
    else if (propName.StartsWith ("T:"))
    {
      Value* objects = view3d->GetModelRepository ()->GetTemplatesValue ();
      chosen = ui->AskDialog ("Select a template", objects, "Template,M", TEMPLATE_COL_NAME,
	    TEMPLATE_COL_MODIFIED);
      col = TEMPLATE_COL_NAME;
    }
    else if (propName.StartsWith ("V:"))
    {
      csRef<iUIDialog> dialog = ui->CreateDialog ("Position Wizard",
	  "WOrigin\nWFrom an object...\nWCurrent 3D View Camera Position\nWCurrent Selected Object\nWStored Position 1\nWStored Position 2\nWStored Position 3");
      int result = dialog->Show (0);
      switch (result)
      {
	case 0:
	case 1:
	  break;
	case 2:
	  Set3Value (this, selectedProperty, "0", "0", "0");
	  break;
	case 3:
	  {
            Value* objects = view3d->GetModelRepository ()->GetObjectsValue ();
	    chosen = ui->AskDialog ("Select an object", objects,
		"Entity,Template,Dynfact,X,Y,Z",
		DYNOBJ_COL_X, DYNOBJ_COL_Y, DYNOBJ_COL_Z,
		DYNOBJ_COL_ENTITY, DYNOBJ_COL_TEMPLATE, DYNOBJ_COL_FACTORY);
	    if (chosen)
	    {
	      csString x = chosen->GetStringArrayValue ()->Get (DYNOBJ_COL_X);
	      csString y = chosen->GetStringArrayValue ()->Get (DYNOBJ_COL_Y);
	      csString z = chosen->GetStringArrayValue ()->Get (DYNOBJ_COL_Z);
	      Set3Value (this, selectedProperty, x, y, z);
	    }
	  }
	  break;
	case 4:
	  Set3Value (this, selectedProperty,
	      view3d->GetCsCamera ()->GetTransform ().GetOrigin ());
	  break;
	case 5:
	  Set3Value (this, selectedProperty, TransformTools::GetCenterSelected (
		  view3d->GetSelection ()));
	  break;
	case 6:
	  Set3Value (this, selectedProperty, app->GetCameraWindow ()->GetStoredLocation (0));
	  break;
	case 7:
	  Set3Value (this, selectedProperty, app->GetCameraWindow ()->GetStoredLocation (1));
	  break;
	case 8:
	  Set3Value (this, selectedProperty, app->GetCameraWindow ()->GetStoredLocation (2));
	  break;
      }
      return;
    }
    if (chosen)
    {
      csString n = chosen->GetStringArrayValue ()->Get (col);
      selectedProperty->SetValue (wxString::FromUTF8 (n));
      OnPropertyGridChanged (selectedProperty);
    }
  }
}

// @@@ This should not be needed but for some reason on windows the EVT_CONTEXT_MENU
// is not generated when hovering on the property grid.
void EntityMode::OnPropertyGridRight (wxPropertyGridEvent& event)
{
  printf ("OnPropertyGridRight\n"); fflush (stdout);
  contextLastProperty = event.GetProperty ();

  if (contextLastProperty)
  {
    wxMenu contextMenu;
    if (editQuestMode)
      questEditor->DoContext (contextLastProperty, &contextMenu);
    else
      templateEditor->DoContext (contextLastProperty, &contextMenu);
    panel->PopupMenu (&contextMenu);
  }
}

void EntityMode::OnContextMenu (wxContextMenuEvent& event)
{
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
      iCelPropertyClassTemplate* pctpl = templateEditor->GetPCForProperty (contextLastProperty,
	  pcPropName, selectedPropName);
      wxMenu contextMenu;
      templateEditor->DoContext (pctpl, pcPropName, selectedPropName, &contextMenu);
      panel->PopupMenu (&contextMenu);
    }
  }
}

void EntityMode::OnIdle ()
{
  if (delayedRefreshType != REFRESH_NOCHANGE)
  {
    printf ("Delayed refresh %d!\n", delayedRefreshType); fflush (stdout);
    if (editQuestMode)
    {
      QuestWasEdited (refreshStateFact, refreshSeqFact, delayedRefreshType);
    }
    else
    {
      PCWasEdited (refreshPctpl, delayedRefreshType);
      if (!refreshPctpl)
        SelectTemplate (GetCurrentTemplate ());
    }
    delayedRefreshType = REFRESH_NOCHANGE;
    refreshPctpl = 0;
    refreshStateFact = 0;
    refreshSeqFact = 0;
  }
}

void EntityMode::DelayedRefresh (iQuestStateFactory* stateFact, iCelSequenceFactory* seqFact,
      RefreshType refreshType)
{
  delayedRefreshType = refreshType;
  refreshPctpl = 0;
  refreshStateFact = stateFact;
  refreshSeqFact = seqFact;
}

void EntityMode::DelayedRefresh (iCelPropertyClassTemplate* pctpl, RefreshType refreshType)
{
  delayedRefreshType = refreshType;
  refreshPctpl = pctpl;
  refreshStateFact = 0;
  refreshSeqFact = 0;
}

void EntityMode::OnPropertyGridChanging (wxPropertyGridEvent& event)
{
  wxPGProperty* selectedProperty = event.GetProperty ();
  if (editQuestMode)
  {
    // @@@ Todo?
  }
  else
  {
    csString selectedPropName, pcPropName;
    iCelPropertyClassTemplate* pctpl = templateEditor->GetPCForProperty (selectedProperty, pcPropName, selectedPropName);
    printf ("PG changing %s/%s!\n", selectedPropName.GetData (), pcPropName.GetData ()); fflush (stdout);
    csString value = (const char*)event.GetValue ().GetString ().mb_str (wxConvUTF8);
    if (!templateEditor->Validate (pctpl, pcPropName, selectedPropName, value, event))
      event.Veto ();
  }
}

void EntityMode::OnPropertyGridChanged (wxPropertyGridEvent& event)
{
  wxPGProperty* selectedProperty = event.GetProperty ();
  OnPropertyGridChanged (selectedProperty);
}

void EntityMode::OnPropertyGridChanged (wxPGProperty* selectedProperty)
{
  if (editQuestMode)
  {
    iQuestStateFactory* stateFact;
    iCelSequenceFactory* seqFact;
    RefreshType refreshType = questEditor->Update (selectedProperty, stateFact, seqFact);
    if (refreshType != REFRESH_NOCHANGE)
      DelayedRefresh (stateFact, seqFact, refreshType);
  }
  else
  {
    iCelPropertyClassTemplate* pctpl;
    RefreshType refreshType = templateEditor->Update (selectedProperty, pctpl);
    if (refreshType != REFRESH_NOCHANGE)
      DelayedRefresh (pctpl, refreshType);
  }
}

void EntityMode::BuildDetailGrid ()
{
  wxPanel* detailPanel = XRCCTRL (*panel, "detail_Panel", wxPanel);

  detailGrid = new wxPropertyGrid (detailPanel);
  detailGrid->SetId (PG_ID);
  detailPanel->GetSizer ()->Add (detailGrid, 1, wxEXPAND | wxALL);
  //detailGrid->SetColumnCount (3);

  templateEditor.AttachNew (new PcEditorSupportTemplate (this));
  questEditor.AttachNew (new QuestEditorSupportMain (this));
}

void EntityMode::FillDetailGrid (iQuestFactory* questFact)
{
  csString s;
  detailGrid->Freeze ();

  if (questFact)
  {
    detailGrid->Clear ();
    s.Format ("Quest (%s)", questFact->GetName ());
    wxPGProperty* questProp = detailGrid->Append (new wxPropertyCategory (wxString::FromUTF8 (s)));
    questEditor->Fill (questProp, questFact);
  }
  else
  {
    detailGrid->Clear ();
  }

  detailGrid->FitColumns ();
  detailGrid->Thaw ();
}

void EntityMode::FillDetailGrid (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl)
{
  csString s;
  detailGrid->Freeze ();

  if (pctpl)
  {
    for (size_t i = 0 ; i < tpl->GetPropertyClassTemplateCount () ; i++)
    {
      iCelPropertyClassTemplate* pc = tpl->GetPropertyClassTemplate (i);
      if (pc == pctpl)
      {
        s.Format ("PC:%d", int (i));
	wxPGProperty* pcProp = detailGrid->GetPropertyByName (wxString::FromUTF8 (s));
	if (!pcProp)
	{
	  // There is no property for this PC yet. We insert one at the appropriate
	  // place.
	  csString ss;
	  ss.Format ("Template (%s)", tpl->GetName ());
	  wxPGProperty* templateProp = detailGrid->GetPropertyByName (wxString::FromUTF8 (ss));
	  wxPropertyCategory* propCat = new wxPropertyCategory (wxT ("PC"), wxString::FromUTF8 (s));
	  if (i == tpl->GetPropertyClassTemplateCount ()-1)
	    pcProp = detailGrid->AppendIn (templateProp, propCat);
	  else
	    pcProp = detailGrid->Insert (templateProp, i, propCat);
	}

	if (pcProp)
	{
	  pcProp->Empty ();
          templateEditor->Fill (pcProp, pctpl);
	}
	break;
      }
    }
  }
  else
  {
    detailGrid->Clear ();
    if (!tpl)
    {
      detailGrid->Thaw ();
      return;
    }

    s.Format ("Template (%s)", tpl->GetName ());
    wxPGProperty* templateProp = detailGrid->Append (new wxPropertyCategory (wxString::FromUTF8 (s)));
    templateEditor->Fill (templateProp, 0);
  }

  detailGrid->FitColumns ();
  detailGrid->Thaw ();
}

// -----------------------------------------------------------------------

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
  contextMenuNode = "";
  questsValue->Refresh ();
}

void EntityMode::Stop ()
{
  EditingMode::Stop ();
  graphView->SetVisible (false);
}

const char* EntityMode::GetSeqOpType (iSeqOpFactory* seqop)
{
  if (!seqop) return "Delay";

  {
    csRef<iDebugPrintSeqOpFactory> s = scfQueryInterface<iDebugPrintSeqOpFactory> (seqop);
    if (s) return "DebugPrint";
  }
  {
    csRef<iAmbientMeshSeqOpFactory> s = scfQueryInterface<iAmbientMeshSeqOpFactory> (seqop);
    if (s) return "AmbientMesh";
  }
  {
    csRef<iLightSeqOpFactory> s = scfQueryInterface<iLightSeqOpFactory> (seqop);
    if (s) return "Light";
  }
  {
    csRef<iMovePathSeqOpFactory> s = scfQueryInterface<iMovePathSeqOpFactory> (seqop);
    if (s) return "MovePath";
  }
  {
    csRef<iTransformSeqOpFactory> s = scfQueryInterface<iTransformSeqOpFactory> (seqop);
    if (s) return "Transform";
  }
  {
    csRef<iPropertySeqOpFactory> s = scfQueryInterface<iPropertySeqOpFactory> (seqop);
    if (s) return "Property";
  }
  return "?";
}

const char* EntityMode::GetTriggerType (iTriggerFactory* trigger)
{
  {
    csRef<iTimeoutTriggerFactory> s = scfQueryInterface<iTimeoutTriggerFactory> (trigger);
    if (s) return "Timeout";
  }
  {
    csRef<iEnterSectorTriggerFactory> s = scfQueryInterface<iEnterSectorTriggerFactory> (trigger);
    if (s) return "EnterSector";
  }
  {
    csRef<iSequenceFinishTriggerFactory> s = scfQueryInterface<iSequenceFinishTriggerFactory> (trigger);
    if (s) return "SequenceFinish";
  }
  {
    csRef<iPropertyChangeTriggerFactory> s = scfQueryInterface<iPropertyChangeTriggerFactory> (trigger);
    if (s) return "PropertyChange";
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
    if (s) return "MeshSelect";
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
    if (s) return "DebugPrint";
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
    if (s) return "SequenceFinish";
  }
  {
    csRef<iChangePropertyRewardFactory> s = scfQueryInterface<iChangePropertyRewardFactory> (reward);
    if (s) return "ChangeProperty";
  }
  {
    csRef<iCreateEntityRewardFactory> s = scfQueryInterface<iCreateEntityRewardFactory> (reward);
    if (s) return "CreateEntity";
  }
  {
    csRef<iDestroyEntityRewardFactory> s = scfQueryInterface<iDestroyEntityRewardFactory> (reward);
    if (s) return "DestroyEntity";
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

iQuestFactory* EntityMode::GetQuestFactory (iCelPropertyClassTemplate* pctpl)
{
  csString questName = GetQuestName (pctpl);
  if (questName.IsEmpty ()) return 0;

  csRef<iQuestManager> quest_mgr = csQueryRegistryOrLoad<iQuestManager> (
    GetObjectRegistry (), "cel.manager.quests");
  iQuestFactory* questFact = quest_mgr->GetQuestFactory (questName);
  return questFact;
}

csString EntityMode::GetQuestName (iCelPropertyClassTemplate* pctpl)
{
  if (editQuestMode)
    return editQuestMode->GetName ();
  else if (pctpl)
    return InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
  else if (GetContextMenuNode ().IsEmpty ())
    return "";
  else
    return GetQuestName (GetPCTemplate (GetContextMenuNode ()));
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
  iQuestFactory* questFact = GetQuestFactory (pctpl);
  if (!questFact) return;

  csString defaultState = InspectTools::GetPropertyValueString (pl, pctpl, "state");
  BuildQuestGraph (questFact, pcKey, false, defaultState);
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
  RefreshGrid ();
  delayedRefreshType = REFRESH_NOCHANGE;
  refreshPctpl = 0;
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
  }
  else
  {
    BuildTemplateGraph (currentTemplate);
    if (pctpl) SelectPC (pctpl);
  }
}

void EntityMode::RefreshGrid (iCelPropertyClassTemplate* pctpl)
{
  if (!started) return;
  if (editQuestMode)
  {
printf ("FillDetailGrid\n"); fflush (stdout);
    FillDetailGrid (editQuestMode);
  }
  else
  {
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    FillDetailGrid (tpl, pctpl);
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
  if (!key) return 0;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (key);
  return GetQuestFactory (pctpl);
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

iCelEntityTemplate* EntityMode::GetCurrentTemplate ()
{
  return pl->FindEntityTemplate (currentTemplate);
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
  iQuestFactory* selectedQuest = questMgr->GetQuestFactory (questName);
  if (editQuestMode != selectedQuest)
  {
    editQuestMode = selectedQuest;
    currentTemplate = "";
    RefreshView ();
    RefreshGrid ();
  }
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
    RefreshGrid ();
    ActivateNode (0);
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
    app->SetObjectForComment ("template", tpl->QueryObject ());
  }
}

// @@@ Check if needed?
void EntityMode::RegisterModification (iCelEntityTemplate* tpl)
{
  if (!tpl)
    tpl = pl->FindEntityTemplate (currentTemplate);
  view3d->GetApplication ()->RegisterModification (tpl->QueryObject ());
  view3d->GetModelRepository ()->GetTemplatesValue ()->Refresh ();
  RefreshView ();
  RefreshGrid ();
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
  RefreshGrid ();
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
    RefreshGrid ();
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
	    InspectTools::AddActionParameter (pl, view3d->GetPM (), pctpl, "NewQuest", "name",
		CEL_DATA_STRING, name->GetData ());
	    RegisterModification (tpl);
	  }
        }
      }
    }

    questsValue->Refresh ();
    SelectQuest (questFact);
    RefreshView ();
    RefreshGrid ();
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
    RefreshGrid ();
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

void EntityMode::OnDeletePC ()
{
  csString selectedPropName, pcPropName;
  iCelPropertyClassTemplate* pctpl = templateEditor->GetPCForProperty (contextLastProperty, pcPropName, selectedPropName);
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  tpl->RemovePropertyClassTemplate (pctpl);
  RegisterModification (tpl);
  editQuestMode = 0;
  RefreshView ();
  RefreshGrid ();
}

void EntityMode::OnDelete ()
{
  if (editQuestMode && contextLastProperty)
  {
    if (questEditor->DeleteFromContext (contextLastProperty, editQuestMode))
    {
      RegisterModification (editQuestMode);
      RefreshView ();
      RefreshGrid ();
    }
  }
  else
  {
    if (GetContextMenuNode ().IsEmpty ()) return;
    DeleteItem (contextMenuNode);
  }
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
      RefreshGrid ();
    }
  }
  else if (type == 'S')
  {
    // Delete state.
    csString state = GetSelectedStateName (item);
    iQuestFactory* questFact = GetSelectedQuest (item);
    questFact->RemoveState (state);
    // @@@ Too much refresh!
    RegisterModification (questFact);
    RefreshView ();
    RefreshGrid ();
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
    RefreshGrid ();
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
    RefreshGrid ();
  }
  else if (type == 's')
  {
    // Delete sequence.
    iCelSequenceFactory* sequence = GetSelectedSequence (item);
    iQuestFactory* questFact = GetSelectedQuest (item);
    questFact->RemoveSequence (sequence->GetName ());
    RegisterModification (questFact);
    RefreshView ();
    RefreshGrid ();
  }
}

void EntityMode::OnCreatePC ()
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New PropertyClass",
      "LName:;Cname,pcobject.mesh,pctools.properties,pctools.inventory,pclogic.quest,pclogic.spawn,pclogic.trigger,pclogic.wire,pctools.messenger,pcinput.standard,pcphysics.object,pcphysics.system,pccamera.old,pcmove.actor.dynamic,pcmove.actor.standard,pcmove.actor.wasd,pcworld.dynamic,ares.gamecontrol\nLTag:;Ttag");
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
      RefreshGrid (pc);
    }
  }
}

void EntityMode::QuestWasEdited (iQuestStateFactory* stateFact, iCelSequenceFactory* seqFact,
      RefreshType refreshType)
{
  RefreshView ();
  switch (refreshType)
  {
    case REFRESH_NOCHANGE:
    case REFRESH_NO:
      break;
    case REFRESH_STATE:		// @@@ Todo: implement
    case REFRESH_SEQUENCE:
    case REFRESH_FULL:
      RefreshGrid ();
      break;
    default:
      break;			// Other types can't happen here.
  }
  view3d->GetApplication ()->RegisterModification (editQuestMode->QueryObject ());
  questsValue->Refresh ();
}

void EntityMode::PCWasEdited (iCelPropertyClassTemplate* pctpl, RefreshType refreshType)
{
  RefreshView (pctpl);
  switch (refreshType)
  {
    case REFRESH_NOCHANGE:
    case REFRESH_NO:
      break;
    case REFRESH_PC:
      RefreshGrid (pctpl);
      break;
    case REFRESH_TEMPLATE:	// @@@ Todo: implement
    case REFRESH_FULL:
      RefreshGrid ();
      break;
    default:
      break;			// Other types can't happen here.
  }
  iCelEntityTemplate* tpl = pl->FindEntityTemplate (currentTemplate);
  view3d->GetApplication ()->RegisterModification (tpl->QueryObject ());
  view3d->GetModelRepository ()->GetTemplatesValue ()->Refresh ();
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
    RefreshGrid ();
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
  activeNode = nodeName;
  app->SetMenuState ();
  printf ("ActivateNode %s\n", nodeName); fflush (stdout);
}

void EntityMode::OnSeqOpMove (int dir)
{
  iCelSequenceFactory* seqFact = questEditor->GetSequenceForProperty (contextLastProperty);
  if (!seqFact) return;

  size_t idx = questEditor->GetSeqOpForProperty (contextLastProperty);
  csRef<iSeqOpFactory> seqOpFact = seqFact->GetSeqOpFactory (idx);
  csString duration = seqFact->GetSeqOpFactoryDuration (idx);

  if (dir <= -1 && idx <= 0) return;
  if (dir >= 1 && idx >= seqFact->GetSeqOpFactoryCount ()-1) return;

  seqFact->RemoveSeqOpFactory (idx);

  // Make a copy of the seqops.
  csRefArray<iSeqOpFactory> seqops;
  csStringArray durations;
  while (seqFact->GetSeqOpFactoryCount () > 0)
  {
    seqops.Push (seqFact->GetSeqOpFactory (0));
    durations.Push (seqFact->GetSeqOpFactoryDuration (0));
    seqFact->RemoveSeqOpFactory (0);
  }

  int newindex = int (idx) + dir;
  if (newindex <= 0)
  {
    seqops.Insert (0, seqOpFact);
    durations.Insert (0, duration);
  }
  else if (newindex >= int (seqops.GetSize ()))
  {
    seqops.Push (seqOpFact);
    durations.Push (duration);
  }
  else
  {
    seqops.Insert (size_t (newindex), seqOpFact);
    durations.Insert (size_t (newindex), duration);
  }

  for (size_t i = 0 ; i < seqops.GetSize () ; i++)
    seqFact->AddSeqOpFactory (seqops.Get (i), durations.Get (i));

  iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
  RegisterModification (questFact);
  graphView->ActivateNode (0);
  ActivateNode (0);
  RefreshView ();
  RefreshGrid ();
}

void EntityMode::OnRewardMove (int dir)
{
  size_t idx;
  csRef<iRewardFactoryArray> array;

  if (editQuestMode && contextLastProperty)
    array = questEditor->GetRewardForProperty (contextLastProperty, idx);
  else if (!GetContextMenuNode ().IsEmpty ())
    array = GetSelectedReward (GetContextMenuNode (), idx);

  if (!array) return;

  if (dir <= -1 && idx <= 0) return;
  if (dir >= 1 && idx >= array->GetSize ()-1) return;

  csRef<iRewardFactory> rf = array->Get (idx);
  array->DeleteIndex (idx);
  int newindex = int (idx) + dir;
  if (newindex <= 0)
    array->Insert (0, rf);
  else if (newindex >= int (array->GetSize ()))
    array->Push (rf);
  else
    array->Insert (size_t (newindex), rf);

  iQuestFactory* questFact = GetSelectedQuest (GetContextMenuNode ());
  RegisterModification (questFact);
  graphView->ActivateNode (0);
  ActivateNode (0);
  RefreshView ();
  RefreshGrid ();
}

void EntityMode::OnCreateReward (int type)
{
  iQuestFactory* questFact;
  iQuestStateFactory* questState;
  iQuestTriggerResponseFactory* resp;
  if (!GetQuestContextInfo (questFact, questState, resp)) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Reward",
      "LName:;Cname,newstate,debugprint,action,changeproperty,createentity,destroyentity,changeclass,inventory,message,cssequence,sequence,sequencefinish");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    iRewardType* rewardType = questMgr->GetRewardType ("cel.rewards."+name);
    csRef<iRewardFactory> fact = rewardType->CreateRewardFactory ();
    if (type == 0) resp->AddRewardFactory (fact);
    else if (type == 1) questState->AddInitRewardFactory (fact);
    else questState->AddExitRewardFactory (fact);

    RegisterModification (questFact);
    RefreshView ();
    RefreshGrid ();
  }
}

bool EntityMode::GetQuestContextInfo (iQuestFactory*& questFact,
    iQuestStateFactory*& stateFact, iQuestTriggerResponseFactory*& resp)
{
  questFact = GetSelectedQuest (GetContextMenuNode ());
  if (!questFact) return false;

  if (editQuestMode && contextLastProperty)
  {
    int idx;
    stateFact = questEditor->GetStateForProperty (contextLastProperty, idx);
    csRef<iQuestTriggerResponseFactoryArray> responses = stateFact->GetTriggerResponseFactories ();
    if (idx >= 0)
      resp = responses->Get (idx);
    else
      resp = 0;
  }
  else if (GetContextMenuNode ().IsEmpty ())
    return false;
  else
  {
    csString state = GetSelectedStateName (GetContextMenuNode ());
    stateFact = questFact->GetState (state);
    resp = GetSelectedTriggerResponse (GetContextMenuNode ());
  }

  return stateFact != 0;
}

void EntityMode::Message_OnCreatePar ()
{
  if (questEditor->OnCreatePar (contextLastProperty))
  {
    // @@@ Smarter refresh?
    RegisterModification (editQuestMode);
    RefreshView ();
    RefreshGrid ();
  }
}

void EntityMode::Message_OnDeletePar ()
{
  if (questEditor->OnDeletePar (contextLastProperty))
  {
    // @@@ Smarter refresh?
    RegisterModification (editQuestMode);
    RefreshView ();
    RefreshGrid ();
  }
}

void EntityMode::OnCreateSeqOp ()
{
  iCelSequenceFactory* seqFact = questEditor->GetSequenceForProperty (contextLastProperty);
  if (!seqFact) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Sequence Operation",
      "LType:;CType,delay,debugprint,ambientmesh,light,movepath,transform\nLDuration:;TDuration");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString type = fields.Get ("Type", "");
    csString duration = fields.Get ("Duration", "");

    iSeqOpType* seqoptype = questMgr->GetSeqOpType ("cel.seqops."+type);
    csRef<iSeqOpFactory> seqopFact = seqoptype->CreateSeqOpFactory ();
    seqFact->AddSeqOpFactory (seqopFact, duration);

    RegisterModification (editQuestMode);
    RefreshView ();
    RefreshGrid ();
  }
}

void EntityMode::OnCreateTrigger ()
{
  iQuestFactory* questFact;
  iQuestStateFactory* questState;
  iQuestTriggerResponseFactory* resp;
  if (!GetQuestContextInfo (questFact, questState, resp)) return;

  iUIManager* ui = view3d->GetApplication ()->GetUI ();
  csRef<iUIDialog> dialog = ui->CreateDialog ("New Trigger",
      "LName:;Cname,entersector,meshentersector,inventory,meshselect,message,operation,propertychange,sequencefinish,timeout,trigger,watch");
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString name = fields.Get ("name", "");
    iTriggerType* triggertype = questMgr->GetTriggerType ("cel.triggers."+name);
    csRef<iTriggerFactory> triggerFact = triggertype->CreateTriggerFactory ();
    resp = questState->CreateTriggerResponseFactory ();
    resp->SetTriggerFactory (triggerFact);
    RegisterModification (questFact);
    RefreshView ();
    RefreshGrid ();
  }
}

void EntityMode::OnDefaultState ()
{
  if (GetContextMenuNode ().IsEmpty ()) return;
  iCelPropertyClassTemplate* pctpl = GetPCTemplate (GetContextMenuNode ());

  iQuestFactory* questFact = GetQuestFactory (pctpl);
  if (!questFact) return;

  csStringArray tokens (GetContextMenuNode (), ",");
  csString state = tokens[0];
  state = state.Slice (2);
  pctpl->RemoveProperty (pl->FetchStringID ("state"));
  pctpl->SetProperty (pl->FetchStringID ("state"), state.GetData ());

  RegisterModification ();

  RefreshView (pctpl);
  RefreshGrid (pctpl);
}

void EntityMode::OnNewSequence ()
{
  csString questName = GetQuestName (0);
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

  iCelPropertyClassTemplate* pctpl = GetSelectedPC ();
  RefreshView (pctpl);
  RefreshGrid (pctpl);
}

void EntityMode::OnNewState ()
{
  csString questName = GetQuestName (0);
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

  iCelPropertyClassTemplate* pctpl = GetSelectedPC ();
  RefreshView (pctpl);
  RefreshGrid (pctpl);
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
  RefreshGrid ();
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
  contextLastProperty = 0;
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

bool EntityMode::OnKeyboard (iEvent& ev, utf32_char code)
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

