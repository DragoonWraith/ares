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
#include <celtool/initapp.h>
#include <cstool/simplestaticlighter.h>
#include <csgeom/math3d.h>
#include "camerawin.h"
#include "selection.h"
#include "common/worldload.h"
#include "transformtools.h"

#include "celtool/persisthelper.h"
#include "physicallayer/pl.h"

/* Fun fact: should occur after csutil/event.h, otherwise, gcc may report
 * missing csMouseEventHelper symbols. */
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

void AresEditSelectionListener::SelectionChanged (
    const csArray<iDynamicObject*>& current_objects)
{
  aresed3d->SelectionChanged (current_objects);
}

void MainModeSelectionListener::SelectionChanged (
    const csArray<iDynamicObject*>& current_objects)
{
  mainMode->CurrentObjectsChanged (current_objects);
}

struct LoadCallback : public OKCallback
{
  AppAresEditWX* ares;
  LoadCallback (AppAresEditWX* ares) : ares (ares) { }
  virtual void OkPressed (const char* filename)
  {
    ares->LoadFile (filename);
  }
};

// =========================================================================

AresEdit3DView::AresEdit3DView (iObjectRegistry* object_reg) :
  object_reg (object_reg),camera (0, this)
{
  do_debug = false;
  do_simulation = true;
  camwin = 0;
  currentTime = 31000;
  do_auto_time = false;
  curvedFactoryCounter = 0;
  roomFactoryCounter = 0;
  worldLoader = 0;
  selection = 0;
  FocusLost = csevFocusLost (object_reg);
}

AresEdit3DView::~AresEdit3DView()
{
  delete camwin;
  delete worldLoader;
  delete selection;
}

void AresEdit3DView::DoStuffOncePerFrame ()
{
  // First get elapsed time from the virtual clock.
  float elapsed_time = vc->GetElapsedSeconds ();
  nature->UpdateTime (currentTime, GetCsCamera ());
  if (do_auto_time)
    currentTime += csTicks (elapsed_time * 1000);

  camera.Frame (elapsed_time, mouseX, mouseY);

  csReversibleTransform tc = GetCsCamera ()->GetTransform ();
  //csVector3 pos = tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, .5);
  csVector3 pos = tc.GetOrigin () + tc.GetT2O () * csVector3 (2, 0, 2);
  camlight->GetMovable ()->GetTransform ().SetOrigin (pos);
  camlight->GetMovable ()->UpdateMove ();

  if (do_simulation)
  {
    float dynamicSpeed = 1.0f;
    dyn->Step (elapsed_time / dynamicSpeed);
  }

  dynworld->PrepareView (GetCsCamera (), elapsed_time);
}

void AresEdit3DView::Frame (EditingMode* editMode)
{
  DoStuffOncePerFrame ();

  if (!g3d->BeginDraw(CSDRAW_3DGRAPHICS)) return;

  view->Draw ();

  editMode->Frame3D ();
  markerMgr->Frame3D ();

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
  markerMgr->Frame2D ();
}

bool AresEdit3DView::OnMouseMove (iEvent& ev)
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

  if (markerMgr->OnMouseMove (ev, mouseX, mouseY))
    return true;

  if (camera.OnMouseMove (ev, mouseX, mouseY))
    return true;

  return false;
}

bool AresEdit3DView::OnUnhandledEvent (iEvent& event)
{
  if (event.Name == FocusLost)
  {
    camera.OnFocusLost ();
    return true;
  }
  return false;
}

void AresEdit3DView::SetStaticSelectedObjects (bool st)
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

#if 0
void AresEdit3DView::SetButtonState ()
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

  bool roomTabEnable = false;
  if (selection->GetSize () == 1)
  {
    csString name = selection->GetFirst ()->GetFactory ()->GetName ();
    if (!roomMeshCreator->GetRoomFactory (name))
      roomTabEnable = true;
  }
  if (roomTabEnable)
    roomTabButton->enable ();
  else
    roomTabButton->disable ();
}
#endif

void AresEdit3DView::SelectionChanged (const csArray<iDynamicObject*>& current_objects)
{
  camwin->CurrentObjectsChanged (current_objects);
}

bool AresEdit3DView::TraceBeamTerrain (const csVector3& start,
    const csVector3& end, csVector3& isect)
{
  if (!terrainMesh) return false;
  csHitBeamResult result = terrainMesh->HitBeam (start, end);
  isect = result.isect;
  return result.hit;
}

