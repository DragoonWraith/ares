/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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
#include "edcommon/transformtools.h"
#include "mainmode.h"
#include "inature.h"
#include "edcommon/uitools.h"
#include "edcommon/listctrltools.h"
#include "editor/i3dview.h"
#include "editor/iselection.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/iapp.h"
#include "editor/ipaster.h"
#include "editor/imodelrepository.h"
#include "iassetmanager.h"

#include "physicallayer/pl.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MainMode::Panel, wxPanel)
  EVT_CHECKBOX (XRCID("staticCheckBox"), MainMode::Panel::OnStaticSelected)
  EVT_TEXT_ENTER (XRCID("objectNameText"), MainMode::Panel::OnObjectNameEntered)
  EVT_TREE_SEL_CHANGED (XRCID("factoryTree"), MainMode::Panel::OnTreeSelChanged)
  EVT_LIST_ITEM_SELECTED (XRCID("objectList"), MainMode::Panel::OnListSelChanged)
  EVT_LIST_ITEM_RIGHT_CLICK (XRCID("objectList"), MainMode::Panel::OnListContext)
END_EVENT_TABLE()

SCF_IMPLEMENT_FACTORY (MainMode)

static csStringID ID_RotReset = csInvalidStringID;
static csStringID ID_RotLeft = csInvalidStringID;
static csStringID ID_RotRight = csInvalidStringID;
static csStringID ID_AlignObj = csInvalidStringID;
static csStringID ID_AlignRot = csInvalidStringID;
static csStringID ID_AlignHeight = csInvalidStringID;
static csStringID ID_Spread = csInvalidStringID;
static csStringID ID_SnapObj = csInvalidStringID;
static csStringID ID_StackObj = csInvalidStringID;
static csStringID ID_Copy = csInvalidStringID;
static csStringID ID_Paste = csInvalidStringID;
static csStringID ID_Delete = csInvalidStringID;
static csStringID ID_ToggleLabels = csInvalidStringID;

//---------------------------------------------------------------------------

class SpawnItemAction : public Ares::Action
{
private:
  MainMode* mainMode;

public:
  SpawnItemAction (MainMode* mainMode) : mainMode (mainMode) { }
  virtual ~SpawnItemAction () { }
  virtual const char* GetName () const { return "Spawn Item"; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    mainMode->SpawnSelectedItem ();
    return true;
  }
};

//---------------------------------------------------------------------------

class RemoveItemAction : public Ares::Action
{
private:
  MainMode* mainMode;

public:
  RemoveItemAction (MainMode* mainMode) : mainMode (mainMode) { }
  virtual ~RemoveItemAction () { }
  virtual const char* GetName () const { return "Remove Factory"; }
  virtual bool Do (Ares::View* view, wxWindow* component)
  {
    mainMode->RemoveSelectedFactory ();
    return true;
  }
};

//---------------------------------------------------------------------------

MainMode::MainMode (iBase* parent) : scfImplementationType (this, parent)
{
  name = "Main";
  do_dragging = false;
  do_kinematic_dragging = false;
  transformationMarker = 0;
  active = false;
  changing3DSelection = 0;
  labelMgr = 0;
  showLabels = true;
  panel = 0;
}

bool MainMode::Initialize (iObjectRegistry* object_reg)
{
  if (!ViewMode::Initialize (object_reg)) return false;

  csRef<iCelPlLayer> pl = csQueryRegistry<iCelPlLayer> (object_reg);
  ID_RotReset = pl->FetchStringID ("RotReset");
  ID_RotLeft = pl->FetchStringID ("RotLeft");
  ID_RotRight = pl->FetchStringID ("RotRight");
  ID_AlignObj = pl->FetchStringID ("AlignObj");
  ID_AlignRot = pl->FetchStringID ("AlignRot");
  ID_AlignHeight = pl->FetchStringID ("AlignHeight");
  ID_Spread = pl->FetchStringID ("Spread");
  ID_SnapObj = pl->FetchStringID ("SnapObj");
  ID_StackObj = pl->FetchStringID ("StackObj");
  ID_Copy = pl->FetchStringID ("Copy");
  ID_Paste = pl->FetchStringID ("Paste");
  ID_Delete = pl->FetchStringID ("Delete");
  ID_ToggleLabels = pl->FetchStringID ("ToggleLabels");

  labelMgr = new LabelManager (object_reg);

  colorTransp = g3d->GetDriver2D ()->FindRGB (100, 150, 80, 128);

  return true;
}

void MainMode::SetTopLevelParent (wxWindow* toplevel)
{
}

void MainMode::BuildMainPanel (wxWindow* parent)
{
  if (panel)
  {
    view.Reset ();
    parent->GetSizer ()->Remove (panel);
    delete panel;
  }
  panel = new Panel (this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("MainModePanel"));
  view.SetParent (panel);

  view.Bind (view3d->GetModelRepository ()->GetDynfactCollectionValue (), "factoryTree");
  wxTreeCtrl* factoryTree = XRCCTRL (*panel, "factoryTree", wxTreeCtrl);
  view.AddAction (factoryTree, NEWREF(Ares::Action, new SpawnItemAction (this)));
  view.AddAction (factoryTree, NEWREF(Ares::Action, new RemoveItemAction (this)));

  view.DefineHeadingIndexed ("objectList", "Factory,Template,Entity,Logic",
      DYNOBJ_COL_FACTORY, DYNOBJ_COL_TEMPLATE, DYNOBJ_COL_ENTITY,  DYNOBJ_COL_LOGIC);
  view.Bind (view3d->GetModelRepository ()->GetObjectsValue (), "objectList");
}

MainMode::~MainMode ()
{
  delete labelMgr;
}

