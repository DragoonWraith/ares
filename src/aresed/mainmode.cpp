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

#include "apparesed.h"
#include "mainmode.h"
#include "transformtools.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(MainMode::Panel, wxPanel)
  EVT_BUTTON (XRCID("rotLeftButton"), MainMode::Panel::OnRotLeft)
  EVT_BUTTON (XRCID("rotRightButton"), MainMode::Panel::OnRotRight)
  EVT_BUTTON (XRCID("rotResetButton"), MainMode::Panel::OnRotReset)
  EVT_BUTTON (XRCID("alignRotButton"), MainMode::Panel::OnAlignR)
  EVT_BUTTON (XRCID("setPosButton"), MainMode::Panel::OnSetPos)
  EVT_BUTTON (XRCID("stackButton"), MainMode::Panel::OnStack)
  EVT_BUTTON (XRCID("sameYButton"), MainMode::Panel::OnSameY)
  EVT_CHECKBOX (XRCID("staticCheckBox"), MainMode::Panel::OnStaticSelected)
  EVT_TREE_SEL_CHANGED (XRCID("factoryTree"), MainMode::Panel::OnTreeSelChanged)
END_EVENT_TABLE()

//---------------------------------------------------------------------------

MainMode::MainMode (wxWindow* parent, AresEdit3DView* aresed3d) :
  EditingMode (0, aresed3d, "Main")
{
  panel = new Panel (parent, this);
  parent->GetSizer ()->Add (panel, 1, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("MainModePanel"));
  do_dragging = false;
  do_kinematic_dragging = false;

  transformationMarker = 0;
}

void MainMode::Start ()
{
  if (!transformationMarker)
  {
    transformationMarker = aresed3d->GetMarkerManager ()->CreateMarker ();
    iMarkerColor* red = aresed3d->GetMarkerManager ()->FindMarkerColor ("red");
    iMarkerColor* green = aresed3d->GetMarkerManager ()->FindMarkerColor ("green");
    iMarkerColor* blue = aresed3d->GetMarkerManager ()->FindMarkerColor ("blue");
    iMarkerColor* yellow = aresed3d->GetMarkerManager ()->FindMarkerColor ("yellow");
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
  }

  if (aresed3d->GetSelection ()->GetSize () >= 1)
  {
    transformationMarker->SetVisible (true);
    transformationMarker->AttachMesh (aresed3d->GetSelection ()->GetFirst ()->GetMesh ());
  }
  else
    transformationMarker->SetVisible (false);
}

void MainMode::Stop ()
{
  transformationMarker->SetVisible (false);
  transformationMarker->AttachMesh (0);
}

void MainMode::SetupItems (const csHash<csStringArray,csString>& items)
{
  wxTreeCtrl* tree = XRCCTRL (*panel, "factoryTree", wxTreeCtrl);
  tree->DeleteAllItems ();
  wxTreeItemId rootId = tree->AddRoot (wxT ("Categories"));

  csHash<csStringArray,csString>::ConstGlobalIterator it = items.GetIterator ();
  while (it.HasNext ())
  {
    csString category;
    csStringArray items = it.Next (category);
    wxTreeItemId categoryId = tree->AppendItem (rootId, wxString (category, wxConvUTF8));
    for (size_t i = 0 ; i < items.GetSize () ; i++)
    {
      wxTreeItemId itemId = tree->AppendItem (categoryId, wxString (items[i], wxConvUTF8));
    }
  }

  tree->SelectItem (rootId);
  tree->Expand (rootId);
}

void MainMode::CurrentObjectsChanged (const csArray<iDynamicObject*>& current)
{
#if 0
  if (current.GetSize () > 1)
    staticCheck->disable ();
  else if (current.GetSize () == 1)
  {
    staticCheck->enable ();
    staticCheck->setSelected (current[0]->IsStatic ());
  }
  else
  {
    staticCheck->enable ();
    staticCheck->setSelected (false);
  }

  if (current.GetSize () >= 1)
  {
    transformationMarker->SetVisible (true);
    transformationMarker->AttachMesh (current[0]->GetMesh ());
  }
  else
    transformationMarker->SetVisible (false);
#endif
}

void MainMode::OnRotLeft ()
{
  bool slow = aresed3d->GetKeyboardDriver ()->GetKeyState (CSKEY_CTRL);
  bool fast = aresed3d->GetKeyboardDriver ()->GetKeyState (CSKEY_SHIFT);
  TransformTools::Rotate (aresed3d->GetSelection (), PI, slow, fast);
}

