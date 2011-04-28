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

//---------------------------------------------------------------------------

MainMode::MainMode (AppAresEdit* aresed) :
  EditingMode (aresed, "Main")
{
  do_dragging = false;
  do_kinematic_dragging = false;

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
  aresed->RotateCurrent (PI);
  return true;
}

bool MainMode::OnRotRightButtonClicked (const CEGUI::EventArgs&)
{
  aresed->RotateCurrent (-PI);
  return true;
}

bool MainMode::OnRotResetButtonClicked (const CEGUI::EventArgs&)
{
  aresed->RotResetSelectedObjects ();
  return true;
}

bool MainMode::OnAlignRButtonClicked (const CEGUI::EventArgs&)
{
  aresed->AlignSelectedObjects ();
  return true;
}

bool MainMode::OnStackButtonClicked (const CEGUI::EventArgs&)
{
  aresed->StackSelectedObjects ();
  return true;
}

bool MainMode::OnSameYButtonClicked (const CEGUI::EventArgs&)
{
  aresed->SameYSelectedObjects ();
  return true;
}

bool MainMode::OnSetPosButtonClicked (const CEGUI::EventArgs&)
{
  aresed->SetPosSelectedObjects ();
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
    csArray<iDynamicObject*>::Iterator it = aresed->GetCurrentObjects ().GetIterator ();
    while (it.HasNext ())
    {
      iDynamicObject* dynobj = it.Next ();
      dynobj->UndoKinematic ();
    }
  }
}

void MainMode::FramePre()
{
  iCamera* camera = aresed->GetCsCamera ();
  iGraphics2D* g2d = aresed->GetG2D ();
  if (do_dragging)
  {
    // Keep the drag joint at the same distance to the camera
    csVector2 v2d (aresed->GetMouseX (), g2d->GetHeight () - aresed->GetMouseY ());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    csVector3 newPosition = endBeam - startBeam;
    newPosition.Normalize ();
    newPosition = camera->GetTransform ().GetOrigin () + newPosition * dragDistance;
    dragJoint->SetPosition (newPosition);
  }
  else if (do_kinematic_dragging)
  {
    csVector2 v2d (aresed->GetMouseX (), g2d->GetHeight () - aresed->GetMouseY ());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    csVector3 newPosition;
    if (doDragRestrictY)
    {
      csIntersect3::SegmentYPlane (startBeam, endBeam, dragRestrictY, newPosition);
    }
    else
    {
      newPosition = endBeam - startBeam;
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
    }
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
  if (code == '2')
  {
    csArray<iDynamicObject*>::Iterator it = aresed->GetCurrentObjects ().GetIterator ();
    while (it.HasNext ())
    {
      iDynamicObject* dynobj = it.Next ();
      if (dynobj->IsStatic ())
	dynobj->MakeDynamic ();
      else
	dynobj->MakeStatic ();
    }
    CurrentObjectsChanged (aresed->GetCurrentObjects ());
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
    aresed->MoveCurrent (csVector3 (0, 0, 1));
  }
  else if (code == CSKEY_DOWN)
  {
    aresed->MoveCurrent (csVector3 (0, 0, -1));
  }
  else if (code == CSKEY_LEFT)
  {
    aresed->MoveCurrent (csVector3 (-1, 0, 0));
  }
  else if (code == CSKEY_RIGHT)
  {
    aresed->MoveCurrent (csVector3 (1, 0, 0));
  }
  else if (code == '<' || code == ',')
  {
    aresed->MoveCurrent (csVector3 (0, -1, 0));
  }
  else if (code == '>' || code == '.')
  {
    aresed->MoveCurrent (csVector3 (0, 1, 0));
  }

  return false;
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

  // Compute the end beam points
  csVector3 startBeam, endBeam, isect;
  iRigidBody* hitBody = aresed->TraceBeam (mouseX, mouseY, startBeam, endBeam, isect);
  if (!hitBody)
  {
    if (but == 1) aresed->SetCurrentObject (0);
    return false;
  }

  iDynamicObject* newobj = aresed->GetDynamicWorld ()->FindObject (hitBody);
  if (but == 1)
  {
    if (shift)
      aresed->AddCurrentObject (newobj);
    else
      aresed->SetCurrentObject (newobj);

    StopDrag ();

    if (ctrl || alt)
    {
      do_kinematic_dragging = true;

      csArray<iDynamicObject*>::Iterator it = aresed->GetCurrentObjects ().GetIterator ();
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
      }

      dragDistance = (isect - startBeam).Norm ();
      doDragRestrictY = alt;
      if (doDragRestrictY)
      {
        dragRestrictY = isect.y;
      }
    }
    else if (!newobj->IsStatic ())
    {
      // Create a pivot joint at the point clicked
      dragJoint = aresed->GetBulletSystem ()->CreatePivotJoint ();
      dragJoint->Attach (hitBody, isect);

      do_dragging = true;
      dragDistance = (isect - startBeam).Norm ();

      // Set some dampening on the rigid body to have a more stable dragging
      csRef<CS::Physics::Bullet::iRigidBody> csBody =
        scfQueryInterface<CS::Physics::Bullet::iRigidBody> (hitBody);
      linearDampening = csBody->GetLinearDampener ();
      angularDampening = csBody->GetRollingDampener ();
      csBody->SetLinearDampener (0.9f);
      csBody->SetRollingDampener (0.9f);
    }
    return true;
  }
  else if (but == 0)
  {
    // Add a force at the point clicked
    csVector3 force = endBeam - startBeam;
    force.Normalize ();
    if (shift)
      force *= -hitBody->GetMass ();
    else
      force *= hitBody->GetMass ();
    hitBody->AddForceAtPos (force, isect);
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