void MainMode::CreateMarkers ()
{
  if (!transformationMarker)
  {
    transformationMarker = markerMgr->CreateMarker ();
    iMarkerColor* red = markerMgr->FindMarkerColor ("red");
    iMarkerColor* green = markerMgr->FindMarkerColor ("green");
    iMarkerColor* blue = markerMgr->FindMarkerColor ("blue");
    iMarkerColor* yellow = markerMgr->FindMarkerColor ("yellow");
    transformationMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (1,0,0), red, true);
    transformationMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,1,0), green, true);
    transformationMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,0,1), blue, true);

    iMarkerHitArea* hitArea = transformationMarker->HitArea (
	MARKER_OBJECT, csVector3 (0), .1f, 0, yellow);
    csRef<MarkerCallback> cb;
    cb.AttachNew (new MarkerCallback (this));
    hitArea->DefineDrag (0, 0, MARKER_WORLD, CONSTRAIN_NONE, cb);
    hitArea->DefineDrag (0, CSMASK_ALT, MARKER_WORLD, CONSTRAIN_YPLANE, cb);

    hitArea = transformationMarker->HitArea (
	MARKER_OBJECT, csVector3 (1,0,0), .07, 0, yellow);
    hitArea->DefineDrag (0, 0, MARKER_OBJECT, CONSTRAIN_ZPLANE+CONSTRAIN_ROTATEX, cb);
    hitArea->DefineDrag (0, CSMASK_SHIFT, MARKER_OBJECT, CONSTRAIN_YPLANE+CONSTRAIN_ROTATEX, cb);

    hitArea = transformationMarker->HitArea (
	MARKER_OBJECT, csVector3 (0,1,0), .07, 0, yellow);
    hitArea->DefineDrag (0, 0, MARKER_OBJECT, CONSTRAIN_XPLANE+CONSTRAIN_ROTATEY, cb);
    hitArea->DefineDrag (0, CSMASK_SHIFT, MARKER_OBJECT, CONSTRAIN_ZPLANE+CONSTRAIN_ROTATEY, cb);

    hitArea = transformationMarker->HitArea (
	MARKER_OBJECT, csVector3 (0,0,1), .07, 0, yellow);
    hitArea->DefineDrag (0, 0, MARKER_OBJECT, CONSTRAIN_YPLANE+CONSTRAIN_ROTATEZ, cb);
    hitArea->DefineDrag (0, CSMASK_SHIFT, MARKER_OBJECT, CONSTRAIN_XPLANE+CONSTRAIN_ROTATEZ, cb);
    transformationMarker->SetVisible (false);
  }
}

void MainMode::Start ()
{
  ViewMode::Start ();
  CreateMarkers ();
  SetTransformationMarkerStatus ();
  Refresh ();
  active = true;
  app->SetMenuItemState ("ToggleLabels", showLabels);
}

void MainMode::Stop ()
{
  ViewMode::Stop ();
  transformationMarker->SetVisible (false);
  transformationMarker->AttachMesh (0);
  //pasteMarker->SetVisible (false);
  active = false;
  labelMgr->Cleanup ();
}

void MainMode::OnListContext (wxListEvent& event)
{
  wxListCtrl* list = XRCCTRL (*panel, "objectList", wxListCtrl);
  wxMenu contextMenu;
  AddContextMenu (&contextMenu, 0, 0);
  list->PopupMenu (&contextMenu);
}

void MainMode::AllocContextHandlers (wxFrame* frame)
{
  iUIManager* ui = view3d->GetApplication ()->GetUI ();

  idSetStatic = ui->AllocContextMenuID ();
  frame->Connect (idSetStatic, wxEVT_COMMAND_MENU_SELECTED,
	      wxCommandEventHandler (MainMode::Panel::OnSetStatic), 0, panel);
  idClearStatic = ui->AllocContextMenuID ();
  frame->Connect (idClearStatic, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (MainMode::Panel::OnClearStatic), 0, panel);
}

void MainMode::AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY)
{
  if (view3d->GetSelection ()->HasSelection ())
  {
    contextMenu->AppendSeparator ();
    contextMenu->Append (wxID_DELETE, wxT ("&Delete"));
    contextMenu->AppendSeparator ();

    contextMenu->Append (idSetStatic, wxT ("Set static"));
    contextMenu->Append (idClearStatic, wxT ("Clear static"));
  }
}

void MainMode::Refresh ()
{
  labelMgr->Cleanup ();
  view3d->GetModelRepository ()->GetDynfactCollectionValue ()->Refresh ();
  if (started)
  {
    wxTreeCtrl* tree = XRCCTRL (*panel, "factoryTree", wxTreeCtrl);
    wxTreeItemId rootId = tree->GetRootItem ();
    tree->SelectItem (rootId);
    tree->Expand (rootId);
    app->SetMenuItemState ("ToggleLabels", showLabels);
  }
}

void MainMode::CurrentObjectsChanged (const csArray<iDynamicObject*>& current)
{
  wxTextCtrl* nameText = XRCCTRL (*panel, "objectNameText", wxTextCtrl);
  wxCheckBox* staticCheck = XRCCTRL (*panel, "staticCheckBox", wxCheckBox);
  if (current.GetSize () > 1)
  {
    staticCheck->Disable ();
    nameText->Disable ();
    UITools::SetValue (panel, "objectNameText", "");
    UITools::SetValue (panel, "factoryNameLabel", "...");
  }
  else if (current.GetSize () == 1)
  {
    staticCheck->Enable ();
    staticCheck->SetValue (current[0]->IsStatic ());
    nameText->Enable ();
    UITools::SetValue (panel, "objectNameText", current[0]->GetEntityName ());
    UITools::SetValue (panel, "factoryNameLabel", current[0]->GetFactory ()->GetName ());
  }
  else
  {
    staticCheck->Disable ();
    staticCheck->SetValue (false);
    nameText->Disable ();
    UITools::SetValue (panel, "objectNameText", "");
    UITools::SetValue (panel, "factoryNameLabel", "");
  }

  if (changing3DSelection <= 0)
  {
    changing3DSelection++;
    wxListCtrl* list = XRCCTRL (*panel, "objectList", wxListCtrl);
    ListCtrlTools::ClearSelection (list, false);
    for (size_t i = 0 ; i < current.GetSize () ; i++)
    {
      size_t index = view3d->GetModelRepository ()->GetDynamicObjectIndexFromObjects (current[i]);
      if (index != csArrayItemNotFound)
        ListCtrlTools::SelectRow (list, (int)index, false, true);
    }
    changing3DSelection--;
  }

  SetTransformationMarkerStatus ();
}