void MainMode::OnRotRight ()
{
  bool slow = aresed3d->GetKeyboardDriver ()->GetKeyState (CSKEY_CTRL);
  bool fast = aresed3d->GetKeyboardDriver ()->GetKeyState (CSKEY_SHIFT);
  TransformTools::Rotate (aresed3d->GetSelection (), -PI, slow, fast);
}

void MainMode::OnRotReset ()
{
  TransformTools::RotResetSelectedObjects (aresed3d->GetSelection ());
}

void MainMode::OnAlignR ()
{
  TransformTools::AlignSelectedObjects (aresed3d->GetSelection ());
}

void MainMode::OnStack ()
{
  TransformTools::StackSelectedObjects (aresed3d->GetSelection ());
}

void MainMode::OnSameY ()
{
  TransformTools::SameYSelectedObjects (aresed3d->GetSelection ());
}

void MainMode::OnSetPos ()
{
  TransformTools::SetPosSelectedObjects (aresed3d->GetSelection ());
}

void MainMode::OnStaticSelected ()
{
  //aresed3d->SetStaticSelectedObjects (staticCheck->isSelected ());
}

void MainMode::OnTreeSelChanged (wxTreeEvent& event)
{
}

void MainMode::MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos, uint button, uint32 modifiers)
{
  //printf ("START: %g,%g,%g\n", pos.x, pos.y, pos.z); fflush (stdout);
  SelectionIterator it = aresed3d->GetSelection ()->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dynobj->MakeKinematic ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    csVector3 meshpos = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    AresDragObject dob;
    dob.kineOffset = pos - meshpos;
    dob.dynobj = dynobj;
    dragObjects.Push (dob);
  }
}

void MainMode::MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
{
  //printf ("MOVE: %g,%g,%g\n", pos.x, pos.y, pos.z); fflush (stdout);
  for (size_t i = 0 ; i < dragObjects.GetSize () ; i++)
  {
    csVector3 np = pos - dragObjects[i].kineOffset;
    iMeshWrapper* mesh = dragObjects[i].dynobj->GetMesh ();
    if (mesh)
    {
      iMovable* mov = mesh->GetMovable ();
      mov->GetTransform ().SetOrigin (np);
      mov->UpdateMove ();
    }
  }
}

void MainMode::MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform)
{
  for (size_t i = 0 ; i < dragObjects.GetSize () ; i++)
  {
    iMeshWrapper* mesh = dragObjects[i].dynobj->GetMesh ();
    if (mesh)
    {
      iMovable* mov = mesh->GetMovable ();
      mov->SetTransform (transform);
      mov->UpdateMove ();
    }
  }
}

void MainMode::MarkerStopDragging (iMarker* marker, iMarkerHitArea* area)
{
  dragObjects.DeleteAll ();
  do_kinematic_dragging = false;
  SelectionIterator it = aresed3d->GetSelection ()->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dynobj->UndoKinematic ();
  }
}

void MainMode::StopDrag ()
{
  dragObjects.DeleteAll ();
  if (do_dragging)
  {
    do_dragging = false;

    // Put back the original dampening on the rigid body
    csRef<CS::Physics::Bullet::iRigidBody> csBody =
      scfQueryInterface<CS::Physics::Bullet::iRigidBody> (dragJoint->GetAttachedBody ());
    csBody->SetLinearDampener (linearDampening);
    csBody->SetRollingDampener (angularDampening);

    // Remove the drag joint
    aresed3d->GetBulletSystem ()->RemovePivotJoint (dragJoint);
    dragJoint = 0;
  }
  if (do_kinematic_dragging)
  {
    do_kinematic_dragging = false;
    SelectionIterator it = aresed3d->GetSelection ()->GetIterator ();
    while (it.HasNext ())
    {
      iDynamicObject* dynobj = it.Next ();
      dynobj->UndoKinematic ();
      if (kinematicFirstOnly) break;
    }
  }
}

