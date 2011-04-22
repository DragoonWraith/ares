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
#include "curvemode.h"

//---------------------------------------------------------------------------

CurveMode::CurveMode (AppAresEdit* aresed)
  : EditingMode (aresed, "Curve")
{
  editingCurveFactory = 0;
  do_dragging = false;
  autoSmooth = true;

  CEGUI::WindowManager* winMgr = aresed->GetCEGUI ()->GetWindowManagerPtr ();
  CEGUI::Window* btn;

  btn = winMgr->getWindow("Ares/CurveWindow/RotLeft");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CurveMode::OnRotLeftButtonClicked, this));
  btn = winMgr->getWindow("Ares/CurveWindow/RotRight");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CurveMode::OnRotRightButtonClicked, this));
  btn = winMgr->getWindow("Ares/CurveWindow/RotReset");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CurveMode::OnRotResetButtonClicked, this));

  btn = winMgr->getWindow("Ares/CurveWindow/Flatten");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CurveMode::OnFlattenButtonClicked, this));

  autoSmoothCheck = static_cast<CEGUI::Checkbox*>(winMgr->getWindow("Ares/CurveWindow/AutoSmooth"));
  autoSmoothCheck->setSelected (autoSmooth);
  autoSmoothCheck->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged,
    CEGUI::Event::Subscriber(&CurveMode::OnAutoSmoothSelected, this));
}

void CurveMode::Start ()
{
  csString name = aresed->GetCurrentObjects ()[0]->GetFactory ()->GetName ();
  editingCurveFactory = aresed->GetCurvedMeshCreator ()->GetCurvedFactory (name);
  selectedPoints.Empty ();
  do_dragging = false;
  dragPoints.Empty ();
}

void CurveMode::SetCurrentPoint (size_t idx)
{
  if (selectedPoints.Find (idx) != csArrayItemNotFound) return;
  selectedPoints.DeleteAll ();
  selectedPoints.Push (idx);
}

void CurveMode::AddCurrentPoint (size_t idx)
{
  if (selectedPoints.Find (idx) != csArrayItemNotFound)
  {
    selectedPoints.Delete (idx);
  }
  else
  {
    selectedPoints.Push (idx);
  }
}

void CurveMode::StopDrag ()
{
  dragPoints.DeleteAll ();
  do_dragging = false;
}

void CurveMode::FramePre()
{
  iCamera* camera = aresed->GetCsCamera ();
  iGraphics2D* g2d = aresed->GetG2D ();
  if (do_dragging)
  {
    //@@@ Implement undo for curve editing too?
    aresed->PushUndo ("MoveCurve");
    csVector2 v2d (aresed->GetMouseX (), g2d->GetHeight () - aresed->GetMouseY ());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    iMeshWrapper* mesh = aresed->GetCurrentObjects ()[0]->GetMesh ();
    csVector3 newPosition;
    if (doDragRestrictY)
    {
      csIntersect3::SegmentYPlane (startBeam, endBeam, dragRestrictY, newPosition);
    }
    else if (doDragMesh)
    {
      csVector3 pos = endBeam - startBeam;
      pos.Normalize ();
      pos = camera->GetTransform ().GetOrigin () + pos * 1000.0f;
      csFlags oldFlags = mesh->GetFlags ();
      mesh->GetFlags ().Set (CS_ENTITY_NOHITBEAM);
      csSectorHitBeamResult result = camera->GetSector ()->HitBeamPortals (
	  camera->GetTransform ().GetOrigin (), pos);
      mesh->GetFlags ().SetAll (oldFlags.Get ());
      if (!result.mesh) return;	// Safety, we hit no mesh so better don't do anything.
      newPosition = result.isect;
    }
    else
    {
      newPosition = endBeam - startBeam;
      newPosition.Normalize ();
      newPosition = camera->GetTransform ().GetOrigin () + newPosition * dragDistance;
    }
    const csReversibleTransform& meshtrans = mesh->GetMovable ()->GetTransform ();
    for (size_t i = 0 ; i < dragPoints.GetSize () ; i++)
    {
      csVector3 np = newPosition - dragPoints[i].kineOffset;
      size_t idx = dragPoints[i].idx;
      csVector3 rp = meshtrans.Other2This (np);
      editingCurveFactory->ChangePoint (idx,
	  rp,
	  editingCurveFactory->GetFront (idx),
	  editingCurveFactory->GetUp (idx));
    }
    if (autoSmooth) DoAutoSmooth ();
    editingCurveFactory->GenerateFactory ();
    aresed->GetCurrentObjects ()[0]->RefreshColliders ();
  }
}

