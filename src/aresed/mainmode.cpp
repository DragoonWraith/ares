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

//---------------------------------------------------------------------------

void MarkerCallback::StartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos)
{
  mainmode->MarkerStartDragging (marker, area, pos);
}

void MarkerCallback::MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
{
  mainmode->MarkerWantsMove (marker, area, pos);
}

void MarkerCallback::StopDragging (iMarker* marker, iMarkerHitArea* area)
{
  mainmode->MarkerStopDragging (marker, area);
}

//---------------------------------------------------------------------------

MainMode::MainMode (AppAresEdit* aresed) :
  EditingMode (aresed, "Main")
{
  do_dragging = false;
  do_kinematic_dragging = false;

  transformationMarker = 0;

  CEGUI::WindowManager* winMgr = aresed->GetCEGUI ()->GetWindowManagerPtr ();
  CEGUI::Window* btn;

  btn = winMgr->getWindow("Ares/ItemWindow/Del");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnDelButtonClicked, this));

  btn = winMgr->getWindow("Ares/ItemWindow/RotLeft");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnRotLeftButtonClicked, this));
  btn = winMgr->getWindow("Ares/ItemWindow/RotRight");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnRotRightButtonClicked, this));
  btn = winMgr->getWindow("Ares/ItemWindow/RotReset");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnRotResetButtonClicked, this));

  btn = winMgr->getWindow("Ares/ItemWindow/AlignR");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnAlignRButtonClicked, this));
  btn = winMgr->getWindow("Ares/ItemWindow/SetPos");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnSetPosButtonClicked, this));

  btn = winMgr->getWindow("Ares/ItemWindow/Stack");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnStackButtonClicked, this));
  btn = winMgr->getWindow("Ares/ItemWindow/SameY");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&MainMode::OnSameYButtonClicked, this));

  itemList = static_cast<CEGUI::MultiColumnList*>(winMgr->getWindow("Ares/ItemWindow/List"));
  itemList->subscribeEvent(CEGUI::MultiColumnList::EventSelectionChanged,
    CEGUI::Event::Subscriber(&MainMode::OnItemListSelection, this));

  categoryList = static_cast<CEGUI::MultiColumnList*>(winMgr->getWindow("Ares/ItemWindow/Category"));
  categoryList->subscribeEvent(CEGUI::MultiColumnList::EventSelectionChanged,
    CEGUI::Event::Subscriber(&MainMode::OnCategoryListSelection, this));

  staticCheck = static_cast<CEGUI::Checkbox*>(winMgr->getWindow("Ares/ItemWindow/Static"));
  staticCheck->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged,
    CEGUI::Event::Subscriber(&MainMode::OnStaticSelected, this));
}

void MainMode::Start ()
{
  if (!transformationMarker)
  {
    transformationMarker = aresed->GetMarkerManager ()->CreateMarker ();
    iMarkerColor* red = aresed->GetMarkerManager ()->FindMarkerColor ("red");
    iMarkerColor* green = aresed->GetMarkerManager ()->FindMarkerColor ("green");
    iMarkerColor* blue = aresed->GetMarkerManager ()->FindMarkerColor ("blue");
    iMarkerColor* yellow = aresed->GetMarkerManager ()->FindMarkerColor ("yellow");
    transformationMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (1,0,0), red, true);
    transformationMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,1,0), green, true);
    transformationMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,0,1), blue, true);
    iMarkerHitArea* hitArea = transformationMarker->HitArea (
	MARKER_OBJECT, csVector3 (0), .1f, 0, yellow);
    csRef<MarkerCallback> cb;
    cb.AttachNew (new MarkerCallback (this));
    hitArea->DefineDrag (0, false, false, false, MARKER_WORLD, false, false, false, cb);
    hitArea->DefineDrag (0, false, false, true, MARKER_WORLD, false, true, false, cb);
  }

  if (aresed->GetSelection ()->GetSize () >= 1)
  {
    transformationMarker->SetVisible (true);
    transformationMarker->AttachMesh (aresed->GetSelection ()->GetFirst ()->GetMesh ());
  }
  else
    transformationMarker->SetVisible (false);
}