void MainMode::SetTransformationMarkerStatus ()
{
  if (do_kinematic_dragging)
  {
    transformationMarker->SetVisible (false);
    return;
  }
  if (view3d->GetSelection ()->GetSize () >= 1)
  {
    transformationMarker->SetVisible (true);
    transformationMarker->AttachMesh (view3d->GetSelection ()->GetFirst ()->GetMesh ());
  }
  else
    transformationMarker->SetVisible (false);
}

bool MainMode::Command (csStringID id, const csString& args, bool checked)
{
  if (id == ID_RotReset) TransformTools::RotResetSelectedObjects (view3d->GetSelection ());
  else if (id == ID_RotLeft) TransformTools::Rotate (view3d->GetSelection (), PI/2.0);
  else if (id == ID_RotRight) TransformTools::Rotate (view3d->GetSelection (), -PI/2.0);
  else if (id == ID_AlignObj) TransformTools::SetPosSelectedObjects (view3d->GetSelection ());
  else if (id == ID_AlignRot) TransformTools::AlignSelectedObjects (view3d->GetSelection ());
  else if (id == ID_AlignHeight) TransformTools::SameYSelectedObjects (view3d->GetSelection ());
  else if (id == ID_Spread) TransformTools::SpreadSelectedObjects (view3d->GetSelection ());
  else if (id == ID_SnapObj) OnSnapObjects ();
  else if (id == ID_StackObj) TransformTools::StackSelectedObjects (view3d->GetSelection ());
  else if (id == ID_Copy) view3d->GetPaster ()->CopySelection ();
  else if (id == ID_Paste) view3d->GetPaster ()->StartPasteSelection ();
  else if (id == ID_Delete) view3d->DeleteSelectedObjects ();
  else if (id == ID_ToggleLabels) ToggleLabels (checked);
  else return false;
  return true;
}

bool MainMode::IsCommandValid (csStringID id, const csString& args,
      iSelection* selection, size_t pastesize)
{
  if (!active) return false;
  int cnt = selection->GetSize ();
  if (id == ID_AlignObj) return cnt > 1;
  if (id == ID_AlignRot) return cnt > 1;
  if (id == ID_AlignHeight) return cnt > 1;
  if (id == ID_Spread) return cnt > 2;
  if (id == ID_SnapObj) return cnt > 1;
  if (id == ID_StackObj) return cnt > 1;
  if (id == ID_Copy) return cnt > 0;
  if (id == ID_Paste) return pastesize > 0;
  if (id == ID_Delete) return cnt > 0;
  return true;
}

csPtr<iString> MainMode::GetAlternativeLabel (csStringID id,
      iSelection* selection, size_t pastesize)
{
  if (id == ID_Paste)
  {
    if (pastesize == 0) return new scfString ("Paste\tCtrl+V");
    else
    {
      scfString* label = new scfString ();
      label->Format ("Paste %d objects\tCtrl+V", pastesize);
      return label;
    }
  }
  return 0;
}

void MainMode::OnStaticSelected ()
{
  wxCheckBox* staticCheck = XRCCTRL (*panel, "staticCheckBox", wxCheckBox);
  view3d->SetStaticSelectedObjects (staticCheck->IsChecked ());
}

void MainMode::OnObjectNameEntered ()
{
  csString n = UITools::GetValue (panel, "objectNameText");
  view3d->ChangeNameSelectedObject (n);
}

void MainMode::OnSetStatic ()
{
  view3d->SetStaticSelectedObjects (true);
  wxCheckBox* staticCheck = XRCCTRL (*panel, "staticCheckBox", wxCheckBox);
  staticCheck->SetValue (true);
}

void MainMode::OnClearStatic ()
{
  view3d->SetStaticSelectedObjects (false);
  wxCheckBox* staticCheck = XRCCTRL (*panel, "staticCheckBox", wxCheckBox);
  staticCheck->SetValue (false);
}

void MainMode::OnTreeSelChanged (wxTreeEvent& event)
{
  view3d->GetApplication ()->SetFocus3D ();
  csString selection = GetSelectedItem ();
  iObject* factObject = 0;
  if (!selection.IsEmpty ())
  {
    iDynamicFactory* fact = view3d->GetDynamicWorld ()->FindFactory (selection);
    if (fact)
      factObject = fact->QueryObject ();
  }
  view3d->GetApplication ()->SetObjectForComment ("dynamic factory", factObject);
}

void MainMode::OnListSelChanged (wxListEvent& event)
{
  if (changing3DSelection > 0) return;
  changing3DSelection++;
  iSelection* selection = view3d->GetSelection ();
  wxListCtrl* list = XRCCTRL (*panel, "objectList", wxListCtrl);
  csArray<Ares::Value*> values = view.GetSelectedValues (list);
  selection->SetCurrentObject (0);
  for (size_t i = 0 ; i < values.GetSize () ; i++)
  {
    iDynamicObject* dynobj = view3d->GetModelRepository ()->GetDynamicObjectFromObjects (values[i]);
    if (dynobj)
      selection->AddCurrentObject (dynobj);
  }
  view3d->GetApplication ()->SetFocus3D ();
  changing3DSelection--;
}