csSegment3 AresEdit3DView::GetBeam (int x, int y, float maxdist)
{
  iCamera* cam = GetCsCamera ();
  csVector2 v2d (x, GetG2D ()->GetHeight () - y);
  csVector3 v3d = cam->InvPerspective (v2d, maxdist);
  csVector3 start = cam->GetTransform ().GetOrigin ();
  csVector3 end = cam->GetTransform ().This2Other (v3d);
  return csSegment3 (start, end);
}

csSegment3 AresEdit3DView::GetMouseBeam (float maxdist)
{
  return GetBeam (mouseX, mouseY, maxdist);
}

iRigidBody* AresEdit3DView::TraceBeam (const csSegment3& beam, csVector3& isect)
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
      iDynamicObject* dynobj = dynworld->FindObject (result2.mesh);
      if (dynobj)
      {
        hitBody = dynobj->GetBody ();
        isect = result2.isect;
      }
    }
  }
  return hitBody;
}

bool AresEdit3DView::OnMouseDown (iEvent& ev)
{
  uint but = csMouseEventHelper::GetButton (&ev);
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

  if (markerMgr->OnMouseDown (ev, but, mouseX, mouseY))
    return true;

  if (camera.OnMouseDown (ev, but, mouseX, mouseY))
    return true;

  return false;
}

bool AresEdit3DView::OnMouseUp (iEvent& ev)
{
  uint but = csMouseEventHelper::GetButton (&ev);
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

  if (markerMgr->OnMouseUp (ev, but, mouseX, mouseY))
    return true;

  if (camera.OnMouseUp (ev, but, mouseX, mouseY))
    return true;

  return false;
}

bool AresEdit3DView::OnKeyboard(iEvent& ev)
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
      eventQueue->GetEventOutlet()->Broadcast(csevQuit(GetObjectRegistry()));
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
    else if (code == CSKEY_F4)
    {
      nature->SetFoliageDensityFactor (nature->GetFoliageDensityFactor ()-.05);
    }
    else if (code == CSKEY_F5)
    {
      nature->SetFoliageDensityFactor (nature->GetFoliageDensityFactor ()+.05);
    }
    else if (code == '.')
    {
      if (GetCamera ().IsPanningEnabled ())
      {
	GetCamera ().DisablePanning ();
      }
      else if (selection->HasSelection ())
      {
        csVector3 center = TransformTools::GetCenterSelected (selection);
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
    else return false;
    return true;
  }
  return false;
}

//---------------------------------------------------------------------------

void AresEdit3DView::DeleteSelectedObjects ()
{
  csArray<iDynamicObject*> objects = selection->GetObjects ();
  selection->SetCurrentObject (0);
  SelectionIterator it = objects.GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dyncell->DeleteObject (dynobj);
  }
}

bool AresEdit3DView::SwitchToCurveMode ()
{
  if (selection->GetSize () != 1) return true;
  csString name = selection->GetFirst ()->GetFactory ()->GetName ();
  if (!curvedMeshCreator->GetCurvedFactory (name)) return true;

#if 0
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();
  mainTabButton->setSelected(false);
  roomTabButton->setSelected(false);
  foliageTabButton->setSelected(false);
  curveTabButton->setSelected(true);
  winMgr->getWindow("Ares/ItemWindow")->setVisible(false);
  winMgr->getWindow("Ares/RoomWindow")->setVisible(false);
  winMgr->getWindow("Ares/FoliageWindow")->setVisible(false);
  winMgr->getWindow("Ares/CurveWindow")->setVisible(true);
  if (editMode) editMode->Stop ();
  editMode = curveMode;
  editMode->Start ();
#endif
  return true;
}

bool AresEdit3DView::SwitchToRoomMode ()
{
  if (selection->GetSize () != 1) return true;
  csString name = selection->GetFirst ()->GetFactory ()->GetName ();
  if (!roomMeshCreator->GetRoomFactory (name)) return true;

#if 0
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();
  mainTabButton->setSelected(false);
  curveTabButton->setSelected(false);
  foliageTabButton->setSelected(false);
  roomTabButton->setSelected(true);
  winMgr->getWindow("Ares/ItemWindow")->setVisible(false);
  winMgr->getWindow("Ares/CurveWindow")->setVisible(false);
  winMgr->getWindow("Ares/FoliageWindow")->setVisible(false);
  winMgr->getWindow("Ares/RoomWindow")->setVisible(true);
  if (editMode) editMode->Stop ();
  editMode = roomMode;
  editMode->Start ();
#endif
  return true;
}