void CurveMode::Frame3D()
{
}

void CurveMode::Frame2D()
{
  iGraphics2D* g2d = aresed->GetG2D ();

  float sy = g2d->GetHeight ();
  iCamera* camera = aresed->GetCsCamera ();
  const csOrthoTransform& camtrans = camera->GetTransform ();
  iMeshWrapper* mesh = aresed->GetCurrentObjects ()[0]->GetMesh ();
  const csReversibleTransform& meshtrans = mesh->GetMovable ()->GetTransform ();
  for (size_t i = 0 ; i < editingCurveFactory->GetPointCount () ; i++)
  {
    bool sel = selectedPoints.Find (i) != csArrayItemNotFound;
    const csVector3& pos = editingCurveFactory->GetPosition (i);
    const csVector3& front = editingCurveFactory->GetFront (i);
    const csVector3& up = editingCurveFactory->GetUp (i);
    csVector3 campos = camtrans.Other2This (meshtrans.This2Other (pos));
    if (campos.z > .1)
    {
      csVector2 scrpos = camera->Perspective (campos);
      csVector3 camfront = camtrans.Other2This (meshtrans.This2Other (pos+front));
      if (camfront.z > .1)
      {
	csVector2 scrfront = camera->Perspective (camfront);
	g2d->DrawLine (scrpos.x, sy-scrpos.y, scrfront.x, sy-scrfront.y, g2d->FindRGB (0, 255, 0));
	if (sel)
	{
	  g2d->DrawLine (scrpos.x+1, sy-scrpos.y, scrfront.x+1, sy-scrfront.y, g2d->FindRGB (0, 255, 0));
	  g2d->DrawLine (scrpos.x, sy-scrpos.y+1, scrfront.x, sy-scrfront.y+1, g2d->FindRGB (0, 255, 0));
	}
      }
      csVector3 camup = camtrans.Other2This (meshtrans.This2Other (pos+up));
      if (camup.z > .1)
      {
	csVector2 scrup = camera->Perspective (camup);
	g2d->DrawLine (scrpos.x, sy-scrpos.y, scrup.x, sy-scrup.y, g2d->FindRGB (255, 0, 0));
	if (sel)
	{
	  g2d->DrawLine (scrpos.x+1, sy-scrpos.y, scrup.x+1, sy-scrup.y, g2d->FindRGB (255, 0, 0));
	  g2d->DrawLine (scrpos.x, sy-scrpos.y+1, scrup.x, sy-scrup.y+1, g2d->FindRGB (255, 0, 0));
	}
      }
    }
  }
}

void CurveMode::DoAutoSmooth ()
{
  if (editingCurveFactory->GetPointCount () <= 2) return;
  for (size_t i = 1 ; i < editingCurveFactory->GetPointCount ()-1 ; i++)
    SmoothPoint (i, false);
  editingCurveFactory->GenerateFactory ();
  aresed->GetCurrentObjects ()[0]->RefreshColliders ();
}

void CurveMode::SmoothPoint (size_t idx, bool regen)
{
  if (idx == 0 || idx >= editingCurveFactory->GetPointCount ()-1)
    return;
  const csVector3& p1 = editingCurveFactory->GetPosition (idx-1);
  const csVector3& pos = editingCurveFactory->GetPosition (idx);
  const csVector3& p2 = editingCurveFactory->GetPosition (idx+1);
  csVector3 fr = ((p2 - pos).Unit () + (pos - p1).Unit ()) / 2.0;
  csVector3 up = editingCurveFactory->GetUp (idx);
  //csVector3 right = fr % up;
  //up = - (fr % right).Unit ();
  editingCurveFactory->ChangePoint (idx, pos, fr, up);
  if (regen)
  {
    editingCurveFactory->GenerateFactory ();
    aresed->GetCurrentObjects ()[0]->RefreshColliders ();
  }
}