void MainMode::Stop ()
{
  transformationMarker->SetVisible (false);
  transformationMarker->AttachMesh (0);
}

void MainMode::AddCategory (const char* category)
{
  CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem (CEGUI::String (category));
  item->setTextColours (CEGUI::colour(0,0,0));
  item->setSelectionBrushImage ("ice", "TextSelectionBrush");
  item->setSelectionColours (CEGUI::colour(0.5f,0.5f,1));
  uint colid = categoryList->getColumnID (0);
  categoryList->addRow (item, colid);
}

void MainMode::CurrentObjectsChanged (const csArray<iDynamicObject*>& current)
{
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
}

void MainMode::UpdateItemList ()
{
  itemList->resetList ();
  CEGUI::ListboxItem* item = categoryList->getFirstSelectedItem ();
  if (!item) return;
  const csStringArray& items = aresed->GetCategories ().Get (item->getText ().c_str (), csStringArray ());
  for (size_t i = 0 ; i < items.GetSize () ; i++)
  {
    CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem (CEGUI::String (items[i]));
    item->setTextColours (CEGUI::colour(0,0,0));
    item->setSelectionBrushImage ("ice", "TextSelectionBrush");
    item->setSelectionColours (CEGUI::colour(0.5f,0.5f,1));
    uint colid = itemList->getColumnID (0);
    itemList->addRow (item, colid);
  }
}

bool MainMode::OnDelButtonClicked (const CEGUI::EventArgs&)
{
  aresed->DeleteSelectedObjects ();
  return true;
}

bool MainMode::OnRotLeftButtonClicked (const CEGUI::EventArgs&)
{
  bool slow = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_CTRL);
  bool fast = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_SHIFT);
  TransformTools::Rotate (aresed->GetSelection (), PI, slow, fast);
  return true;
}

bool MainMode::OnRotRightButtonClicked (const CEGUI::EventArgs&)
{
  bool slow = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_CTRL);
  bool fast = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_SHIFT);
  TransformTools::Rotate (aresed->GetSelection (), -PI, slow, fast);
  return true;
}

bool MainMode::OnRotResetButtonClicked (const CEGUI::EventArgs&)
{
  TransformTools::RotResetSelectedObjects (aresed->GetSelection ());
  return true;
}

bool MainMode::OnAlignRButtonClicked (const CEGUI::EventArgs&)
{
  TransformTools::AlignSelectedObjects (aresed->GetSelection ());
  return true;
}

bool MainMode::OnStackButtonClicked (const CEGUI::EventArgs&)
{
  TransformTools::StackSelectedObjects (aresed->GetSelection ());
  return true;
}

bool MainMode::OnSameYButtonClicked (const CEGUI::EventArgs&)
{
  TransformTools::SameYSelectedObjects (aresed->GetSelection ());
  return true;
}

bool MainMode::OnSetPosButtonClicked (const CEGUI::EventArgs&)
{
  TransformTools::SetPosSelectedObjects (aresed->GetSelection ());
  return true;
}

bool MainMode::OnStaticSelected (const CEGUI::EventArgs&)
{
  aresed->SetStaticSelectedObjects (staticCheck->isSelected ());
  return true;
}

bool MainMode::OnCategoryListSelection (const CEGUI::EventArgs&)
{
  UpdateItemList ();
  return true;
}

bool MainMode::OnItemListSelection (const CEGUI::EventArgs&)
{
  return true;
}