void MainMode::MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos, uint button, uint32 modifiers)
{
  //printf ("START: %g,%g,%g\n", pos.x, pos.y, pos.z); fflush (stdout);
  csRef<iSelectionIterator> it = view3d->GetSelection ()->GetIterator ();
  while (it->HasNext ())
  {
    iDynamicObject* dynobj = it->Next ();
    dynobj->RemovePivotJoints ();
    dynobj->MakeKinematic ();
    AresDragObject dob;
    dob.originalTransform = dynobj->GetTransform ();
    csVector3 meshpos = dob.originalTransform.GetOrigin ();
    dob.kineOffset = pos - meshpos;
    dob.dynobj = dynobj;
    dragObjects.Push (dob);
  }
}

void MainMode::SetDynObjOrigin (iDynamicObject* dynobj, const csVector3& pos)
{
  csReversibleTransform tr = dynobj->GetTransform ();
  tr.SetOrigin (pos);
  dynobj->SetTransform (tr);
  app->RegisterModification ();
}

void MainMode::SetDynObjTransform (iDynamicObject* dynobj, const csReversibleTransform& trans)
{
  dynobj->SetTransform (trans);
  app->RegisterModification ();
}

void MainMode::MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
{
  //printf ("MOVE: %g,%g,%g\n", pos.x, pos.y, pos.z); fflush (stdout);
  for (size_t i = 0 ; i < dragObjects.GetSize () ; i++)
  {
    csVector3 np = pos - dragObjects[i].kineOffset;
    SetDynObjOrigin (dragObjects[i].dynobj, np);
  }
}

void MainMode::MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform)
{
  for (size_t i = 0 ; i < dragObjects.GetSize () ; i++)
  {
    SetDynObjTransform (dragObjects[i].dynobj, transform);
  }
}

void MainMode::MarkerStopDragging (iMarker* marker, iMarkerHitArea* area)
{
  dragObjects.DeleteAll ();
  do_kinematic_dragging = false;
  csRef<iSelectionIterator> it = view3d->GetSelection ()->GetIterator ();
  while (it->HasNext ())
  {
    iDynamicObject* dynobj = it->Next ();
    dynobj->UndoKinematic ();
    dynobj->RecreatePivotJoints ();
  }
}

void MainMode::StopDrag (bool cancel)
{
  if (do_dragging)
  {
    do_dragging = false;

    // Put back the original dampening on the rigid body
    view3d->GetDynamicSystem ()->RemoveJoint (dragJoint);
#if 0
    // @@@
    csBody->SetLinearDampener (linearDampening);
    csBody->SetRollingDampener (angularDampening);

    // Remove the drag joint
    view3d->GetBulletSystem ()->RemovePivotJoint (dragJoint);
#endif
    dragJoint = 0;
  }
  if (do_kinematic_dragging)
  {
    do_kinematic_dragging = false;
    size_t i = 0;
    csRef<iSelectionIterator> it = view3d->GetSelection ()->GetIterator ();
    while (it->HasNext ())
    {
      iDynamicObject* dynobj = it->Next ();
      if (cancel)
      {
    	AresDragObject& dob = dragObjects[i];
	dynobj->SetTransform (dob.originalTransform);
	app->RegisterModification ();
      }
      dynobj->UndoKinematic ();
      dynobj->RecreatePivotJoints ();
      if (kinematicFirstOnly) break;
      i++;
    }
  }
  dragObjects.DeleteAll ();
  view3d->GetApplication ()->ClearStatus ();
  view3d->GetPaster ()->HideConstrainMarker ();
  SetTransformationMarkerStatus ();
}

