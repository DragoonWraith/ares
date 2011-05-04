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
#ifdef USE_CEL
#include <celtool/initapp.h>
#endif
#include <cstool/simplestaticlighter.h>
#include <csgeom/math3d.h>
#include "camerawin.h"
#include "selection.h"
#include "common/worldload.h"

void AresEditSelectionListener::SelectionChanged (
    const csArray<iDynamicObject*>& current_objects)
{
  aresed->SelectionChanged (current_objects);
}

AppAresEdit::AppAresEdit() : csApplicationFramework(), camera (this)
{
  SetApplicationName("ares");
  do_debug = false;
  do_simulation = true;
  filereq = 0;
  camwin = 0;
  currentTime = 31000;
  do_auto_time = false;
  editMode = 0;
  mainMode = 0;
  curveMode = 0;
  curvedFactoryCounter = 0;
  worldLoader = 0;
  selection = 0;
}

AppAresEdit::~AppAresEdit()
{
  delete filereq;
  delete camwin;
  delete mainMode;
  delete curveMode;
  delete worldLoader;
  delete selection;
}

void AppAresEdit::DoStuffOncePerFrame ()
{
  // First get elapsed time from the virtual clock.
  csTicks elapsed_time = vc->GetElapsedTicks ();
  nature->UpdateTime (currentTime, GetCsCamera ());
  if (do_auto_time)
    currentTime += elapsed_time;

  camera.Frame (elapsed_time, mouseX, mouseY);

  csReversibleTransform tc = GetCsCamera ()->GetTransform ();
  //csVector3 pos = tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, .5);
  csVector3 pos = tc.GetOrigin () + tc.GetT2O () * csVector3 (2, 0, 2);
  camlight->GetMovable ()->GetTransform ().SetOrigin (pos);
  camlight->GetMovable ()->UpdateMove ();

  editMode->FramePre ();

  if (do_simulation)
  {
    float dynamicSpeed = 1.0f;
    const float speed = elapsed_time / 1000.0;
    dyn->Step (speed / dynamicSpeed);
  }

  dynworld->PrepareView (GetCsCamera (), elapsed_time);
}

void AppAresEdit::Frame ()
{
  /*
    Note: if you use CEL, you probably don't want to call DoStuffOncePerFrame()
          nor g3d->BeginDraw() and use the entity/propclass system.
          This part (similar to simpmap tutorial) is just there as a kickstart.
  */

  DoStuffOncePerFrame ();

  if (g3d->BeginDraw(CSDRAW_3DGRAPHICS))
  {
    // Draw frame.
#ifdef USE_CEL
    // When using CEL, the entity system takes care of drawing,
    // so there's no need to call iView::Draw() here.
    // That template doesn't setup any CEL entity, so we have to call it now.
    // Once you have your entities set, remove it.
    view->Draw ();
#else
    view->Draw ();
#endif
  }

  editMode->Frame3D ();

  if (do_debug)
    bullet_dynSys->DebugDraw (view);

  g3d->BeginDraw (CSDRAW_2DGRAPHICS);
  csString buf;
  const csOrthoTransform& trans = view->GetCamera ()->GetTransform ();
  const csVector3& origin = trans.GetOrigin ();
  buf.Format ("%g,%g,%g", origin.x, origin.y, origin.z);
  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->Write (font, 200, g2d->GetHeight ()-20, colorWhite, -1, buf);

  editMode->Frame2D ();

  cegui->Render ();
}

bool AppAresEdit::OnMouseMove (iEvent& ev)
{
  // Save the mouse position
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

#if USE_DECAL
  csSegment3 beam = GetMouseBeam ();

#if 1
  csSectorHitBeamResult result = GetCsCamera ()->GetSector()->HitBeamPortals (
      beam.Start (), beam.End ());
  if (result.mesh)
  {
    printf ("hit!\n"); fflush (stdout);
    cursorDecal = decalMgr->CreateDecal (cursorDecalTemplate,
        cam->GetSector (), result.isect, csVector3 (0, 1, 0),
	csVector3 (0, -1, 0), 1.0f, 1.0f, cursorDecal);
  }
#else
  csHitBeamResult result = terrainMesh->HitBeam (beam.Start (), beam.End ());
  if (result.hit)
  {
    printf ("hit!\n"); fflush (stdout);
    cursorDecal = decalMgr->CreateDecal (cursorDecalTemplate,
        cam->GetSector (), result.isect, csVector3 (0, 1, 0),
	csVector3 (0, 1, 0), 1.0f, 1.0f, cursorDecal);
  }
#endif

#endif

  if (camera.OnMouseMove (ev, mouseX, mouseY))
    return true;

  return editMode->OnMouseMove(ev, mouseX, mouseY);

  return false;
}

bool AppAresEdit::OnUnhandledEvent (iEvent& event)
{
  if (event.Name == FocusLost)
  {
    camera.OnFocusLost ();
    editMode->OnFocusLost ();
    return true;
  }
  return false;
}