bool AresEdit3DView::SwitchToFoliageMode ()
{
#if 0
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();
  mainTabButton->setSelected(false);
  curveTabButton->setSelected(false);
  roomTabButton->setSelected(false);
  foliageTabButton->setSelected(true);
  winMgr->getWindow("Ares/ItemWindow")->setVisible(false);
  winMgr->getWindow("Ares/CurveWindow")->setVisible(false);
  winMgr->getWindow("Ares/RoomWindow")->setVisible(false);
  winMgr->getWindow("Ares/FoliageWindow")->setVisible(true);
  if (editMode) editMode->Stop ();
  editMode = foliageMode;
  editMode->Start ();
#endif
  return true;
}

void AresEdit3DView::CleanupWorld ()
{
  selection->SetCurrentObject (0);

  nature->CleanUp ();

  curvedFactories.DeleteAll ();
  roomFactories.DeleteAll ();
  factory_to_origin_offset.DeleteAll ();
  curvedFactoryCreators.DeleteAll ();
  roomFactoryCreators.DeleteAll ();
  static_factories.DeleteAll ();

  camlight = 0;
  dynworld->DeleteAll ();
  dynworld->DeleteFactories ();
  engine->DeleteAll ();
}

void AresEdit3DView::SaveFile (const char* filename)
{
  //filenameLabel->setText (CEGUI::String (filename));
  // @@@ Error handling.
  worldLoader->SaveFile (filename);
}

void AresEdit3DView::LoadFile (const char* filename)
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

  for (size_t i = 0 ; i < roomMeshCreator->GetRoomFactoryCount () ; i++)
  {
    iRoomFactory* cfact = roomMeshCreator->GetRoomFactory (i);
    static_factories.Add (cfact->GetName ());
  }
  roomFactories = worldLoader->GetRoomFactories ();

  // @@@ Error handling.
  SetupDynWorld ();

  // @@@ Error handling.
  PostLoadMap ();
}

void AresEdit3DView::OnExit()
{
}

void AresEdit3DView::AddItem (const char* category, const char* itemname)
{
  if (!categories.In (category))
    categories.Put (category, csStringArray());
  csStringArray a;
  categories.Get (category, a).Push (itemname);
}

#if USE_DECAL
bool AresEdit3DView::SetupDecal ()
{
  iMaterialWrapper* material = engine->GetMaterialList ()->FindByName ("stone2");
  if (!material)
    return ReportError("Can't find cursor decal material!");
  cursorDecalTemplate = decalMgr->CreateDecalTemplate (material);
  cursorDecal = 0;
  return true;
}
#endif

void AresEdit3DView::ResizeView (int width, int height)
{
  // We use the full window to draw the world.
  view_width = width;
  view_height = height;
  //view->GetPerspectiveCamera ()->SetFOV ((float) (width) / (float) (height), 1.0f);
  view->SetRectangle (0, 0, view_width, view_height);
}