void MainMode::HandleKinematicDragging ()
{
  iCamera* camera = view3d->GetCsCamera ();
  const csReversibleTransform& meshtrans = dragObjects[0].dynobj->GetTransform ();

  csSegment3 beam = view3d->GetMouseBeam (1000.0f);
  csVector3 dr;
  if (doDragLocal)
  {
    beam.SetStart (meshtrans.Other2This (beam.Start ()));
    beam.SetEnd (meshtrans.Other2This (beam.End ()));
    dr = dragRestrictLocal;
  }
  else
  {
    dr = dragRestrict;
  }

  csVector3 newPosition;
  if (doDragRestrict[CS_AXIS_Y])
  {
    if (fabs (beam.Start ().y-beam.End ().y) < 0.1f) return;
    if (beam.End ().y < beam.Start ().y && dr.y > beam.Start ().y) return;
    if (beam.End ().y > beam.Start ().y && dr.y < beam.Start ().y) return;
    float dist = csIntersect3::SegmentYPlane (beam, dr.y, newPosition);
    if (dist > 0.08f)
    {
      newPosition = beam.Start () + (beam.End ()-beam.Start ()).Unit () * 80.0f;
      newPosition.y = dr.y;
    }
    if (doDragRestrict[CS_AXIS_X])
      newPosition.x = dr.x;
    if (doDragRestrict[CS_AXIS_Z])
      newPosition.z = dr.z;
  }
  else if (doDragRestrict[CS_AXIS_Z])
  {
    if (fabs (beam.Start ().z-beam.End ().z) < 0.1f) return;
    if (beam.End ().z < beam.Start ().z && dr.z > beam.Start ().z) return;
    if (beam.End ().z > beam.Start ().z && dr.z < beam.Start ().z) return;
    float dist = csIntersect3::SegmentZPlane (beam, dr.z, newPosition);
    if (dist > 0.08f)
    {
      newPosition = beam.Start () + (beam.End ()-beam.Start ()).Unit () * 80.0f;
      newPosition.z = dr.z;
    }
    if (doDragRestrict[CS_AXIS_X])
      newPosition.x = dr.x;
  }
  else if (doDragRestrict[CS_AXIS_X])
  {
    if (fabs (beam.Start ().x-beam.End ().x) < 0.1f) return;
    //if (beam.End ().x < beam.Start ().x && dr.x > beam.Start ().x) return;
    //if (beam.End ().x > beam.Start ().x && dr.x < beam.Start ().x) return;
    float dist = csIntersect3::SegmentXPlane (beam, dr.x, newPosition);
    if (dist > 0.08f)
    {
      newPosition = beam.Start () + (beam.End ()-beam.Start ()).Unit () * 80.0f;
      newPosition.x = dr.x;
    }
  }
  else
  {
    csPlane3 zplane (0, 0, -1, dragDistance);
    zplane = camera->GetTransform ().This2Other (zplane);
    csIntersect3::SegmentPlane (zplane, beam);
    newPosition = beam.Start ();
  }

  if (view3d->GetPaster ()->IsGridModeEnabled ())
  {
    float gridSize = view3d->GetPaster ()->GetGridSize ();
    float m;
    m = fmod (newPosition.x, gridSize);
    newPosition.x -= m;
    m = fmod (newPosition.y, gridSize);
    newPosition.y -= m;
    m = fmod (newPosition.z, gridSize);
    newPosition.z -= m;
  }

  if (doDragLocal)
  {
    newPosition = meshtrans.This2Other (newPosition);
  }

  for (size_t i = 0 ; i < dragObjects.GetSize () ; i++)
  {
    csVector3 np = newPosition - dragObjects[i].kineOffset;
    SetDynObjOrigin (dragObjects[i].dynobj, np);
    if (kinematicFirstOnly) break;
  }

  if (doDragLocal)
  {
    view3d->GetPaster ()->MoveConstrainMarker (meshtrans);
  }
  else
  {
    csReversibleTransform tr;
    tr.SetOrigin (meshtrans.GetOrigin ());
    view3d->GetPaster ()->MoveConstrainMarker (tr);
  }
}

void MainMode::HandlePhysicalDragging ()
{
  // Keep the drag joint at the same distance to the camera
  iCamera* camera = view3d->GetCsCamera ();
  csSegment3 beam = view3d->GetMouseBeam ();
  csVector3 newPosition = beam.End () - beam.Start ();
  newPosition.Normalize ();
  newPosition = camera->GetTransform ().GetOrigin () + newPosition * dragDistance;
  dragJoint->SetPosition (newPosition);
}

void MainMode::FramePre()
{
  ViewMode::FramePre ();
  if (do_dragging)
  {
    HandlePhysicalDragging ();
  }
  else if (do_kinematic_dragging)
  {
    HandleKinematicDragging ();
  }
  if (showLabels)
    labelMgr->FramePre (view3d->GetDynamicWorld (), view3d->GetCsCamera ());
}

void MainMode::Frame3D()
{
  ViewMode::Frame3D ();
}

void MainMode::WriteLine (int x, int& y, const char* txt)
{
  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->Write (font, g2d->GetWidth ()-x, y, colorWhite, colorTransp, txt);
  y += font->GetTextHeight ();
}

void MainMode::Frame2D()
{
  ViewMode::Frame2D ();
  int y = 20;
  int dx = 340;

  iSelection* sel = view3d->GetSelection ();

  if (do_dragging || do_kinematic_dragging)
  {
    WriteLine (dx, y, "LMB: place object and stop dragging     ");
    WriteLine (dx, y, "RMB: cancel dragging                    ");
  }
  else
  {
    WriteLine (dx, y, "LMB (marker): start kinematic drag      ");
    if (sel->GetSize () == 0)
      WriteLine (dx, y, "LMB: select object                      ");
    else
      WriteLine (dx, y, "LMB: (de)select object                  ");
    WriteLine (dx, y, "Shift+LMB: add/remove to selection      ");
    WriteLine (dx, y, "Ctrl+LMB: start kinematic drag          ");
    WriteLine (dx, y, "Alt+LMB: start kinematic drag (y-plane) ");
    WriteLine (dx, y, "LMB: start physics drag                 ");
  }

  if (view3d->GetPaster ()->IsPasteSelectionActive ())
  {
    WriteLine (dx, y, "q: toggle grid mode                     ");
    int mode = view3d->GetPaster ()->GetPasteConstrain ();
    if (mode == CONSTRAIN_XPLANE)
      WriteLine (dx, y, "x/X: disable x-axis constraint          ");
    else
      WriteLine (dx, y, "x/X: set constraint on x-axis           ");
    if (mode == CONSTRAIN_YPLANE)
      WriteLine (dx, y, "y/Y: disable y-axis constraint          ");
    else
      WriteLine (dx, y, "y/Y: set constraint on y-axis           ");
    if (mode == CONSTRAIN_ZPLANE)
      WriteLine (dx, y, "z/Z: disable z-axis constraint          ");
    else
      WriteLine (dx, y, "z/Z: set constraint on Z-axis           ");
  }
  else if (do_kinematic_dragging)
  {
    WriteLine (dx, y, "q: toggle grid mode                     ");
    bool drx = doDragRestrict[CS_AXIS_X];
    bool dry = doDragRestrict[CS_AXIS_Y];
    bool drz = doDragRestrict[CS_AXIS_Z];
    bool local = doDragLocal;
    if (drx || dry || drz)
    {
      if (!drx && dry && drz && !local)
        WriteLine (dx, y, "x: switch to local constraint           ");
      else
        WriteLine (dx, y, "x: disable constraint on x-axis/plane   ");

      if (!dry && drx && drz && !local)
        WriteLine (dx, y, "y: switch to local constraint           ");
      else
        WriteLine (dx, y, "y: disable constraint on y-axis/plane   ");

      if (!drz && drx && dry && !local)
        WriteLine (dx, y, "z: switch to local constraint           ");
      else
        WriteLine (dx, y, "z: disable constraint on z-axis/plane   ");

      if (drx && !dry && !drz && !local)
        WriteLine (dx, y, "X: switch to local constraint           ");
      else
        WriteLine (dx, y, "X: disable constraint on x-axis/plane   ");

      if (dry && !drx && !drz && !local)
        WriteLine (dx, y, "Y: switch to local constraint           ");
      else
        WriteLine (dx, y, "Y: disable constraint on y-axis/plane   ");

      if (drz && !drx && !dry && !local)
        WriteLine (dx, y, "Z: switch to local constraint           ");
      else
        WriteLine (dx, y, "Z: disable constraint on z-axis/plane   ");
    }
    else
    {
      WriteLine (dx, y, "x: enable constraint on x-axis (global) ");
      WriteLine (dx, y, "y: enable constraint on y-axis (global) ");
      WriteLine (dx, y, "z: enable constraint on z-axis (global) ");
      WriteLine (dx, y, "X: enable constraint on x-plane (global)");
      WriteLine (dx, y, "Y: enable constraint on y-plane (global)");
      WriteLine (dx, y, "Z: enable constraint on z-plane (global)");
    }
  }

  if (sel->HasSelection () && !do_dragging)
  {
    if (do_kinematic_dragging)
      WriteLine (dx, y, "g: snap object to cursor                ");
    else
      WriteLine (dx, y, "g: enable grabbing                      ");
  }
}