void AppAresEdit::SetStaticSelectedObjects (bool st)
{
  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (st)
    {
      if (!dynobj->IsStatic ())
        dynobj->MakeStatic ();
    }
    else
    {
      if (dynobj->IsStatic ())
        dynobj->MakeDynamic ();
    }
  }
}

void AppAresEdit::SetButtonState ()
{
  bool curveTabEnable = false;
  if (selection->GetSize () == 1)
  {
    csString name = selection->GetFirst ()->GetFactory ()->GetName ();
    if (!curvedMeshCreator->GetCurvedFactory (name))
      curveTabEnable = true;
  }
  if (curveTabEnable)
    curveTabButton->enable ();
  else
    curveTabButton->disable ();
}

void AppAresEdit::SelectionChanged (const csArray<iDynamicObject*>& current_objects)
{
  mainMode->CurrentObjectsChanged (current_objects);
  camwin->CurrentObjectsChanged (current_objects);
}

bool AppAresEdit::TraceBeamTerrain (const csVector3& start,
    const csVector3& end, csVector3& isect)
{
  if (!terrainMesh) return false;
  csHitBeamResult result = terrainMesh->HitBeam (start, end);
  isect = result.isect;
  return result.hit;
}

csSegment3 AppAresEdit::GetBeam (int x, int y, float maxdist)
{
  iCamera* cam = GetCsCamera ();
  csVector2 v2d (x, GetG2D ()->GetHeight () - y);
  csVector3 v3d = cam->InvPerspective (v2d, maxdist);
  csVector3 start = cam->GetTransform ().GetOrigin ();
  csVector3 end = cam->GetTransform ().This2Other (v3d);
  return csSegment3 (start, end);
}

csSegment3 AppAresEdit::GetMouseBeam (float maxdist)
{
  return GetBeam (mouseX, mouseY, maxdist);
}

iRigidBody* AppAresEdit::TraceBeam (const csSegment3& beam, csVector3& isect)
{
  // Trace the physical beam
  iRigidBody* hitBody = 0;
  CS::Physics::Bullet::HitBeamResult result = GetBulletSystem ()->HitBeam (
      beam.Start (), beam.End ());
  if (result.body)
  {
    hitBody = result.body->QueryRigidBody ();
    isect = result.isect;
  }
  else
  {
    printf ("Work around needed!\n"); fflush (stdout);
    // @@@ This is a workaround for the fact that bullet->HitBeam() doesn't appear to work
    // on mesh colliders.
    csSectorHitBeamResult result2 = GetCsCamera ()->GetSector ()->HitBeamPortals (
	beam.Start (), beam.End ());
    if (result2.mesh)
    {
      iDynamicObject* dynobj = GetDynamicWorld ()->FindObject (result2.mesh);
      if (dynobj)
      {
        hitBody = dynobj->GetBody ();
        isect = result2.isect;
      }
    }
  }
  return hitBody;
}

bool AppAresEdit::OnMouseDown (iEvent& ev)
{
  uint but = csMouseEventHelper::GetButton (&ev);
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

  if (camera.OnMouseDown (ev, but, mouseX, mouseY))
    return true;

  return editMode->OnMouseDown (ev, but, mouseX, mouseY);
}

bool AppAresEdit::OnMouseUp (iEvent& ev)
{
  uint but = csMouseEventHelper::GetButton (&ev);
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

  if (camera.OnMouseUp (ev, but, mouseX, mouseY))
    return true;

  return editMode->OnMouseUp (ev, but, mouseX, mouseY);
}

csVector3 AppAresEdit::GetCenterSelected ()
{
  csVector3 center (0);
  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    const csBox3& box = dynobj->GetFactory ()->GetBBox ();
    const csReversibleTransform& tr = dynobj->GetTransform ();
    center += tr.This2Other (box.GetCenter ());
  }
  center /= selection->GetSize ();
  return center;
}

bool AppAresEdit::OnKeyboard(iEvent& ev)
{
  if (csKeyEventHelper::GetEventType(&ev) == csKeyEventTypeDown)
  {
    // The user pressed a key (as opposed to releasing it).
    utf32_char code = csKeyEventHelper::GetCookedCode(&ev);
    if (code == CSKEY_ESC)
    {
      // The user pressed escape, so terminate the application.  The proper way
      // to terminate a Crystal Space application is by broadcasting a
      // csevQuit event.  That will cause the main run loop to stop.  To do
      // so we retrieve the event queue from the object registry and then post
      // the event.
      csRef<iEventQueue> q =
        csQueryRegistry<iEventQueue> (GetObjectRegistry());
      if (q.IsValid())
        q->GetEventOutlet()->Broadcast(csevQuit(GetObjectRegistry()));
      return true;
    }
    else if (code == '1')
    {
      do_debug = !do_debug;
    }
    else if (code == CSKEY_F2)
    {
      currentTime += 500;
    }
    else if (code == CSKEY_F3)
    {
      do_auto_time = !do_auto_time;
    }
    else if (code == '.')
    {
      if (GetCamera ().IsPanningEnabled ())
      {
	GetCamera ().DisablePanning ();
      }
      else if (selection->HasSelection ())
      {
        csVector3 center = GetCenterSelected ();
        GetCamera ().EnablePanning (center);
      }
    }
    else if (code == 'c')
    {
      if (camwin->IsVisible ())
	camwin->Hide ();
      else
	camwin->Show ();
    }
    else
      return editMode->OnKeyboard (ev, code);
  }
  return false;
}