void MainMode::HandleKinematicDragging ()
{
  csSegment3 beam = aresed3d->GetMouseBeam (1000.0f);
  csVector3 newPosition;
  if (doDragRestrictY)
  {
    if (fabs (beam.Start ().y-beam.End ().y) < 0.1f) return;
    if (beam.End ().y < beam.Start ().y && dragRestrictY > beam.Start ().y) return;
    if (beam.End ().y > beam.Start ().y && dragRestrictY < beam.Start ().y) return;
    float dist = csIntersect3::SegmentYPlane (beam, dragRestrictY, newPosition);
    if (dist > 0.08f)
    {
      newPosition = beam.Start () + (beam.End ()-beam.Start ()).Unit () * 80.0f;
      newPosition.y = dragRestrictY;
    }
  }
  else
  {
    iCamera* camera = aresed3d->GetCsCamera ();
    newPosition = beam.End () - beam.Start ();
    newPosition.Normalize ();
    newPosition = camera->GetTransform ().GetOrigin () + newPosition * dragDistance;
  }
  for (size_t i = 0 ; i < dragObjects.GetSize () ; i++)
  {
    csVector3 np = newPosition - dragObjects[i].kineOffset;
    iMeshWrapper* mesh = dragObjects[i].dynobj->GetMesh ();
    if (mesh)
    {
      iMovable* mov = mesh->GetMovable ();
      mov->GetTransform ().SetOrigin (np);
      mov->UpdateMove ();
    }
    if (kinematicFirstOnly) break;
  }
}

void MainMode::HandlePhysicalDragging ()
{
  // Keep the drag joint at the same distance to the camera
  iCamera* camera = aresed3d->GetCsCamera ();
  csSegment3 beam = aresed3d->GetMouseBeam ();
  csVector3 newPosition = beam.End () - beam.Start ();
  newPosition.Normalize ();
  newPosition = camera->GetTransform ().GetOrigin () + newPosition * dragDistance;
  dragJoint->SetPosition (newPosition);
}

void MainMode::FramePre()
{
  if (do_dragging)
  {
    HandlePhysicalDragging ();
  }
  else if (do_kinematic_dragging)
  {
    HandleKinematicDragging ();
  }
}

void MainMode::Frame3D()
{
}

void MainMode::Frame2D()
{
}

csString MainMode::GetSelectedItem ()
{
  wxTreeCtrl* tree = XRCCTRL (*panel, "factoryTree", wxTreeCtrl);
  wxTreeItemId id = tree->GetSelection ();
  if (!id.IsOk ()) return csString ();
  return csString ((const char*)tree->GetItemText (id).mb_str (wxConvUTF8));
}

bool MainMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  bool slow = aresed3d->GetKeyboardDriver ()->GetKeyState (CSKEY_CTRL);
  bool fast = aresed3d->GetKeyboardDriver ()->GetKeyState (CSKEY_SHIFT);
  if (code == '2')
  {
    SelectionIterator it = aresed3d->GetSelection ()->GetIterator ();
    while (it.HasNext ())
    {
      iDynamicObject* dynobj = it.Next ();
      if (dynobj->IsStatic ())
	dynobj->MakeDynamic ();
      else
	dynobj->MakeStatic ();
    }
    CurrentObjectsChanged (aresed3d->GetSelection ()->GetObjects ());
  }
  else if (code == 'h')
  {
    if (holdJoint)
    {
      aresed3d->GetBulletSystem ()->RemovePivotJoint (holdJoint);
      holdJoint = 0;
    }
    if (do_dragging)
    {
      csRef<CS::Physics::Bullet::iRigidBody> csBody =
	scfQueryInterface<CS::Physics::Bullet::iRigidBody> (dragJoint->GetAttachedBody ());
      csBody->SetLinearDampener (linearDampening);
      csBody->SetRollingDampener (angularDampening);
      holdJoint = dragJoint;
      dragJoint = 0;
      do_dragging = false;
    }
  }
  else if (code == 'e')
  {
    csString itemName = GetSelectedItem ();
    if (!itemName.IsEmpty ())
    {
      aresed3d->SpawnItem (itemName);
    }
  }
  else if (code == CSKEY_UP)
  {
    TransformTools::Move (aresed3d->GetSelection (), csVector3 (0, 0, 1), slow, fast);
  }
  else if (code == CSKEY_DOWN)
  {
    TransformTools::Move (aresed3d->GetSelection (), csVector3 (0, 0, -1), slow, fast);
  }
  else if (code == CSKEY_LEFT)
  {
    TransformTools::Move (aresed3d->GetSelection (), csVector3 (-1, 0, 0), slow, fast);
  }
  else if (code == CSKEY_RIGHT)
  {
    TransformTools::Move (aresed3d->GetSelection (), csVector3 (1, 0, 0), slow, fast);
  }
  else if (code == '<' || code == ',')
  {
    TransformTools::Move (aresed3d->GetSelection (), csVector3 (0, -1, 0), slow, fast);
  }
  else if (code == '>' || code == '.')
  {
    TransformTools::Move (aresed3d->GetSelection (), csVector3 (0, 1, 0), slow, fast);
  }

  return false;
}