static void CorrectFactoryName (csString& name)
{
  if (name[name.Length ()-1] == '*')
    name = name.Slice (0, name.Length ()-1);
}

csString MainMode::GetSelectedItem ()
{
  wxTreeCtrl* tree = XRCCTRL (*panel, "factoryTree", wxTreeCtrl);
  wxTreeItemId id = tree->GetSelection ();
  if (!id.IsOk ()) return csString ();
  csString selection ((const char*)tree->GetItemText (id).mb_str (wxConvUTF8));
  CorrectFactoryName (selection);
  return selection;
}

void MainMode::RemoveSelectedFactory ()
{
  csString itemName = GetSelectedItem ();
  if (!itemName.IsEmpty ())
  {
    iDynamicFactory* fact = view3d->GetDynamicWorld ()->FindFactory (itemName);
    if (fact)
    {
      iUIManager* ui = view3d->GetApplication ()->GetUI ();
      if (view3d->GetApplication ()->GetAssetManager ()->IsLocked (fact->QueryObject ()))
	if (!ui->Ask ("Are you sure you want to remove a locked factory?"))
	  return;
      if (fact->GetObjectCount () > 0)
      {
	ui->Message ("There are %d objects using this factory. Delete those objects first!",
	    fact->GetObjectCount ());
	return;
      }
      csSet<csPtrKey<iObject> > resources;
      resources.Add (fact->QueryObject ());
      view3d->RemoveResources (resources);
    }
  }
}

void MainMode::SpawnSelectedItem ()
{
  csString itemName = GetSelectedItem ();
  if (!itemName.IsEmpty ())
    view3d->GetPaster ()->StartPasteSelection (itemName);
}

void MainMode::ToggleKinematicMode (int axis, bool shift)
{
  bool& drMain = doDragRestrict[axis];
  bool& drOther1 = doDragRestrict[(axis+1)%3];
  bool& drOther2 = doDragRestrict[(axis+2)%3];
  if (doDragRestrict[CS_AXIS_X] || doDragRestrict[CS_AXIS_Y] || doDragRestrict[CS_AXIS_Z])
  {
    if (shift && drMain && !drOther1 && !drOther2 && !doDragLocal)
      doDragLocal = true;
    else if (!drMain && drOther1 && drOther2 && !doDragLocal)
      doDragLocal = true;
    else
    {
      doDragLocal = false;
      drMain = drOther1 = drOther2 = false;
    }
  }
  else if (shift)
  {
    drMain = true;
    drOther1 = false;
    drOther2 = false;
    doDragLocal = false;
  }
  else
  {
    drMain = false;
    drOther1 = true;
    drOther2 = true;
    doDragLocal = false;
  }
  view3d->GetPaster ()->ShowConstrainMarker (
      doDragRestrict[CS_AXIS_X],
      doDragRestrict[CS_AXIS_Y],
      doDragRestrict[CS_AXIS_Z]);
}

void MainMode::ToggleLabels (bool checked)
{
  labelMgr->Cleanup ();
  showLabels = checked;
  app->SetMenuItemState ("ToggleLabels", showLabels);
}