//---------------------------------------------------------------------------

bool AppAresEdit::InitWindowSystem ()
{
  cegui = csQueryRegistry<iCEGUI> (GetObjectRegistry());
  if (!cegui) return ReportError("Failed to locate CEGUI plugin");

  cegui->Initialize ();

  vfs->ChDir ("/cegui/");

  // Load the ice skin (which uses Falagard skinning system)
  cegui->GetSchemeManagerPtr ()->create("ice.scheme");

  cegui->GetSystemPtr ()->setDefaultMouseCursor("ice", "MouseArrow");

  cegui->GetFontManagerPtr ()->createFreeTypeFont("DejaVuSans", 10, true, "/fonts/ttf/DejaVuSans.ttf");

  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  // Load layout and set as root
  vfs->ChDir ("/this/data/windows/");
  cegui->GetSystemPtr ()->setGUISheet(winMgr->loadWindowLayout("ice.layout"));

  CEGUI::Window* btn;

  filenameLabel = winMgr->getWindow("Ares/StateWindow/File");

  btn = winMgr->getWindow("Ares/StateWindow/Save");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&AppAresEdit::OnSaveButtonClicked, this));
  btn = winMgr->getWindow("Ares/StateWindow/Load");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&AppAresEdit::OnLoadButtonClicked, this));
  undoButton = static_cast<CEGUI::PushButton*>(winMgr->getWindow("Ares/StateWindow/Undo"));
  undoButton->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&AppAresEdit::OnUndoButtonClicked, this));
  undoButton->disable ();

  mainTabButton = static_cast<CEGUI::TabButton*>(winMgr->getWindow("Ares/StateWindow/MainTab"));
  mainTabButton->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&AppAresEdit::OnMainTabButtonClicked, this));
  mainTabButton->setTargetWindow(winMgr->getWindow("Ares/ItemWindow"));
  curveTabButton = static_cast<CEGUI::TabButton*>(winMgr->getWindow("Ares/StateWindow/CurveTab"));
  curveTabButton->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&AppAresEdit::OnCurveTabButtonClicked, this));
  curveTabButton->setTargetWindow(winMgr->getWindow("Ares/CurveWindow"));

  simulationCheck = static_cast<CEGUI::Checkbox*>(winMgr->getWindow("Ares/StateWindow/Simulation"));
  simulationCheck->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged,
    CEGUI::Event::Subscriber(&AppAresEdit::OnSimulationSelected, this));

  filereq = new FileReq (cegui, vfs, "/saves");
  camwin = new CameraWindow (this, cegui);

  mainMode = new MainMode (this);
  curveMode = new CurveMode (this);
  editMode = mainMode;
  editMode->Start ();
  mainTabButton->setSelected(true);

  return true;
}

bool AppAresEdit::OnSimulationSelected (const CEGUI::EventArgs&)
{
  do_simulation = simulationCheck->isSelected ();
  return true;
}

void AppAresEdit::DeleteSelectedObjects ()
{
  csArray<iDynamicObject*> objects = selection->GetObjects ();
  selection->SetCurrentObject (0);
  SelectionIterator it = objects.GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dynworld->DeleteObject (dynobj);
  }
}