csVector3 CurveMode::GetWorldPosPoint (size_t idx)
{
  iMeshWrapper* mesh = aresed->GetCurrentObjects ()[0]->GetMesh ();
  const csReversibleTransform& meshtrans = mesh->GetMovable ()->GetTransform ();
  const csVector3& pos = editingCurveFactory->GetPosition (idx);
  return meshtrans.This2Other (pos);
}

size_t CurveMode::FindCurvePoint (int mouseX, int mouseY)
{
  iGraphics2D* g2d = aresed->GetG2D ();

  float sy = g2d->GetHeight ();
  iCamera* camera = aresed->GetCsCamera ();
  const csOrthoTransform& camtrans = camera->GetTransform ();
  float bestSqDist = 1000000000.0;
  size_t bestIdx = csArrayItemNotFound;
  for (size_t i = 0 ; i < editingCurveFactory->GetPointCount () ; i++)
  {
    csVector3 campos = camtrans.Other2This (GetWorldPosPoint (i));
    if (campos.z > .1)
    {
      csVector2 scrpos = camera->Perspective (campos);
      scrpos.y = sy-scrpos.y;
      float sqDist = (scrpos.x-mouseX) * (scrpos.x-mouseX)
	+ (scrpos.y-mouseY) * (scrpos.y-mouseY);
      if (sqDist < bestSqDist)
      {
	bestSqDist = sqDist;
	bestIdx = i;
      }
    }
  }
  if (bestSqDist < 100)
    return bestIdx;
  else
    return csArrayItemNotFound;
}

void CurveMode::RotateCurrent (float baseAngle)
{
  aresed->PushUndo ("RotCurve");
  bool slow = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_CTRL);
  bool fast = aresed->GetKeyboardDriver ()->GetKeyState (CSKEY_SHIFT);
  float angle = baseAngle;
  if (slow) angle /= 180.0;
  else if (fast) angle /= 2.0;
  else angle /= 8.0;

  csArray<size_t>::Iterator it = selectedPoints.GetIterator ();
  while (it.HasNext ())
  {
    size_t id = it.Next ();
    // @TODO: optimize!!!
    const csVector3& pos = editingCurveFactory->GetPosition (id);
    const csVector3& f = editingCurveFactory->GetFront (id);
    const csVector3& u = editingCurveFactory->GetUp (id);
    csVector3 r = f % u;
    csMatrix3 m (r.x, r.y, r.z, u.x, u.y, u.z, f.x, f.y, f.z);
    csReversibleTransform tr (m, pos);
    tr.RotateOther (u, angle);
    editingCurveFactory->ChangePoint (id, pos, tr.GetFront (), u);
  }
  editingCurveFactory->GenerateFactory ();
  aresed->GetCurrentObjects ()[0]->RefreshColliders ();
}

void CurveMode::FlatPoint (size_t idx)
{
  float sideHeight = editingCurveFactory->GetSideHeight ();
  iMeshWrapper* mesh = aresed->GetCurrentObjects ()[0]->GetMesh ();
  const csReversibleTransform& meshtrans = mesh->GetMovable ()->GetTransform ();
  csVector3 pos = editingCurveFactory->GetPosition (idx);
  pos = meshtrans.This2Other (pos);
  const csVector3& f = editingCurveFactory->GetFront (idx);
  const csVector3& u = editingCurveFactory->GetUp (idx);
  pos.y += 100.0;
  iSector* sector = aresed->GetCsCamera ()->GetSector ();

  csFlags oldFlags = mesh->GetFlags ();
  mesh->GetFlags ().Set (CS_ENTITY_NOHITBEAM);
  csSectorHitBeamResult result = sector->HitBeamPortals (pos, pos - csVector3 (0, 1000, 0));
  mesh->GetFlags ().SetAll (oldFlags.Get ());

  if (result.mesh)
  {
    pos = result.isect;
    pos.y += sideHeight / 2.0;
    pos = meshtrans.Other2This (pos);
    editingCurveFactory->ChangePoint (idx, pos, f, u);
    if (autoSmooth)
      DoAutoSmooth ();
    else
      editingCurveFactory->GenerateFactory ();
    aresed->GetCurrentObjects ()[0]->RefreshColliders ();
  }
}