bool MainMode::OnKeyboard (iEvent& ev, utf32_char code)
{
  if (ViewMode::OnKeyboard (ev, code))
    return true;

  bool ctrl = kbd->GetKeyState (CSKEY_CTRL);
  bool shift = kbd->GetKeyState (CSKEY_SHIFT);
  if (code == '2')
  {
    csRef<iSelectionIterator> it = view3d->GetSelection ()->GetIterator ();
    while (it->HasNext ())
    {
      iDynamicObject* dynobj = it->Next ();
      if (dynobj->IsStatic ())
	dynobj->MakeDynamic ();
      else
	dynobj->MakeStatic ();
    }
    app->RegisterModification ();
    CurrentObjectsChanged (view3d->GetSelection ()->GetObjects ());
  }
  else if (code == 'h')
  {
    if (holdJoint)
    {
      view3d->GetDynamicSystem ()->RemoveJoint (holdJoint);
      holdJoint = 0;
    }
    if (do_dragging)
    {
#if 0
      // @@@
      csRef<CS::Physics::Bullet::iRigidBody> csBody =
	scfQueryInterface<CS::Physics::Bullet::iRigidBody> (dragJoint->GetAttachedBody ());
      csBody->SetLinearDampener (linearDampening);
      csBody->SetRollingDampener (angularDampening);
#endif
      holdJoint = dragJoint;
      dragJoint = 0;
      do_dragging = false;
    }
  }
  else if (code == 'l')
  {
    ToggleLabels (!showLabels);
  }
  else if (code == 'e')
  {
    SpawnSelectedItem ();
  }
  else if (code == 'q')
  {
    if (view3d->GetPaster ()->IsPasteSelectionActive () || do_kinematic_dragging)
      view3d->GetPaster ()->ToggleGridMode ();
  }
  else if (code == 'g')
  {
    if (view3d->GetSelection ()->HasSelection ())
    {
      if (do_kinematic_dragging)
      {
	// We are already dragging. In that case we snap the items to the
	// place where the mouse is pointing and continue dragging.
	csVector3 offset = dragObjects[0].kineOffset;
        for (size_t i = 0 ; i < dragObjects.GetSize () ; i++)
	  dragObjects[i].kineOffset -= offset;
      }
      else
      {
        csSegment3 seg = view3d->GetMouseBeam (500);
        csVector3 isect (0);
        view3d->TraceBeamHit (seg, isect);

        StartKinematicDragging (false, seg, isect, false);
      }
    }
  }
  else if (code == 'x' || code == 'X')
  {
    if (view3d->GetPaster ()->IsPasteSelectionActive ())
    {
      int mode = view3d->GetPaster ()->GetPasteConstrain ();
      view3d->GetPaster ()->SetPasteConstrain (
	  mode == CONSTRAIN_XPLANE ? CONSTRAIN_NONE : CONSTRAIN_XPLANE);
    }
    else if (do_kinematic_dragging)
    {
      ToggleKinematicMode (CS_AXIS_X, shift);
    }
  }
  else if (code == 'y' || code == 'Y')
  {
    if (view3d->GetPaster ()->IsPasteSelectionActive ())
    {
      int mode = view3d->GetPaster ()->GetPasteConstrain ();
      view3d->GetPaster ()->SetPasteConstrain (
	  mode == CONSTRAIN_YPLANE ? CONSTRAIN_NONE : CONSTRAIN_YPLANE);
    }
    else if (do_kinematic_dragging)
    {
      ToggleKinematicMode (CS_AXIS_Y, shift);
    }
  }
  else if (code == 'z' || code == 'Z')
  {
    if (view3d->GetPaster ()->IsPasteSelectionActive ())
    {
      int mode = view3d->GetPaster ()->GetPasteConstrain ();
      view3d->GetPaster ()->SetPasteConstrain (
	  mode == CONSTRAIN_ZPLANE ? CONSTRAIN_NONE : CONSTRAIN_ZPLANE);
    }
    else if (do_kinematic_dragging)
    {
      ToggleKinematicMode (CS_AXIS_Z, shift);
    }
  }
  else if (code == CSKEY_UP)
  {
    TransformTools::Move (view3d->GetSelection (), csVector3 (0, 0, 1), ctrl, shift);
  }
  else if (code == CSKEY_DOWN)
  {
    TransformTools::Move (view3d->GetSelection (), csVector3 (0, 0, -1), ctrl, shift);
  }
  else if (code == CSKEY_LEFT)
  {
    TransformTools::Move (view3d->GetSelection (), csVector3 (-1, 0, 0), ctrl, shift);
  }
  else if (code == CSKEY_RIGHT)
  {
    TransformTools::Move (view3d->GetSelection (), csVector3 (1, 0, 0), ctrl, shift);
  }
  else if (code == '<' || code == ',')
  {
    TransformTools::Move (view3d->GetSelection (), csVector3 (0, -1, 0), ctrl, shift);
  }
  else if (code == '>' || code == '.')
  {
    TransformTools::Move (view3d->GetSelection (), csVector3 (0, 1, 0), ctrl, shift);
  }

  return false;
}

void MainMode::OnSnapObjects ()
{
  iSelection* selection = view3d->GetSelection ();
  if (selection->GetSize () < 2)
  {
    view3d->GetApplication ()->GetUI ()->Error ("Select at least two objects to snap together!");
    return;
  }
  const csArray<iDynamicObject*>& ob = selection->GetObjects ();
  for (size_t i = 1 ; i < ob.GetSize () ; i++)
  {
    ob[i]->MakeKinematic ();
    ob[i]->SetTransform (ob[0]->GetTransform ());
    ob[i]->UndoKinematic ();
  }
}

csRef<iString> MainMode::GetStatusLine ()
{
  csRef<iString> str = ViewMode::GetStatusLine ();
  if (do_kinematic_dragging)
    str->Append (", LMB: end drag. RMB: cancel drag. x/y/z to constrain placement. q for grid)");
  else
    str->Append (", LMB: select objects (shift to add to selection)");
  return str;
}