void AppAresEdit::MoveCurrent (const csVector3& baseVector)
{
  bool slow = kbd->GetKeyState (CSKEY_CTRL);
  bool fast = kbd->GetKeyState (CSKEY_SHIFT);
  csVector3 vector = baseVector;
  if (slow) vector *= 0.01f;
  else if (!fast) vector *= 0.1f;

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    csReversibleTransform& trans = mesh->GetMovable ()->GetTransform ();
    trans.Translate (trans.This2OtherRelative (vector));
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

void AppAresEdit::RotateCurrent (float baseAngle)
{
  bool slow = kbd->GetKeyState (CSKEY_CTRL);
  bool fast = kbd->GetKeyState (CSKEY_SHIFT);
  float angle = baseAngle;
  if (slow) angle /= 180.0;
  else if (fast) angle /= 2.0;
  else angle /= 8.0;

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    mesh->GetMovable ()->GetTransform ().RotateOther (csVector3 (0, 1, 0), angle);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

static float GetAngle (float x1, float y1, float x2, float y2)
{
  float dot = x1 * x2 + y1 * y2;
  float angle = acos (dot);
  return angle;
}

static csVector3 FindBiggestHorizontalMovement (
    const csBox3& box1, const csReversibleTransform& trans1,
    const csBox3& box2, const csReversibleTransform& trans2)
{
  // The origin of the second box in the 3D space of the first box.
  csVector3 o2Tr = trans1.Other2This (trans2.GetOrigin ());
  // Transformed bounding box.
  csBox3 box2Tr;
  box2Tr.StartBoundingBox (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (0))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (1))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (2))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (3))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (4))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (5))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (6))));
  box2Tr.AddBoundingVertexSmart (trans1.Other2ThisRelative (trans2.This2OtherRelative (box2.GetCorner (7))));

  float xr = o2Tr.x - box1.MaxX () + box2Tr.MinX ();
  float xl = box1.MinX () - o2Tr.x - box2Tr.MaxX ();
  float zr = o2Tr.z - box1.MaxZ () + box2Tr.MinZ ();
  float zl = box1.MinZ () - o2Tr.z - box2Tr.MaxZ ();
  csVector3 newpos = trans2.GetOrigin ();
  if (xr >= xl && xr >= zr && xr >= zl)
    newpos = trans1.This2Other (o2Tr + csVector3 (-xr, 0, -o2Tr.z));
  else if (xl >= xr && xl >= zr && xl >= zl)
    newpos = trans1.This2Other (o2Tr + csVector3 (xl, 0, -o2Tr.z));
  else if (zr >= xl && zr >= xr && zr >= zl)
    newpos = trans1.This2Other (o2Tr + csVector3 (-o2Tr.x, 0, -zr));
  else if (zl >= xl && zl >= xr && zl >= zr)
    newpos = trans1.This2Other (o2Tr + csVector3 (-o2Tr.x, 0, zl));
  return newpos;
}

/**
 * Modify the slave transform so that it is nicely aligned to the master
 * transform (horizontally) but with the smallest possible rotation.
 */
static void FindBestAlignedTransform (const csReversibleTransform& masterTrans,
    csReversibleTransform& slaveTrans)
{
  csReversibleTransform masterCopy = masterTrans;
  int i = 4;
  while (i > 0)
  {
    csVector3 front = masterCopy.GetFront ();
    csVector3 fr = slaveTrans.GetFront ();
    float angle = GetAngle (front.x, front.z, fr.x, fr.z);
    if (fabs (angle) <= (PI / 4.0))
    {
      slaveTrans.SetO2T (masterCopy.GetO2T ());
      return;
    }
    i--;
    masterCopy.RotateOther (csVector3 (0, 1, 0), PI/2.0);
  }
}

void AppAresEdit::AlignSelectedObjects ()
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  const csReversibleTransform& trans = selection->GetFirst ()->GetMesh ()->GetMovable ()->GetTransform ();

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;
    dynobj->MakeKinematic ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    FindBestAlignedTransform (trans, tr);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

void AppAresEdit::StackSelectedObjects ()
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  csReversibleTransform firstTrans = selection->GetFirst ()->GetMesh ()->GetMovable ()->GetTransform ();
  csBox3 firstBbox = selection->GetFirst ()->GetFactory ()->GetPhysicsBBox ();

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;
    dynobj->MakeKinematic ();
    const csBox3& bbox = dynobj->GetFactory ()->GetPhysicsBBox ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    csVector3 v = tr.GetOrigin ();
    v.y = firstTrans.GetOrigin ().y + firstBbox.MaxY () - bbox.MinY ();
    tr.SetOrigin (v);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();

    // Next stack we perform relative to the previous one we stacked.
    firstTrans = tr;
    firstBbox = bbox;
  }
}

void AppAresEdit::SameYSelectedObjects ()
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  csReversibleTransform trans = selection->GetFirst ()->GetMesh ()->GetMovable ()->GetTransform ();
  float y = trans.GetOrigin ().y;

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;
    dynobj->MakeKinematic ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    csVector3 v = tr.GetOrigin ();
    v.y = y;
    tr.SetOrigin (v);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

void AppAresEdit::SetPosSelectedObjects ()
{
  if (selection->GetSize () <= 1) return;
  if (!selection->GetFirst ()->GetMesh ()) return;

  csReversibleTransform firstTrans = selection->GetFirst ()->GetMesh ()->GetMovable ()
    ->GetTransform ();
  csBox3 firstBbox = selection->GetFirst ()->GetFactory ()->GetPhysicsBBox ();

  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (dynobj == selection->GetFirst ()) continue;
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    csReversibleTransform& tr = mesh->GetMovable ()->GetTransform ();
    const csBox3& bbox = dynobj->GetFactory ()->GetPhysicsBBox ();
    csVector3 newpos = FindBiggestHorizontalMovement (firstBbox, firstTrans, bbox, tr);
    tr.SetOrigin (newpos);
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();

    // Next SetPos we perform relative to the previous one we stacked.
    firstTrans = tr;
    firstBbox = bbox;
  }
}