void MainMode::MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos)
{
  //printf ("START: %g,%g,%g\n", pos.x, pos.y, pos.z); fflush (stdout);
  SelectionIterator it = aresed->GetSelection ()->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dynobj->MakeKinematic ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    csVector3 meshpos = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    DragObject dob;
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

void MainMode::MarkerStopDragging (iMarker* marker, iMarkerHitArea* area)
{
  dragObjects.DeleteAll ();
  do_kinematic_dragging = false;
  SelectionIterator it = aresed->GetSelection ()->GetIterator ();
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
    aresed->GetBulletSystem ()->RemovePivotJoint (dragJoint);
    dragJoint = 0;
  }
  if (do_kinematic_dragging)
  {
    do_kinematic_dragging = false;
    SelectionIterator it = aresed->GetSelection ()->GetIterator ();
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
  csSegment3 beam = aresed->GetMouseBeam (1000.0f);
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
    iCamera* camera = aresed->GetCsCamera ();
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
  iCamera* camera = aresed->GetCsCamera ();
  csSegment3 beam = aresed->GetMouseBeam ();
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

bool MainMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  bool slow = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_CTRL);
  bool fast = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_SHIFT);
  if (code == '2')
  {
    SelectionIterator it = aresed->GetSelection ()->GetIterator ();
    while (it.HasNext ())
    {
      iDynamicObject* dynobj = it.Next ();
      if (dynobj->IsStatic ())
	dynobj->MakeDynamic ();
      else
	dynobj->MakeStatic ();
    }
    CurrentObjectsChanged (aresed->GetSelection ()->GetObjects ());
  }
  else if (code == 'h')
  {
    if (holdJoint)
    {
      aresed->GetBulletSystem ()->RemovePivotJoint (holdJoint);
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
    CEGUI::ListboxItem* item = itemList->getFirstSelectedItem ();
    if (item)
    {
      csString itemName = item->getText().c_str();
      aresed->SpawnItem (itemName);
    }
  }
  else if (code == CSKEY_UP)
  {
    TransformTools::Move (aresed->GetSelection (), csVector3 (0, 0, 1), slow, fast);
  }
  else if (code == CSKEY_DOWN)
  {
    TransformTools::Move (aresed->GetSelection (), csVector3 (0, 0, -1), slow, fast);
  }
  else if (code == CSKEY_LEFT)
  {
    TransformTools::Move (aresed->GetSelection (), csVector3 (-1, 0, 0), slow, fast);
  }
  else if (code == CSKEY_RIGHT)
  {
    TransformTools::Move (aresed->GetSelection (), csVector3 (1, 0, 0), slow, fast);
  }
  else if (code == '<' || code == ',')
  {
    TransformTools::Move (aresed->GetSelection (), csVector3 (0, -1, 0), slow, fast);
  }
  else if (code == '>' || code == '.')
  {
    TransformTools::Move (aresed->GetSelection (), csVector3 (0, 1, 0), slow, fast);
  }

  return false;
}

void MainMode::StartKinematicDragging (bool restrictY,
    const csSegment3& beam, const csVector3& isect, bool firstOnly)
{
  do_kinematic_dragging = true;
  kinematicFirstOnly = firstOnly;

  SelectionIterator it = aresed->GetSelection ()->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dynobj->MakeKinematic ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    csVector3 meshpos = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    DragObject dob;
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
  dragJoint = aresed->GetBulletSystem ()->CreatePivotJoint ();
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

  if (mouseX > aresed->GetViewWidth ()) return false;
  if (mouseY > aresed->GetViewHeight ()) return false;

  uint32 mod = csMouseEventHelper::GetModifiers (&ev);
  bool shift = (mod & CSMASK_SHIFT) != 0;
  bool ctrl = (mod & CSMASK_CTRL) != 0;
  bool alt = (mod & CSMASK_ALT) != 0;

  int data;
  iMarker* hitMarker = aresed->GetMarkerManager ()
    ->FindHitMarker (mouseX, mouseY, data);
  if (hitMarker == transformationMarker)
  {
    iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
    iCamera* camera = aresed->GetCsCamera ();
    csVector3 isect = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    StartKinematicDragging (alt, csSegment3 (camera->GetTransform ().GetOrigin (),
	  isect), isect, true);
    return true;
  }

  // Compute the end beam points
  csVector3 isect;
  csSegment3 beam = aresed->GetBeam (mouseX, mouseY);
  iRigidBody* hitBody = aresed->TraceBeam (beam, isect);
  if (!hitBody)
  {
    if (but == 0) aresed->GetSelection ()->SetCurrentObject (0);
    return false;
  }

  iDynamicObject* newobj = aresed->GetDynamicWorld ()->FindObject (hitBody);
  if (but == 0)
  {
    if (shift)
      aresed->GetSelection ()->AddCurrentObject (newobj);
    else
      aresed->GetSelection ()->SetCurrentObject (newobj);

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

bool MainMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
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

