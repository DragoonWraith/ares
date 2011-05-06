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

void CurveMode::UpdateMarkers ()
{
  iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
  if (editingCurveFactory->GetPointCount () != markers.GetSize ())
  {
    Stop ();
    for (size_t i = 0 ; i < editingCurveFactory->GetPointCount () ; i++)
    {
      iMarker* marker = aresed->GetMarkerManager ()->CreateMarker ();
      markers.Push (marker);
      marker->AttachMesh (mesh);
    }
  }

  iMarkerColor* red = aresed->GetMarkerManager ()->FindMarkerColor ("red");
  iMarkerColor* green = aresed->GetMarkerManager ()->FindMarkerColor ("green");

  for (size_t i = 0 ; i < editingCurveFactory->GetPointCount () ; i++)
  {
    csVector3 pos = editingCurveFactory->GetPosition (i);
    csVector3 front = editingCurveFactory->GetFront (i);
    csVector3 up = editingCurveFactory->GetUp (i);
    iMarker* marker = markers[i];
    marker->Clear ();
    marker->Line (MARKER_OBJECT, pos, pos+front, green, true);
    marker->Line (MARKER_OBJECT, pos, pos+up, red, true);
    marker->ClearHitAreas ();
    marker->HitArea (MARKER_OBJECT, pos, 10.0f, 0);
  }

  UpdateMarkerSelection ();
}

void CurveMode::UpdateMarkerSelection ()
{
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    markers[i]->SetSelectionLevel (SELECTION_NONE);
  for (size_t i = 0 ; i < selectedPoints.GetSize () ; i++)
    markers[selectedPoints[i]]->SetSelectionLevel (SELECTION_SELECTED);
}

void CurveMode::Start ()
{
  csString name = aresed->GetSelection ()->GetFirst ()->GetFactory ()->GetName ();
  editingCurveFactory = aresed->GetCurvedMeshCreator ()->GetCurvedFactory (name);
  ggen = scfQueryInterface<iGeometryGenerator> (editingCurveFactory);
  selectedPoints.Empty ();
  do_dragging = false;
  dragPoints.Empty ();

  UpdateMarkers ();
}

void CurveMode::Stop ()
{
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    aresed->GetMarkerManager ()->DestroyMarker (markers[i]);
  markers.Empty ();
}

void CurveMode::SetCurrentPoint (size_t idx)
{
  if (selectedPoints.Find (idx) != csArrayItemNotFound) return;
  selectedPoints.DeleteAll ();
  selectedPoints.Push (idx);
  UpdateMarkerSelection ();
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
  UpdateMarkerSelection ();
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
    csVector2 v2d (aresed->GetMouseX (), g2d->GetHeight () - aresed->GetMouseY ());
    csVector3 v3d = camera->InvPerspective (v2d, 10000);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

    iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
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
      UpdateMarkers ();
    }
    if (autoSmooth) DoAutoSmooth ();
    ggen->GenerateGeometry (mesh);
    aresed->GetSelection ()->GetFirst ()->RefreshColliders ();
  }
}

void CurveMode::Frame3D()
{
}

void CurveMode::Frame2D()
{
}

void CurveMode::DoAutoSmooth ()
{
  if (editingCurveFactory->GetPointCount () <= 2) return;
  for (size_t i = 1 ; i < editingCurveFactory->GetPointCount ()-1 ; i++)
    SmoothPoint (i, false);
  iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
  ggen->GenerateGeometry (mesh);
  aresed->GetSelection ()->GetFirst ()->RefreshColliders ();
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
  UpdateMarkers ();
  if (regen)
  {
    iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
    ggen->GenerateGeometry (mesh);
    aresed->GetSelection ()->GetFirst ()->RefreshColliders ();
  }
}

csVector3 CurveMode::GetWorldPosPoint (size_t idx)
{
  iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
  const csReversibleTransform& meshtrans = mesh->GetMovable ()->GetTransform ();
  const csVector3& pos = editingCurveFactory->GetPosition (idx);
  return meshtrans.This2Other (pos);
}

size_t CurveMode::FindCurvePoint (int mouseX, int mouseY)
{
  int data;
  iMarker* marker = aresed->GetMarkerManager ()->FindHitMarker (mouseX, mouseY, data);
  if (!marker) return csArrayItemNotFound;
  size_t idx = 0;
  while (idx < markers.GetSize ())
    if (markers[idx] == marker) return idx;
    else idx++;
  return csArrayItemNotFound;
}

void CurveMode::RotateCurrent (float baseAngle)
{
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
    UpdateMarkers ();
  }
  iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
  ggen->GenerateGeometry (mesh);
  aresed->GetSelection ()->GetFirst ()->RefreshColliders ();
}

void CurveMode::FlatPoint (size_t idx)
{
  float sideHeight = editingCurveFactory->GetSideHeight ();
  iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
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
    UpdateMarkers ();
    if (autoSmooth)
      DoAutoSmooth ();
    else
      ggen->GenerateGeometry (mesh);
    aresed->GetSelection ()->GetFirst ()->RefreshColliders ();
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

    iMeshWrapper* mesh = aresed->GetSelection ()->GetFirst ()->GetMesh ();
    ggen->GenerateGeometry (mesh);
    aresed->GetSelection ()->GetFirst ()->RefreshColliders ();
    UpdateMarkers ();
  }
  return false;
}

bool CurveMode::OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (but != 0) return false;

  if (mouseX > aresed->GetViewWidth ()) return false;
  if (mouseY > aresed->GetViewHeight ()) return false;

  csVector3 startBeam = aresed->GetCsCamera ()->GetTransform ().GetOrigin ();

  uint32 mod = csMouseEventHelper::GetModifiers (&ev);
  bool shift = (mod & CSMASK_SHIFT) != 0;
  bool ctrl = (mod & CSMASK_CTRL) != 0;
  bool alt = (mod & CSMASK_ALT) != 0;

  size_t idx = FindCurvePoint (mouseX, mouseY);
  if (idx == csArrayItemNotFound)
    return false;

  csVector3 isect = GetWorldPosPoint (idx);

  if (shift)
    AddCurrentPoint (idx);
  else
    SetCurrentPoint (idx);

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