void AppAresEdit::RotResetSelectedObjects ()
{
  SelectionIterator it = selection->GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    iMeshWrapper* mesh = dynobj->GetMesh ();
    if (!mesh) continue;

    dynobj->MakeKinematic ();
    mesh->GetMovable ()->GetTransform ().LookAt (csVector3 (0, 0, 1), csVector3 (0, 1, 0));
    mesh->GetMovable ()->UpdateMove ();
    dynobj->UndoKinematic ();
  }
}

bool AppAresEdit::OnMainTabButtonClicked (const CEGUI::EventArgs&)
{
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();
  mainTabButton->setSelected(true);
  curveTabButton->setSelected(false);
  winMgr->getWindow("Ares/ItemWindow")->setVisible(true);
  winMgr->getWindow("Ares/CurveWindow")->setVisible(false);
  if (editMode) editMode->Stop ();
  editMode = mainMode;
  editMode->Start ();
  return true;
}

bool AppAresEdit::OnCurveTabButtonClicked (const CEGUI::EventArgs&)
{
  return SwitchToCurveMode ();
}

bool AppAresEdit::SwitchToCurveMode ()
{
  if (selection->GetSize () != 1) return true;
  csString name = selection->GetFirst ()->GetFactory ()->GetName ();
  if (!curvedMeshCreator->GetCurvedFactory (name)) return true;

  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();
  mainTabButton->setSelected(false);
  curveTabButton->setSelected(true);
  winMgr->getWindow("Ares/ItemWindow")->setVisible(false);
  winMgr->getWindow("Ares/CurveWindow")->setVisible(true);
  if (editMode) editMode->Stop ();
  editMode = curveMode;
  editMode->Start ();
  return true;
}

void AppAresEdit::CleanupWorld ()
{
  selection->SetCurrentObject (0);

  curvedFactories.DeleteAll ();
  factory_to_origin_offset.DeleteAll ();
  curvedFactoryCreators.DeleteAll ();
  static_factories.DeleteAll ();

  camlight = 0;
  engine->DeleteAll ();
}

bool AppAresEdit::OnUndoButtonClicked (const CEGUI::EventArgs&)
{
  return true;
}

void AppAresEdit::SaveFile (const char* filename)
{
  filenameLabel->setText (CEGUI::String (filename));
  // @@@ Error handling.
  worldLoader->SaveFile (filename);
}

struct SaveCallback : public OKCallback
{
  AppAresEdit* ares;
  SaveCallback (AppAresEdit* ares) : ares (ares) { }
  virtual void OkPressed (const char* filename)
  {
    ares->SaveFile (filename);
  }
};

bool AppAresEdit::OnSaveButtonClicked (const CEGUI::EventArgs&)
{
  filereq->Show (new SaveCallback (this));
  return true;
}

void AppAresEdit::LoadFile (const char* filename)
{
  CleanupWorld ();
  SetupWorld ();

  // @@@ Error handling.
  worldLoader->LoadFile (filename);

  sector = engine->FindSector ("room");

  for (size_t i = 0 ; i < curvedMeshCreator->GetCurvedFactoryCount () ; i++)
  {
    iCurvedFactory* cfact = curvedMeshCreator->GetCurvedFactory (i);
    static_factories.Add (cfact->GetName ());
  }
  curvedFactories = worldLoader->GetCurvedFactories ();

  // @@@ Error handling.
  SetupDynWorld ();

  // @@@ Error handling.
  PostLoadMap ();
}

struct LoadCallback : public OKCallback
{
  AppAresEdit* ares;
  LoadCallback (AppAresEdit* ares) : ares (ares) { }
  virtual void OkPressed (const char* filename)
  {
    ares->LoadFile (filename);
  }
};

bool AppAresEdit::OnLoadButtonClicked (const CEGUI::EventArgs&)
{
  filereq->Show (new LoadCallback (this));
  return true;
}

bool AppAresEdit::OnInitialize(int argc, char* argv[])
{
  iObjectRegistry* r = GetObjectRegistry();

  // Load application-specific configuration file.
  if (!csInitializer::SetupConfigManager(r,
      "/ares/AppAres.cfg", GetApplicationName()))
    return ReportError("Failed to initialize configuration manager!");

#ifdef USE_CEL
  celInitializer::SetupCelPluginDirs(r);
#endif

  // RequestPlugins() will load all plugins we specify.  In addition it will
  // also check if there are plugins that need to be loaded from the
  // configuration system (both the application configuration and CS or global
  // configurations).  It also supports specifying plugins on the command line
  // via the --plugin= option.
  if (!csInitializer::RequestPlugins(r,
	CS_REQUEST_VFS,
	CS_REQUEST_OPENGL3D,
	CS_REQUEST_ENGINE,
	CS_REQUEST_FONTSERVER,
	CS_REQUEST_IMAGELOADER,
	CS_REQUEST_LEVELLOADER,
	CS_REQUEST_REPORTER,
	CS_REQUEST_REPORTERLISTENER,
	CS_REQUEST_PLUGIN("crystalspace.collisiondetection.opcode", iCollideSystem),
	CS_REQUEST_PLUGIN("crystalspace.dynamics.bullet", iDynamics),
	CS_REQUEST_PLUGIN("crystalspace.cegui.wrapper", iCEGUI),
	CS_REQUEST_PLUGIN("crystalspace.decal.manager", iDecalManager),
	CS_REQUEST_PLUGIN("utility.dynamicworld", iDynamicWorld),
	CS_REQUEST_PLUGIN("utility.nature", iNature),
	CS_REQUEST_PLUGIN("utility.curvemesh", iCurvedMeshCreator),
	CS_REQUEST_END))
    return ReportError("Failed to initialize plugins!");

  // "Warm up" the event handler so it can interact with the world
  csBaseEventHandler::Initialize(GetObjectRegistry());

  FocusLost = csevFocusLost (GetObjectRegistry ());
 
  return true;
}