bool CurveMode::OnFlattenButtonClicked (const CEGUI::EventArgs&)
{
  if (selectedPoints.GetSize () == 0)
  {
    for (size_t i = 0 ; i < editingCurveFactory->GetPointCount () ; i++)
      FlatPoint (i);
  }
  else
  {
    csArray<size_t>::Iterator it = selectedPoints.GetIterator ();
    while (it.HasNext ())
    {
      size_t id = it.Next ();
      FlatPoint (id);
    }
  }

  return true;
}

bool CurveMode::OnRotLeftButtonClicked (const CEGUI::EventArgs&)
{
  RotateCurrent (-PI);
  return true;
}

bool CurveMode::OnRotRightButtonClicked (const CEGUI::EventArgs&)
{
  RotateCurrent (PI);
  return true;
}

bool CurveMode::OnRotResetButtonClicked (const CEGUI::EventArgs&)
{
  csArray<size_t>::Iterator it = selectedPoints.GetIterator ();
  while (it.HasNext ())
  {
    size_t id = it.Next ();
    SmoothPoint (id);
  }
  return true;
}

bool CurveMode::OnAutoSmoothSelected (const CEGUI::EventArgs&)
{
  autoSmooth = autoSmoothCheck->isSelected ();
  return true;
}

bool CurveMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  if (code == 'e')
  {
    size_t id = editingCurveFactory->GetPointCount ()-1;
    const csVector3& pos = editingCurveFactory->GetPosition (id);
    const csVector3& front = editingCurveFactory->GetFront (id);
    const csVector3& up = editingCurveFactory->GetUp (id);
    editingCurveFactory->AddPoint (pos + front, front, up);
    if (autoSmooth) DoAutoSmooth ();

    iMeshWrapper* mesh = aresed->GetCurrentObjects ()[0]->GetMesh ();
    editingCurveFactory->FlattenToGround (mesh);
    editingCurveFactory->GenerateFactory ();
    aresed->GetCurrentObjects ()[0]->RefreshColliders ();
  }
  return false;
}

bool CurveMode::OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (!(but == 0 || but == 1)) return false;

  if (mouseX > aresed->GetViewWidth ()) return false;
  if (mouseY > aresed->GetViewHeight ()) return false;

  csVector3 startBeam = aresed->GetCsCamera ()->GetTransform ().GetOrigin ();

  uint32 mod = csMouseEventHelper::GetModifiers (&ev);
  bool shift = mod & CSMASK_SHIFT;
  bool ctrl = mod & CSMASK_CTRL;
  bool alt = mod & CSMASK_ALT;

  size_t idx = FindCurvePoint (mouseX, mouseY);
  if (idx == csArrayItemNotFound)
    return false;

  csVector3 isect = GetWorldPosPoint (idx);

  if (shift)
    AddCurrentPoint (idx);
  else
    SetCurrentPoint (idx);

  if (but == 0)
  {
    StopDrag ();

    //if (ctrl || alt)
    {
      do_dragging = true;

      csArray<size_t>::Iterator it = selectedPoints.GetIterator ();
      while (it.HasNext ())
      {
	size_t id = it.Next ();
	csVector3 pos = GetWorldPosPoint (id);
	DragPoint dob;

	dob.kineOffset = isect - pos;
	dob.idx = id;
	dragPoints.Push (dob);
      }

      dragDistance = (isect - startBeam).Norm ();
      if (alt)
      {
        doDragRestrictY = true;
	doDragMesh = false;
        dragRestrictY = isect.y;
      }
      else if (!ctrl)
      {
	doDragRestrictY = false;
	doDragMesh = true;
      }
    }
  }

  return true;
}

bool CurveMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (do_dragging)
  {
    StopDrag ();
    return true;
  }

  return false;
}

bool CurveMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return false;
}