bool AresEdit3DView::Setup ()
{
  iObjectRegistry* r = GetObjectRegistry();

  vfs = csQueryRegistry<iVFS> (r);
  if (!vfs)
    return ReportError("Failed to locate vfs!");

  //camwin = new CameraWindow (this, cegui);

  g3d = csQueryRegistry<iGraphics3D> (r);
  if (!g3d)
    return ReportError("Failed to locate 3D renderer!");

  nature = csQueryRegistry<iNature> (r);
  if (!nature) return ReportError("Failed to locate nature plugin!");

  markerMgr = csQueryRegistry<iMarkerManager> (r);
  if (!markerMgr) return ReportError("Failed to locate marker manager plugin!");

  curvedMeshCreator = csQueryRegistry<iCurvedMeshCreator> (r);
  if (!curvedMeshCreator) return ReportError("Failed to load the curved mesh creator plugin!");

  roomMeshCreator = csQueryRegistry<iRoomMeshCreator> (r);
  if (!roomMeshCreator) return ReportError("Failed to load the room mesh creator plugin!");

  engine = csQueryRegistry<iEngine> (r);
  if (!engine) return ReportError("Failed to locate 3D engine!");

  decalMgr = csQueryRegistry<iDecalManager> (r);
  if (!decalMgr) return ReportError("Failed to load decal manager!");

  eventQueue = csQueryRegistry<iEventQueue> (r);
  if (!eventQueue) return ReportError ("Failed to locate Event Queue!");

  vc = csQueryRegistry<iVirtualClock> (r);
  if (!vc) return ReportError ("Failed to locate Virtual Clock!");

  kbd = csQueryRegistry<iKeyboardDriver> (r);
  if (!kbd) return ReportError ("Failed to locate Keyboard Driver!");

  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  if (!pl) return ReportError ("CEL physical layer missing!");

  loader = csQueryRegistry<iLoader> (r);
  if (!loader) return ReportError("Failed to locate map loader plugin!");

  cdsys = csQueryRegistry<iCollideSystem> (r);
  if (!cdsys) return ReportError ("Failed to locate CD system!");

  cfgmgr = csQueryRegistry<iConfigManager> (r);
  if (!cfgmgr) return ReportError ("Failed to locate the configuration manager plugin!");

  zoneEntity = pl->CreateEntity ("zone", 0, 0,
      "pcworld.dynamic", CEL_PROPCLASS_END);
  if (!zoneEntity) return ReportError ("Failed to create zone entity!");
  dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (zoneEntity);

  elcm = csQueryRegistry<iELCM> (r);
  dynworld->SetELCM (elcm);

  worldLoader = new WorldLoader (r);
  worldLoader->SetZone (dynworld);
  selection = new Selection (0, this);
  SelectionListener* listener = new AresEditSelectionListener (this);
  selection->AddSelectionListener (listener);
  listener->DecRef ();

  iMarkerColor* red = markerMgr->CreateMarkerColor ("red");
  red->SetRGBColor (SELECTION_NONE, .5, 0, 0, 1);
  red->SetRGBColor (SELECTION_SELECTED, 1, 0, 0, 1);
  red->SetRGBColor (SELECTION_ACTIVE, 1, 0, 0, 1);
  red->SetPenWidth (SELECTION_NONE, 1.2f);
  red->SetPenWidth (SELECTION_SELECTED, 2.0f);
  red->SetPenWidth (SELECTION_ACTIVE, 2.0f);
  iMarkerColor* green = markerMgr->CreateMarkerColor ("green");
  green->SetRGBColor (SELECTION_NONE, 0, .5, 0, 1);
  green->SetRGBColor (SELECTION_SELECTED, 0, 1, 0, 1);
  green->SetRGBColor (SELECTION_ACTIVE, 0, 1, 0, 1);
  green->SetPenWidth (SELECTION_NONE, 1.2f);
  green->SetPenWidth (SELECTION_SELECTED, 2.0f);
  green->SetPenWidth (SELECTION_ACTIVE, 2.0f);
  iMarkerColor* blue = markerMgr->CreateMarkerColor ("blue");
  blue->SetRGBColor (SELECTION_NONE, 0, 0, .5, 1);
  blue->SetRGBColor (SELECTION_SELECTED, 0, 0, 1, 1);
  blue->SetRGBColor (SELECTION_ACTIVE, 0, 0, 1, 1);
  blue->SetPenWidth (SELECTION_NONE, 1.2f);
  blue->SetPenWidth (SELECTION_SELECTED, 2.0f);
  blue->SetPenWidth (SELECTION_ACTIVE, 2.0f);
  iMarkerColor* yellow = markerMgr->CreateMarkerColor ("yellow");
  yellow->SetRGBColor (SELECTION_NONE, .5, .5, 0, .5);
  yellow->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, .5);
  yellow->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, .5);
  yellow->SetPenWidth (SELECTION_NONE, 1.2f);
  yellow->SetPenWidth (SELECTION_SELECTED, 2.0f);
  yellow->SetPenWidth (SELECTION_ACTIVE, 2.0f);

  colorWhite = g3d->GetDriver2D ()->FindRGB (255, 255, 255);
  font = g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_COURIER);

  // We need a View to the virtual world.
  view.AttachNew(new csView (engine, g3d));
  view->SetAutoResize (false);
  iGraphics2D* g2d = g3d->GetDriver2D ();
  // We use the full window to draw the world.
  //view_width = (int)(g2d->GetWidth () * 0.86);
  view_width = g2d->GetWidth ();
  view_height = g2d->GetHeight ();
  view->SetRectangle (0, 0, view_width, view_height);

  markerMgr->SetView (view);

  // Set the window title.
  //iNativeWindow* nw = g2d->GetNativeWindow ();
  //if (nw)
    //nw->SetTitle (cfgmgr->GetStr ("WindowTitle",
          //"Please set WindowTitle in AppAresEdit.cfg"));

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
  //Run();

  return true;
}