void AppAresEdit::OnExit()
{
  printer.Invalidate();
}

void AppAresEdit::AddItem (const char* category, const char* itemname)
{
  if (!categories.In (category))
  {
    categories.Put (category, csStringArray());
    mainMode->AddCategory (category);
  }
  csStringArray a;
  categories.Get (category, a).Push (itemname);
}

#if USE_DECAL
bool AppAresEdit::SetupDecal ()
{
  iMaterialWrapper* material = engine->GetMaterialList ()->FindByName ("stone2");
  if (!material)
    return ReportError("Can't find cursor decal material!");
  cursorDecalTemplate = decalMgr->CreateDecalTemplate (material);
  cursorDecal = 0;
  return true;
}
#endif

bool AppAresEdit::Application()
{
  iObjectRegistry* r = GetObjectRegistry();

  // Open the main system. This will open all the previously loaded plugins
  // (i.e. all windows will be opened).
  if (!OpenApplication(r))
    return ReportError("Error opening system!");

  vfs = csQueryRegistry<iVFS> (r);
  if (!vfs)
    return ReportError("Failed to locate vfs!");

  if (!InitWindowSystem ())
    return false;

  // Set up an event handler for the application.  Crystal Space is fully
  // event-driven.  Everything (except for this initialization) happens in
  // response to an event.
  if (!RegisterQueue (r, csevAllEvents(GetObjectRegistry())))
    return ReportError("Failed to set up event handler!");

  // Now get the pointer to various modules we need.  We fetch them from the
  // object registry.  The RequestPlugins() call we did earlier registered all
  // loaded plugins with the object registry.  It is also possible to load
  // plugins manually on-demand.
  g3d = csQueryRegistry<iGraphics3D> (r);
  if (!g3d)
    return ReportError("Failed to locate 3D renderer!");

  dynworld = csQueryRegistry<iDynamicWorld> (r);
  if (!dynworld) return ReportError("Failed to locate dynamic world plugin!");

  nature = csQueryRegistry<iNature> (r);
  if (!nature) return ReportError("Failed to locate nature plugin!");

  curvedMeshCreator = csQueryRegistry<iCurvedMeshCreator> (r);
  if (!curvedMeshCreator)
    return ReportError("Failed to load the curved mesh creator plugin!");

  engine = csQueryRegistry<iEngine> (r);
  if (!engine)
    return ReportError("Failed to locate 3D engine!");

  decalMgr = csQueryRegistry<iDecalManager> (r);
  if (!decalMgr)
    return ReportError("Failed to load decal manager!");

  printer.AttachNew(new FramePrinter(GetObjectRegistry()));

  vc = csQueryRegistry<iVirtualClock> (r);
  if (!vc)
    return ReportError ("Failed to locate Virtual Clock!");

  kbd = csQueryRegistry<iKeyboardDriver> (r);
  if (!kbd)
    return ReportError ("Failed to locate Keyboard Driver!");

  loader = csQueryRegistry<iLoader> (r);
  if (!loader)
    return ReportError("Failed to locate map loader plugin!");

  cdsys = csQueryRegistry<iCollideSystem> (r);
  if (!cdsys)
    return ReportError ("Failed to locate CD system!");

  cfgmgr = csQueryRegistry<iConfigManager> (r);
  if (!cfgmgr)
    return ReportError ("Failed to locate the configuration manager plugin!");

  worldLoader = new WorldLoader (r);
  selection = new Selection (this);
  SelectionListener* listener = new AresEditSelectionListener (this);
  selection->AddSelectionListener (listener);
  listener->DecRef ();

  colorWhite = g3d->GetDriver2D ()->FindRGB (255, 255, 255);
  font = g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_COURIER);

  // We need a View to the virtual world.
  view.AttachNew(new csView (engine, g3d));
  iGraphics2D* g2d = g3d->GetDriver2D ();
  // We use the full window to draw the world.
  view_width = (int)(g2d->GetWidth () * 0.86);
  view_height = g2d->GetHeight ();
  view->SetRectangle (0, 0, view_width, view_height);

  // Set the window title.
  iNativeWindow* nw = g2d->GetNativeWindow ();
  if (nw)
    nw->SetTitle (cfgmgr->GetStr ("WindowTitle",
          "Please set WindowTitle in AppAresEdit.cfg"));

  if (!InitPhysics ())
    return false;

  if (!SetupWorld ())
    return false;

  if (!PostLoadMap ())
    return ReportError ("Error during PostLoadMap()!");

  if (!SetupDynWorld ())
    return false;