void MainMode::StartKinematicDragging (bool restrictY,
    const csSegment3& beam, const csVector3& isect, bool firstOnly)
{
  do_kinematic_dragging = true;
  view3d->GetApplication ()->ClearStatus ();
  kinematicFirstOnly = firstOnly;

  csRef<iSelectionIterator> it = view3d->GetSelection ()->GetIterator ();
  while (it->HasNext ())
  {
    iDynamicObject* dynobj = it->Next ();
    // First remove all pivot joints before starting to drag.
    dynobj->RemovePivotJoints ();
    dynobj->MakeKinematic ();
    AresDragObject dob;
    dob.originalTransform = dynobj->GetTransform ();
    csVector3 meshpos = dob.originalTransform.GetOrigin ();
    dob.kineOffset = isect - meshpos;
    dob.dynobj = dynobj;
    dragObjects.Push (dob);
    if (kinematicFirstOnly) break;
  }

  dragDistance = (isect - beam.Start ()).Norm ();
  dragRestrict = isect;
  const csReversibleTransform& meshtrans = dragObjects[0].dynobj->GetTransform ();
  dragRestrictLocal = meshtrans.Other2This (dragRestrict);

  doDragLocal = false;
  doDragRestrict[CS_AXIS_X] = false;
  doDragRestrict[CS_AXIS_Y] = restrictY;
  doDragRestrict[CS_AXIS_Z] = false;
  view3d->GetPaster ()->ShowConstrainMarker (
      doDragRestrict[CS_AXIS_X],
      doDragRestrict[CS_AXIS_Y],
      doDragRestrict[CS_AXIS_Z]);

  SetTransformationMarkerStatus ();
}

void MainMode::StartPhysicalDragging (CS::Physics::iRigidBody* hitBody,
    const csSegment3& beam, const csVector3& isect)
{
  // Create a pivot joint at the point clicked
  csRef<CS::Physics::iJointFactory> pivotJointFactory = view3d->GetPhysicalSystem ()->CreatePivotJointFactory ();
  dragJoint = pivotJointFactory->CreateJoint ();
  dragJoint->SetTransform (csOrthoTransform (csMatrix3 (), isect));
  dragJoint->Attach (hitBody, 0);
  view3d->GetDynamicSystem ()->AddJoint (dragJoint);

  do_dragging = true;
  dragDistance = (isect - beam.Start ()).Norm ();

  // Set some dampening on the rigid body to have a more stable dragging
#if 0
  // @@@
  csRef<CS::Physics::Bullet::iRigidBody> csBody =
        scfQueryInterface<CS::Physics::Bullet::iRigidBody> (hitBody);
  linearDampening = csBody->GetLinearDampener ();
  angularDampening = csBody->GetRollingDampener ();
  csBody->SetLinearDampener (0.9f);
  csBody->SetRollingDampener (0.9f);
#endif
}

void MainMode::AddForce (iRigidBody* hitBody, bool pull,
      const csSegment3& beam, const csVector3& isect)
{
  // Add a force at the point clicked
  csVector3 force = beam.End () - beam.Start ();
  force.Normalize ();
  if (pull)
    force *= -hitBody->GetMass ();
  else
    force *= hitBody->GetMass ();
  hitBody->AddForceAtPos (force, isect);
}

bool MainMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (do_dragging || do_kinematic_dragging)
  {
    if (but == csmbLeft)
    {
      StopDrag ();
      return true;
    }

    if (but == csmbRight)
    {
      StopDrag (true);
      return true;
    }
  }

  if (ViewMode::OnMouseDown (ev, but, mouseX, mouseY))
    return true;

  if (!(but == csmbLeft || but == csmbMiddle)) return false;

  if (mouseX > view3d->GetViewWidth ()) return false;
  if (mouseY > view3d->GetViewHeight ()) return false;

  uint32 mod = csMouseEventHelper::GetModifiers (&ev);
  bool shift = (mod & CSMASK_SHIFT) != 0;
  bool ctrl = (mod & CSMASK_CTRL) != 0;
  bool alt = (mod & CSMASK_ALT) != 0;

  int data;
  iMarker* hitMarker = markerMgr->FindHitMarker (mouseX, mouseY, data);
  if (hitMarker == transformationMarker)
  {
    iMeshWrapper* mesh = view3d->GetSelection ()->GetFirst ()->GetMesh ();
    iCamera* camera = view3d->GetCsCamera ();
    csVector3 isect = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    StartKinematicDragging (alt, csSegment3 (camera->GetTransform ().GetOrigin (),
	  isect), isect, true);
    return true;
  }

  // Compute the end beam points
  csVector3 isect;
  csSegment3 beam = view3d->GetBeam (mouseX, mouseY);
  iDynamicObject* newobj = view3d->TraceBeam (beam, isect);
  if (!newobj)
  {
    if (but == csmbLeft) view3d->GetSelection ()->SetCurrentObject (0);
    return false;
  }

  if (but == csmbLeft)
  {
    iSelection* sel = view3d->GetSelection ();
    if (shift)
      sel->AddCurrentObject (newobj);
    else
    {
      if (sel->GetSize () == 0)
      {
        sel->SetCurrentObject (newobj);
      }
      else if (!ctrl && !alt && !shift)
      {
        if (sel->GetSize () == 1 && sel->GetFirst () == newobj)
        {
	  // We are trying to select the same object. Deselect it instead.
          sel->SetCurrentObject (0);
        }
        else
        {
          sel->SetCurrentObject (newobj);
        }
      }
    }

    StopDrag ();

    if (ctrl || alt)
      StartKinematicDragging (alt, beam, isect, false);
    else if (!newobj->IsStatic () && newobj->GetBody ())
      StartPhysicalDragging (newobj->GetBody (), beam, isect);

    return true;
  }

  return false;
}

bool MainMode::OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (ViewMode::OnMouseUp (ev, but, mouseX, mouseY))
    return true;

  if (do_dragging || do_kinematic_dragging)
  {
    StopDrag ();
    return true;
  }

  return false;
}

bool MainMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return ViewMode::OnMouseMove (ev, mouseX, mouseY);
}


//---------------------------------------------------------------------------