bool AresEdit3DView::SetupDynWorld ()
{
  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    if (curvedFactories.Find (fact) != csArrayItemNotFound) continue;
    if (roomFactories.Find (fact) != csArrayItemNotFound) continue;
    printf ("%d %s\n", int (i), fact->GetName ()); fflush (stdout);
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
  for (size_t i = 0 ; i < roomMeshCreator->GetRoomFactoryTemplateCount () ; i++)
  {
    iRoomFactoryTemplate* cft = roomMeshCreator->GetRoomFactoryTemplate (i);
    const char* category = cft->GetAttribute ("category");
    AddItem (category, cft->GetName ());

    RoomFactoryCreator creator;
    creator.name = cft->GetName ();

    roomFactoryCreators.Push (creator);
  }
  return true;
}

bool AresEdit3DView::PostLoadMap ()
{
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

  // Force the update of the clock.
  nature->UpdateTime (currentTime+100, GetCsCamera ());
  nature->UpdateTime (currentTime, GetCsCamera ());

  return true;
}

bool AresEdit3DView::SetupWorld ()
{
  vfs->Mount ("/aresnode", "data$/node.zip");
  if (!LoadLibrary ("/aresnode/", "library"))
    return ReportError ("Error loading library!");
  vfs->PopDir ();
  vfs->Unmount ("/aresnode", "data$/node.zip");

  sector = engine->CreateSector ("room");

  dyncell = dynworld->AddCell ("outside", sector, dynSys);
  dynworld->SetCurrentCell (dyncell);

  return true;
}

CurvedFactoryCreator* AresEdit3DView::FindFactoryCreator (const char* name)
{
  for (size_t i = 0 ; i < curvedFactoryCreators.GetSize () ; i++)
    if (curvedFactoryCreators[i].name == name)
      return &curvedFactoryCreators[i];
  return 0;
}

RoomFactoryCreator* AresEdit3DView::FindRoomFactoryCreator (const char* name)
{
  for (size_t i = 0 ; i < roomFactoryCreators.GetSize () ; i++)
    if (roomFactoryCreators[i].name == name)
      return &roomFactoryCreators[i];
  return 0;
}

void AresEdit3DView::SpawnItem (const csString& name)
{
  csString fname;
  iCurvedFactory* curvedFactory = 0;
  iRoomFactory* roomFactory = 0;
  CurvedFactoryCreator* cfc = FindFactoryCreator (name);
  RoomFactoryCreator* rfc = FindRoomFactoryCreator (name);
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
  else if (rfc)
  {
    roomFactoryCounter++;
    fname.Format("%s%d", name.GetData (), roomFactoryCounter);
    roomFactory = roomMeshCreator->AddRoomFactory (fname, name);

    iDynamicFactory* fact = dynworld->AddFactory (fname, 1.0f, -1.0f);
    csRef<iGeometryGenerator> ggen = scfQueryInterface<iGeometryGenerator> (roomFactory);
    if (ggen)
      fact->SetGeometryGenerator (ggen);

    fact->AddRigidMesh (csVector3 (0), cfc->mass);
    static_factories.Add (fname);
    roomFactories.Push (fact);
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
  iDynamicObject* dynobj = dyncell->AddObject (fname, tc);
  dynobj->SetEntity (0, 0);
  dynworld->ForceVisible (dynobj);

  if (!static_factories.In (fname))
  {
    // For a dynamic object we make sure the object is above the ground on
    // all four corners too. This is to ensure that the object doesn't jump
    // up suddenly because it was embedded in the ground partially.
#if 0
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
#endif
  }

  if (static_factories.In (fname))
    dynobj->MakeStatic ();
  selection->SetCurrentObject (dynobj);

  if (curvedFactory)
    SwitchToCurveMode ();
  else if (roomFactory)
    SwitchToRoomMode ();
}

bool AresEdit3DView::InitPhysics ()
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

bool AresEdit3DView::LoadLibrary (const char* path, const char* file)
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

// =========================================================================

BEGIN_EVENT_TABLE(AppAresEditWX, wxFrame)
  EVT_SHOW (AppAresEditWX::OnShow)
  EVT_ICONIZE (AppAresEditWX::OnIconize)
  EVT_MENU (ID_Open, AppAresEditWX :: OnMenuOpen)
  EVT_MENU (ID_Save, AppAresEditWX :: OnMenuSave)
  EVT_MENU (ID_Quit, AppAresEditWX :: OnMenuQuit)
  EVT_MENU (ID_Delete, AppAresEditWX :: OnMenuDelete)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AppAresEditWX::Panel, wxPanel)
  EVT_SIZE(AppAresEditWX::Panel::OnSize)