void MainMode::StartKinematicDragging (bool restrictY,
    const csSegment3& beam, const csVector3& isect, bool firstOnly)
{
  do_kinematic_dragging = true;
  kinematicFirstOnly = firstOnly;

  SelectionIterator it = aresed3d->GetSelection ()->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dynobj->MakeKinematic ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    csVector3 meshpos = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    AresDragObject dob;
    dob.kineOffset = isect - meshpos;
    dob.dynobj = dynobj;
    dragObjects.Push (dob);
    if (kinematicFirstOnly) break;
  }

  dragDistance = (isect - beam.Start ()).Norm ();
  doDragRestrictY = restrictY;
  if (doDragRestrictY)
  {
    dragRestrictY = isect.y;
  }
}

void MainMode::StartPhysicalDragging (iRigidBody* hitBody,
    const csSegment3& beam, const csVector3& isect)
{
  // Create a pivot joint at the point clicked
  dragJoint = aresed3d->GetBulletSystem ()->CreatePivotJoint ();
  dragJoint->Attach (hitBody, isect);

  do_dragging = true;
  dragDistance = (isect - beam.Start ()).Norm ();

  // Set some dampening on the rigid body to have a more stable dragging
  csRef<CS::Physics::Bullet::iRigidBody> csBody =
        scfQueryInterface<CS::Physics::Bullet::iRigidBody> (hitBody);
  linearDampening = csBody->GetLinearDampener ();
  angularDampening = csBody->GetRollingDampener ();
  csBody->SetLinearDampener (0.9f);
  csBody->SetRollingDampener (0.9f);
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
  if (!(but == 0 || but == 1)) return false;

  if (mouseX > aresed3d->GetViewWidth ()) return false;
  if (mouseY > aresed3d->GetViewHeight ()) return false;

  uint32 mod = csMouseEventHelper::GetModifiers (&ev);
  bool shift = (mod & CSMASK_SHIFT) != 0;
  bool ctrl = (mod & CSMASK_CTRL) != 0;
  bool alt = (mod & CSMASK_ALT) != 0;

  int data;
  iMarker* hitMarker = aresed3d->GetMarkerManager ()
    ->FindHitMarker (mouseX, mouseY, data);
  if (hitMarker == transformationMarker)
  {
    iMeshWrapper* mesh = aresed3d->GetSelection ()->GetFirst ()->GetMesh ();
    iCamera* camera = aresed3d->GetCsCamera ();
    csVector3 isect = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    StartKinematicDragging (alt, csSegment3 (camera->GetTransform ().GetOrigin (),
	  isect), isect, true);
    return true;
  }

  // Compute the end beam points
  csVector3 isect;
  csSegment3 beam = aresed3d->GetBeam (mouseX, mouseY);
  iRigidBody* hitBody = aresed3d->TraceBeam (beam, isect);
  if (!hitBody)
  {
    if (but == 0) aresed3d->GetSelection ()->SetCurrentObject (0);
    return false;
  }

  iDynamicObject* newobj = aresed3d->GetDynamicWorld ()->FindObject (hitBody);
  if (but == 0)
  {
    if (shift)
      aresed3d->GetSelection ()->AddCurrentObject (newobj);
    else
      aresed3d->GetSelection ()->SetCurrentObject (newobj);

    StopDrag ();

    if (ctrl || alt)
      StartKinematicDragging (alt, beam, isect, false);
    else if (!newobj->IsStatic ())
      StartPhysicalDragging (hitBody, beam, isect);

    return true;
  }
  else if (but == 1)
  {
    if (ctrl)
    {
    }
    else
    {
      AddForce (hitBody, shift, beam, isect);
    }
    return true;
  }

  return false;
}

bool MainMode::OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (do_dragging || do_kinematic_dragging)
  {
    StopDrag ();
    return true;
  }

  return false;
}

bool MainMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return false;
}


//---------------------------------------------------------------------------