#if USE_DECAL
  if (!SetupDecal ())
    return false;
#endif

  // Start the default run/event loop.  This will return only when some code,
  // such as OnKeyboard(), has asked the run loop to terminate.
  Run();

  return true;
}

bool AppAresEdit::SetupDynWorld ()
{
  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    if (curvedFactories.Find (fact) != csArrayItemNotFound) continue;
    printf ("%d %s\n", i, fact->GetName ()); fflush (stdout);
    csBox3 bbox = fact->GetPhysicsBBox ();
    factory_to_origin_offset.Put (fact->GetName (), bbox.MinY ());
    const char* st = fact->GetAttribute ("defaultstatic");
    if (st && *st == 't') static_factories.Add (fact->GetName ());
    const char* category = fact->GetAttribute ("category");
    AddItem (category, fact->GetName ());
  }

  iDynamicFactory* fact = dynworld->AddFactory ("Node", 1.0, -1);
  fact->AddRigidBox (csVector3 (0.0f), csVector3 (0.2f), 1.0f);
  AddItem ("Nodes", "Node");

  for (size_t i = 0 ; i < curvedMeshCreator->GetCurvedFactoryTemplateCount () ; i++)
  {
    iCurvedFactoryTemplate* cft = curvedMeshCreator->GetCurvedFactoryTemplate (i);
    const char* category = cft->GetAttribute ("category");
    AddItem (category, cft->GetName ());

    CurvedFactoryCreator creator;
    creator.name = cft->GetName ();
    const char* maxradiusS = cft->GetAttribute ("maxradius");
    csScanStr (maxradiusS, "%f", &creator.maxradius);
    const char* imposterradiusS = cft->GetAttribute ("imposterradius");
    csScanStr (imposterradiusS, "%f", &creator.imposterradius);
    const char* massS = cft->GetAttribute ("mass");
    csScanStr (massS, "%f", &creator.mass);

    curvedFactoryCreators.Push (creator);
  }
  return true;
}

bool AppAresEdit::PostLoadMap ()
{
  dynworld->Setup (sector, dynSys);

  // Initialize collision objects for all loaded objects.
  csColliderHelper::InitializeCollisionWrappers (cdsys, engine);

  // @@@ Bad: hardcoded terrain name! Don't do this at home!
  terrainMesh = sector->GetMeshes ()->FindByName ("Terrain");

  if (terrainMesh)
  {
    csRef<iTerrainSystem> terrain =
      scfQueryInterface<iTerrainSystem> (terrainMesh->GetMeshObject ());
    if (!terrain)
    {
      ReportError("Error cannot find the terrain interface!");
      return false;
    }

    // Create a terrain collider for each cell of the terrain
    for (size_t i = 0; i < terrain->GetCellCount (); i++)
      bullet_dynSys->AttachColliderTerrain (terrain->GetCell (i));
  }

  iLightList* lightList = sector->GetLights ();
  lightList->RemoveAll ();

  nature->InitSector (sector);

  camlight = engine->CreateLight(0, csVector3(0.0f, 0.0f, 0.0f), 10, csColor (0.8f, 0.9f, 1.0f));
  lightList->Add (camlight);

  engine->Prepare ();
  //CS::Lighting::SimpleStaticLighter::ShineLights (sector, engine, 4);

  // Setup the camera.
  camera.Init (view->GetCamera (), sector, csVector3 (0, 10, 0));

  return true;
}

bool AppAresEdit::SetupWorld ()
{
  vfs->Mount ("/aresnode", "data$/node.zip");
  if (!LoadLibrary ("/aresnode/", "library"))
    return ReportError ("Error loading library!");
  vfs->PopDir ();
  vfs->Unmount ("/aresnode", "data$/node.zip");

  sector = engine->CreateSector ("room");

  return true;
}

CurvedFactoryCreator* AppAresEdit::FindFactoryCreator (const char* name)
{
  for (size_t i = 0 ; i < curvedFactoryCreators.GetSize () ; i++)
    if (curvedFactoryCreators[i].name == name)
      return &curvedFactoryCreators[i];
  return 0;
}

static float TestVerticalBeam (const csVector3& start, float distance, iCamera* camera)
{
  csVector3 end = start;
  end.y -= distance;
  iSector* sector = camera->GetSector ();

  csSectorHitBeamResult result = sector->HitBeamPortals (start, end);
  if (result.mesh)
    return result.isect.y;
  else
    return end.y-.1;
}