END_EVENT_TABLE()

// The global pointer to AresEd
AppAresEditWX* aresed = 0;

AppAresEditWX::AppAresEditWX (iObjectRegistry* object_reg)
  : wxFrame (0, -1, wxT ("Crystal Space WxWidget Canvas test"), 
             wxDefaultPosition, wxSize (1000, 600))
{
  AppAresEditWX::object_reg = object_reg;
  aresed3d = 0;
  editMode = 0;
  mainMode = 0;
  curveMode = 0;
  roomMode = 0;
  foliageMode = 0;
  FocusLost = csevFocusLost (object_reg);
}

AppAresEditWX::~AppAresEditWX ()
{
  delete mainMode;
  delete curveMode;
  delete roomMode;
  delete foliageMode;
  delete aresed3d;
}

void AppAresEditWX::OnMenuDelete (wxCommandEvent& event)
{
  aresed3d->DeleteSelectedObjects ();
}

void AppAresEditWX::LoadFile (const char* filename)
{
  aresed3d->LoadFile (filename);
  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  mainMode->SetupItems (categories);
}

void AppAresEditWX::OnMenuOpen (wxCommandEvent& event)
{
  filereq->Show (new LoadCallback (this));
}

void AppAresEditWX::OnMenuSave (wxCommandEvent& event)
{
  //filereq->Show (new LoadCallback (this));
}

void AppAresEditWX::OnMenuQuit (wxCommandEvent& event)
{
}

bool AppAresEditWX::HandleEvent (iEvent& ev)
{
  if (ev.Name == Frame)
  {
    editMode->FramePre ();
    if (aresed3d) aresed3d->Frame (editMode);
    return true;
  }
  else if (CS_IS_KEYBOARD_EVENT(object_reg, ev))
  {
    utf32_char code = csKeyEventHelper::GetCookedCode(&ev);
    if ((ev.Name == KeyboardDown) && (code == CSKEY_ESC))
    {
      /* Close the main window, which will trigger an application exit.
         CS-specific cleanup happens in OnClose(). */
      Close();
      return true;
    }
    if (aresed3d)
    {
      if (aresed3d->OnKeyboard (ev))
	return true;
      else if (csKeyEventHelper::GetEventType(&ev) == csKeyEventTypeDown)
        return editMode->OnKeyboard (ev, code);
    }
  }
  else if ((ev.Name == MouseMove))
  {
    int mouseX = csMouseEventHelper::GetX (&ev);
    int mouseY = csMouseEventHelper::GetY (&ev);
    if (aresed3d)
    {
      if (aresed3d->OnMouseMove (ev))
	return true;
      else
	return editMode->OnMouseMove (ev, mouseX, mouseY);
    }
  }
  else if ((ev.Name == MouseDown))
  {
    uint but = csMouseEventHelper::GetButton (&ev);
    int mouseX = csMouseEventHelper::GetX (&ev);
    int mouseY = csMouseEventHelper::GetY (&ev);
    if (aresed3d)
    {
      if (aresed3d->OnMouseDown (ev))
	return true;
      else
	return editMode->OnMouseDown (ev, but, mouseX, mouseY);
    }
  }
  else if ((ev.Name == MouseUp))
  {
    uint but = csMouseEventHelper::GetButton (&ev);
    int mouseX = csMouseEventHelper::GetX (&ev);
    int mouseY = csMouseEventHelper::GetY (&ev);
    if (aresed3d)
    {
      if (aresed3d->OnMouseUp (ev))
	return true;
      else
	return editMode->OnMouseUp (ev, but, mouseX, mouseY);
    }
  }
  else
  {
    if (ev.Name == FocusLost) editMode->OnFocusLost ();
    if (aresed3d)
      return aresed3d->OnUnhandledEvent (ev);
  }

  return false;
}

bool AppAresEditWX::SimpleEventHandler (iEvent& ev)
{
  return aresed ? aresed->HandleEvent (ev) : false;
}

bool AppAresEditWX::Initialize ()
{
  if (!celInitializer::SetupConfigManager (object_reg,
      "/ares/AppAresEdit.cfg", "ares"))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Failed to setup configmanager!");
    return false;
  }

  if (!celInitializer::RequestPlugins (object_reg,
	CS_REQUEST_VFS,
	CS_REQUEST_PLUGIN ("crystalspace.graphics2d.wxgl", iGraphics2D),
	CS_REQUEST_OPENGL3D,
	CS_REQUEST_ENGINE,
	CS_REQUEST_FONTSERVER,
	CS_REQUEST_IMAGELOADER,
	CS_REQUEST_LEVELLOADER,
	CS_REQUEST_REPORTER,
	CS_REQUEST_REPORTERLISTENER,
	CS_REQUEST_PLUGIN ("cel.physicallayer", iCelPlLayer),
	CS_REQUEST_PLUGIN ("cel.tools.elcm", iELCM),
	CS_REQUEST_PLUGIN ("crystalspace.collisiondetection.opcode", iCollideSystem),
	CS_REQUEST_PLUGIN ("crystalspace.dynamics.bullet", iDynamics),
	CS_REQUEST_PLUGIN ("crystalspace.decal.manager", iDecalManager),
	CS_REQUEST_PLUGIN ("utility.nature", iNature),
	CS_REQUEST_PLUGIN ("utility.marker", iMarkerManager),
	CS_REQUEST_PLUGIN ("utility.curvemesh", iCurvedMeshCreator),
	CS_REQUEST_PLUGIN ("utility.rooms", iRoomMeshCreator),
	CS_REQUEST_END))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Can't initialize plugins!");
    return false;
  }

  //csEventNameRegistry::Register (object_reg);
  if (!csInitializer::SetupEventHandler (object_reg, SimpleEventHandler))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Can't initialize event handler!");
    return false;
  }
  CS_INITIALIZE_EVENT_SHORTCUTS (object_reg);

  KeyboardDown = csevKeyboardDown (object_reg);
  MouseMove = csevMouseMove (object_reg, 0);
  MouseUp = csevMouseUp (object_reg, 0);
  MouseDown = csevMouseDown (object_reg, 0);

  // Check for commandline help.
  if (csCommandLineHelper::CheckHelp (object_reg))
  {
    csCommandLineHelper::Help (object_reg);
    return false;
  }

  // The virtual clock.
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  if (vc == 0)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Can't find the virtual clock!");
    return false;
  }

  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  if (g3d == 0)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "No iGraphics3D plugin!");
    return false;
  }

  vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "No iVFS plugin!");
    return false;
  }

  // Load the frame from an XRC file
  wxXmlResource::Get ()->InitAllHandlers ();

  wxString searchPath (wxT ("data/windows"));
  wxString resourceLocation;
  wxFileSystem wxfs;
  if (!wxfs.FindFileInPath (&resourceLocation, searchPath, wxT ("AresMainFrame.xrc"))
    || !wxXmlResource::Get ()->Load (resourceLocation))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Could not load XRC ressource file!");
    return false;
  }
  if (!wxfs.FindFileInPath (&resourceLocation, searchPath, wxT ("FileRequester.xrc"))
    || !wxXmlResource::Get ()->Load (resourceLocation))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Could not load XRC ressource file!");
    return false;
  }
  if (!wxfs.FindFileInPath (&resourceLocation, searchPath, wxT ("MainModePanel.xrc"))
    || !wxXmlResource::Get ()->Load (resourceLocation))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Could not load XRC ressource file!");
    return false;
  }

  wxPanel* mainPanel = wxXmlResource::Get ()->LoadPanel (this, wxT ("AresMainPanel"));
  if (!mainPanel)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Could not find main panel in XRC ressource file!");
    return false;
  }

  // Find the panel where to place the wxgl canvas
  wxPanel* panel = XRCCTRL (*this, "main3DPanel", wxPanel);
  if (!panel)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Could not find the panel for the wxgl canvas in XRC ressource file!");
    return false;
  }

  // Create the wxgl canvas
  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->AllowResize (true);
  wxwindow = scfQueryInterface<iWxWindow> (g2d);
  if (!wxwindow)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Canvas is no iWxWindow plugin!");
    return false;
  }

  wxPanel* panel1 = new AppAresEditWX::Panel (panel, this);
  panel->GetSizer ()->Add (panel1, 1, wxALL | wxEXPAND);
  wxwindow->SetParent (panel1);

  Show (true);

  // Open the main system. This will open all the previously loaded plug-ins.
  if (!csInitializer::OpenApplication (object_reg))
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR,
              "crystalspace.application.aresedit",
              "Error opening system!");
    return false;
  }

  /* Manually focus the GL canvas.
     This is so it receives keyboard events (and conveniently translate these
     into CS keyboard events/update the CS keyboard state).
   */
  wxwindow->GetWindow ()->SetFocus ();

  aresed3d = new AresEdit3DView (object_reg);
  if (!aresed3d->Setup ())
    return false;
 
  filereq = new FileReq (wxwindow->GetWindow (), vfs, "/saves");

  wxPanel* mainModeTabPanel = XRCCTRL (*this, "mainModeTabPanel", wxPanel);
  mainMode = new MainMode (mainModeTabPanel, aresed3d);
  curveMode = new CurveMode (0, aresed3d);
  roomMode = new RoomMode (0, aresed3d);
  foliageMode = new FoliageMode (0, aresed3d);
  editMode = mainMode;
  editMode->Start ();
  //mainTabButton->setSelected(true);

  SelectionListener* listener = new MainModeSelectionListener (mainMode);
  aresed3d->GetSelection ()->AddSelectionListener (listener);

  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  mainMode->SetupItems (categories);

  SetupMenuBar ();

  printer.AttachNew (new FramePrinter (object_reg));

  return true;
}