void AppAresEdit::SpawnItem (const csString& name)
{
  csString fname;
  iCurvedFactory* curvedFactory = 0;
  CurvedFactoryCreator* cfc = FindFactoryCreator (name);
  if (cfc)
  {
    curvedFactoryCounter++;
    fname.Format("%s%d", name.GetData (), curvedFactoryCounter);
    curvedFactory = curvedMeshCreator->AddCurvedFactory (fname, name);

    iDynamicFactory* fact = dynworld->AddFactory (fname, cfc->maxradius, cfc->imposterradius);
    csRef<iGeometryGenerator> ggen = scfQueryInterface<iGeometryGenerator> (curvedFactory);
    if (ggen)
      fact->SetGeometryGenerator (ggen);

    fact->AddRigidMesh (csVector3 (0), cfc->mass);
    static_factories.Add (fname);
    curvedFactories.Push (fact);
  }
  else
  {
    fname = name;
  }

  // Use the camera transform.
  csSegment3 beam = GetMouseBeam (50.0f);
  csSectorHitBeamResult result = sector->HitBeamPortals (beam.Start (), beam.End ());

  float yorigin = factory_to_origin_offset.Get (fname, 1000000.0);

  csVector3 newPosition;
  if (result.mesh)
  {
    newPosition = result.isect;
    if (yorigin < 999999.0)
      newPosition.y -= yorigin;
  }
  else
  {
    newPosition = beam.End () - beam.Start ();
    newPosition.Normalize ();
    newPosition = GetCsCamera ()->GetTransform ().GetOrigin () + newPosition * 3.0f;
  }

  csReversibleTransform tc = GetCsCamera ()->GetTransform ();
  csVector3 front = tc.GetFront ();
  front.y = 0;
  tc.LookAt (front, csVector3 (0, 1, 0));
  tc.SetOrigin (newPosition);
  iDynamicObject* dynobj = dynworld->AddObject (fname, tc);

  if (!static_factories.In (fname))
  {
    // For a dynamic object we make sure the object is above the ground on
    // all four corners too. This is to ensure that the object doesn't jump
    // up suddenly because it was embedded in the ground partially.
    const csBox3& box = dynobj->GetBBox ();
    float dist = (box.MaxY () - box.MinY ()) * 2.0;
    float y1 = TestVerticalBeam (box.GetCorner (CS_BOX_CORNER_xYz), dist, GetCsCamera ());
    if (yorigin < 999999.0) y1 -= yorigin;
    float y2 = TestVerticalBeam (box.GetCorner (CS_BOX_CORNER_XYz), dist, GetCsCamera ());
    if (yorigin < 999999.0) y2 -= yorigin;
    float y3 = TestVerticalBeam (box.GetCorner (CS_BOX_CORNER_xYZ), dist, GetCsCamera ());
    if (yorigin < 999999.0) y3 -= yorigin;
    float y4 = TestVerticalBeam (box.GetCorner (CS_BOX_CORNER_XYZ), dist, GetCsCamera ());
    if (yorigin < 999999.0) y4 -= yorigin;
    bool changed = false;
    if (y1 > newPosition.y) { newPosition.y = y1; changed = true; }
    if (y2 > newPosition.y) { newPosition.y = y2; changed = true; }
    if (y3 > newPosition.y) { newPosition.y = y3; changed = true; }
    if (y4 > newPosition.y) { newPosition.y = y4; changed = true; }
    if (changed)
    {
      dynobj->MakeKinematic ();
      csReversibleTransform trans = dynobj->GetTransform ();
      printf ("Changed: orig=%g new=%g\n", trans.GetOrigin ().y, newPosition.y); fflush (stdout);
      trans.SetOrigin (newPosition);
      dynobj->SetTransform (trans);
      dynobj->UndoKinematic ();
    }
  }

  if (static_factories.In (fname))
    dynobj->MakeStatic ();
  selection->SetCurrentObject (dynobj);

  if (curvedFactory)
    SwitchToCurveMode ();
}

bool AppAresEdit::InitPhysics ()
{
  dyn = csQueryRegistry<iDynamics> (GetObjectRegistry ());
  if (!dyn) return ReportError ("Error loading bullet plugin!");

  dynSys = dyn->CreateSystem ();
  if (!dynSys) return ReportError ("Error creating dynamic system!");
  //dynSys->SetLinearDampener(.3f);
  dynSys->SetRollingDampener(.995f);
  dynSys->SetGravity (csVector3 (0.0f, -9.81f, 0.0f));

  bullet_dynSys = scfQueryInterface<CS::Physics::Bullet::iDynamicSystem> (dynSys);
  bullet_dynSys->SetInternalScale (1.0f);
  bullet_dynSys->SetStepParameters (0.005f, 2, 10);

  return true;
}

bool AppAresEdit::LoadLibrary (const char* path, const char* file)
{
  // Set current VFS dir to the level dir, helps with relative paths in maps
  vfs->PushDir (path);
  csLoadResult rc = loader->Load (file);
  if (!rc.success)
  {
    vfs->PopDir ();
    return ReportError("Couldn't load library file %s!", path);
  }
  vfs->PopDir ();
  return true;
}