void AppAresEditWX::SetupMenuBar ()
{
  wxMenu* fileMenu = new wxMenu ();
  fileMenu->Append (ID_Open, wxT ("&Open...\tCtrl+O"));
  fileMenu->Append (ID_Save, wxT ("&Save...\tCtrl+S"));
  fileMenu->Append (ID_Quit, wxT ("&Exit..."));

  wxMenu* editMenu = new wxMenu ();
  editMenu->Append (ID_Delete, wxT ("&Delete"));

  wxMenuBar* menuBar = new wxMenuBar ();
  menuBar->Append (fileMenu, wxT ("&File"));
  menuBar->Append (editMenu, wxT ("&Edit"));
  SetMenuBar (menuBar);
  menuBar->Reparent (this);
}

void AppAresEditWX::PushFrame ()
{
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  if (!q)
    return ;
  csRef<iVirtualClock> vc (csQueryRegistry<iVirtualClock> (object_reg));

  if (vc)
    vc->Advance();
  q->Process();
}

void AppAresEditWX::OnSize (wxSizeEvent& event)
{
  if (!wxwindow->GetWindow ()) return;

  wxSize size = event.GetSize();
  printf ("OnSize %d,%d\n", size.x, size.y); fflush (stdout);
  wxwindow->GetWindow ()->SetSize (size); // TODO: csGraphics2DGLCommon::Resize is called here...
  aresed3d->ResizeView (size.x, size.y);
  // TODO: ... but here the CanvasResize event has still not been catched by iGraphics3D
}

void AppAresEditWX::OnClose (wxCloseEvent& event)
{
  csPrintf("got close event\n");
  
  // Tell CS we're going down
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  if (q) q->GetEventOutlet()->Broadcast (csevQuit(object_reg));
  
  // WX will destroy the 'AppAresEditWX' instance
  aresed = 0;
}

void AppAresEditWX::OnIconize (wxIconizeEvent& event)
{
  csPrintf("got iconize %d\n", (int) event.Iconized ());
}

void AppAresEditWX::OnShow (wxShowEvent& event)
{
  csPrintf("got show %d\n", (int) event.GetShow ());
}
