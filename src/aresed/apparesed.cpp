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
#include "include/imarker.h"
#include "ui/uimanager.h"
#include "ui/filereq.h"
#include "ui/newproject.h"
#include "ui/celldialog.h"
#include "modes/mainmode.h"
#include "modes/curvemode.h"
#include "modes/roommode.h"
#include "modes/foliagemode.h"
#include "modes/entitymode.h"
#include "camera.h"

#include "celtool/initapp.h"
#include "cstool/simplestaticlighter.h"
#include "celtool/persisthelper.h"
#include "physicallayer/pl.h"
#include "tools/parameters.h"

#include <csgeom/math3d.h>
#include "camerawin.h"
#include "selection.h"
#include "common/worldload.h"
#include "transformtools.h"


/* Fun fact: should occur after csutil/event.h, otherwise, gcc may report
 * missing csMouseEventHelper symbols. */
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

void AresEditSelectionListener::SelectionChanged (
    const csArray<iDynamicObject*>& current_objects)
{
  aresed3d->SelectionChanged (current_objects);
}

void AppSelectionListener::SelectionChanged (
    const csArray<iDynamicObject*>& current_objects)
{
  app->SetMenuState ();
  app->GetMainMode ()->CurrentObjectsChanged (current_objects);
}

struct NewProjectCallbackImp : public NewProjectCallback
{
  AppAresEditWX* ares;
  NewProjectCallbackImp (AppAresEditWX* ares) : ares (ares) { }
  virtual ~NewProjectCallbackImp () { }
  virtual void OkPressed (const csArray<Asset>& assets)
  {
    ares->NewProject (assets);
  }
};

struct LoadCallback : public OKCallback
{
  AppAresEditWX* ares;
  LoadCallback (AppAresEditWX* ares) : ares (ares) { }
  virtual ~LoadCallback () { }
  virtual void OkPressed (const char* filename)
  {
    ares->LoadFile (filename);
  }
};

struct SaveCallback : public OKCallback
{
  AppAresEditWX* ares;
  SaveCallback (AppAresEditWX* ares) : ares (ares) { }
  virtual ~SaveCallback () { }
  virtual void OkPressed (const char* filename)
  {
    ares->SaveFile (filename);
  }
};

// =========================================================================

AresEdit3DView::AresEdit3DView (AppAresEditWX* app, iObjectRegistry* object_reg) :
  app (app), object_reg (object_reg), camera (this)
{
  do_debug = false;
  do_simulation = true;
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
  delete worldLoader;
  delete selection;
}

void AresEdit3DView::Do3DPreFrameStuff ()
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
  g3d->BeginDraw( CSDRAW_3DGRAPHICS);
  editMode->Frame3D ();
  markerMgr->Frame3D ();

  g3d->BeginDraw (CSDRAW_2DGRAPHICS);
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

void AresEdit3DView::SelectionChanged (const csArray<iDynamicObject*>& current_objects)
{
  app->GetCameraWindow ()->CurrentObjectsChanged (current_objects);

  bool curveTabEnable = false;
  if (selection->GetSize () == 1)
  {
    csString name = selection->GetFirst ()->GetFactory ()->GetName ();
    if (curvedMeshCreator->GetCurvedFactory (name)) curveTabEnable = true;
  }
  app->SetCurveModeEnabled (curveTabEnable);
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

  return false;
}

bool AresEdit3DView::OnMouseUp (iEvent& ev)
{
  uint but = csMouseEventHelper::GetButton (&ev);
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

  if (markerMgr->OnMouseUp (ev, but, mouseX, mouseY))
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
  if (!worldLoader->SaveFile (filename))
  {
    app->GetUIManager ()->Error ("Error saving file '%s'!",filename);
    return;
  }
}

void AresEdit3DView::NewProject (const csArray<Asset>& assets)
{
  CleanupWorld ();
  SetupWorld ();

  if (!worldLoader->NewProject (assets))
  {
    PostLoadMap ();
    app->GetUIManager ()->Error ("Error creating project!");
    return;
  }

  // @@@ Hardcoded sector name!
  sector = engine->FindSector ("outside");

  // @@@ Error handling.
  SetupDynWorld ();

  // @@@ Error handling.
  PostLoadMap ();
}

void AresEdit3DView::LoadFile (const char* filename)
{
  CleanupWorld ();
  SetupWorld ();

  if (!worldLoader->LoadFile (filename))
  {
    PostLoadMap ();
    app->GetUIManager ()->Error ("Error loading file '%s'!",filename);
    return;
  }

  // @@@ Hardcoded sector name!
  sector = engine->FindSector ("outside");

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
    return app->ReportError("Can't find cursor decal material!");
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
  if (!vfs) return app->ReportError("Failed to locate vfs!");

  g3d = csQueryRegistry<iGraphics3D> (r);
  if (!g3d) return app->ReportError("Failed to locate 3D renderer!");

  nature = csQueryRegistry<iNature> (r);
  if (!nature) return app->ReportError("Failed to locate nature plugin!");

  markerMgr = csQueryRegistry<iMarkerManager> (r);
  if (!markerMgr) return app->ReportError("Failed to locate marker manager plugin!");

  curvedMeshCreator = csQueryRegistry<iCurvedMeshCreator> (r);
  if (!curvedMeshCreator) return app->ReportError("Failed to load the curved mesh creator plugin!");

  roomMeshCreator = csQueryRegistry<iRoomMeshCreator> (r);
  if (!roomMeshCreator) return app->ReportError("Failed to load the room mesh creator plugin!");

  engine = csQueryRegistry<iEngine> (r);
  if (!engine) return app->ReportError("Failed to locate 3D engine!");

  decalMgr = csQueryRegistry<iDecalManager> (r);
  if (!decalMgr) return app->ReportError("Failed to load decal manager!");

  eventQueue = csQueryRegistry<iEventQueue> (r);
  if (!eventQueue) return app->ReportError ("Failed to locate Event Queue!");

  vc = csQueryRegistry<iVirtualClock> (r);
  if (!vc) return app->ReportError ("Failed to locate Virtual Clock!");

  kbd = csQueryRegistry<iKeyboardDriver> (r);
  if (!kbd) return app->ReportError ("Failed to locate Keyboard Driver!");

  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  if (!pl) return app->ReportError ("CEL physical layer missing!");

  loader = csQueryRegistry<iLoader> (r);
  if (!loader) return app->ReportError("Failed to locate map loader plugin!");

  cdsys = csQueryRegistry<iCollideSystem> (r);
  if (!cdsys) return app->ReportError ("Failed to locate CD system!");

  cfgmgr = csQueryRegistry<iConfigManager> (r);
  if (!cfgmgr) return app->ReportError ("Failed to locate the configuration manager plugin!");

  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (
      r, "cel.parameters.manager");
  pm->SetRememberExpression (true);

  zoneEntity = pl->CreateEntity ("zone", 0, 0,
      "pcworld.dynamic", CEL_PROPCLASS_END);
  if (!zoneEntity) return app->ReportError ("Failed to create zone entity!");
  dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (zoneEntity);

  elcm = csQueryRegistry<iELCM> (r);
  dynworld->SetELCM (elcm);

  worldLoader = new WorldLoader (r);
  worldLoader->SetZone (dynworld);
  selection = new Selection (this);
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
  iMarkerColor* white = markerMgr->CreateMarkerColor ("white");
  white->SetRGBColor (SELECTION_NONE, .5, .5, .5, .5);
  white->SetRGBColor (SELECTION_SELECTED, 1, 1, 1, .5);
  white->SetRGBColor (SELECTION_ACTIVE, 1, 1, 1, .5);
  white->SetPenWidth (SELECTION_NONE, 1.2f);
  white->SetPenWidth (SELECTION_SELECTED, 2.0f);
  white->SetPenWidth (SELECTION_ACTIVE, 2.0f);

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
    return app->ReportError ("Error during PostLoadMap()!");

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
      return app->ReportError("Error cannot find the terrain interface!");

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

void AresEdit3DView::WarpCell (iDynamicCell* cell)
{
  if (cell == dynworld->GetCurrentCell ()) return; 
  dyncell = cell;
  if (sector && camlight) sector->GetLights ()->Remove (camlight);
  camlight = 0;
  dynworld->SetCurrentCell (cell);
  sector = engine->FindSector (cell->GetName ());
  nature->InitSector (sector);
  camlight = engine->CreateLight(0, csVector3(0.0f, 0.0f, 0.0f), 10, csColor (0.8f, 0.9f, 1.0f));
  iLightList* lightList = sector->GetLights ();
  lightList->Add (camlight);

  camera.Init (view->GetCamera (), sector, csVector3 (0, 10, 0));
}

bool AresEdit3DView::SetupWorld ()
{
  vfs->Mount ("/aresnode", "data$/node.zip");
  if (!LoadLibrary ("/aresnode/", "library"))
    return app->ReportError ("Error loading library!");
  vfs->PopDir ();
  vfs->Unmount ("/aresnode", "data$/node.zip");

  sector = engine->CreateSector ("outside");

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

csReversibleTransform AresEdit3DView::GetSpawnTransformation (const csString& name,
    csReversibleTransform* trans)
{
  csString fname;
  CurvedFactoryCreator* cfc = FindFactoryCreator (name);
  RoomFactoryCreator* rfc = FindRoomFactoryCreator (name);
  if (cfc)
    fname.Format("%s%d", name.GetData (), curvedFactoryCounter+1);
  else if (rfc)
    fname.Format("%s%d", name.GetData (), roomFactoryCounter+1);
  else
    fname = name;

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
  if (trans) tc = *trans;
  tc.SetOrigin (tc.GetOrigin () + newPosition);
  return tc;
}

iDynamicObject* AresEdit3DView::SpawnItem (const csString& name,
    csReversibleTransform* trans)
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
  if (trans)
  {
    tc = *trans;
    tc.SetOrigin (tc.GetOrigin () + newPosition);
  }
  else
  {
    tc.SetOrigin (newPosition);
  }
  iDynamicObject* dynobj = dyncell->AddObject (fname, tc);
  //dynobj->SetEntity (0, fname, 0);
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
    app->SwitchToCurveMode ();
  else if (roomFactory)
    app->SwitchToRoomMode ();
  return dynobj;
}

bool AresEdit3DView::InitPhysics ()
{
  dyn = csQueryRegistry<iDynamics> (GetObjectRegistry ());
  if (!dyn) return app->ReportError ("Error loading bullet plugin!");

  dynSys = dyn->CreateSystem ();
  // @@@ Every cell has its own system. Not handled here!
  dynSys->QueryObject ()->SetName ("ares.dynamics.system");
  if (!dynSys) return app->ReportError ("Error creating dynamic system!");
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
    return app->ReportError("Couldn't load library file %s!", path);
  }
  vfs->PopDir ();
  return true;
}

// =========================================================================

BEGIN_EVENT_TABLE(AppAresEditWX, wxFrame)
  EVT_SHOW (AppAresEditWX::OnShow)
  EVT_ICONIZE (AppAresEditWX::OnIconize)
  EVT_MENU (ID_New, AppAresEditWX :: OnMenuNew)
  EVT_MENU (ID_Cells, AppAresEditWX :: OnMenuCells)
  EVT_MENU (ID_Open, AppAresEditWX :: OnMenuOpen)
  EVT_MENU (ID_Save, AppAresEditWX :: OnMenuSave)
  EVT_MENU (ID_Quit, AppAresEditWX :: OnMenuQuit)
  EVT_MENU (ID_Delete, AppAresEditWX :: OnMenuDelete)
  EVT_MENU (ID_Copy, AppAresEditWX :: OnMenuCopy)
  EVT_MENU (ID_Paste, AppAresEditWX :: OnMenuPaste)
  EVT_NOTEBOOK_PAGE_CHANGING (XRCID("mainNotebook"), AppAresEditWX :: OnNotebookChange)
  EVT_NOTEBOOK_PAGE_CHANGED (XRCID("mainNotebook"), AppAresEditWX :: OnNotebookChanged)
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
  camwin = 0;
  editMode = 0;
  mainMode = 0;
  curveMode = 0;
  roomMode = 0;
  foliageMode = 0;
  entityMode = 0;
  uiManager = 0;
  //oldPageIdx = csArrayItemNotFound;
  FocusLost = csevFocusLost (object_reg);
}

AppAresEditWX::~AppAresEditWX ()
{
  delete camwin;
  delete mainMode;
  delete curveMode;
  delete roomMode;
  delete foliageMode;
  delete entityMode;
  delete aresed3d;
  delete uiManager;
}

void AppAresEditWX::OnMenuCopy (wxCommandEvent& event)
{
  if (editMode == mainMode) mainMode->CopySelection ();
}

void AppAresEditWX::OnMenuPaste (wxCommandEvent& event)
{
  if (editMode == mainMode) mainMode->StartPasteSelection ();
}

void AppAresEditWX::OnMenuDelete (wxCommandEvent& event)
{
  aresed3d->DeleteSelectedObjects ();
}

void AppAresEditWX::NewProject (const csArray<Asset>& assets)
{
  aresed3d->NewProject (assets);
  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  mainMode->SetupItems (categories);
}

void AppAresEditWX::LoadFile (const char* filename)
{
  aresed3d->LoadFile (filename);
  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  mainMode->SetupItems (categories);
}

void AppAresEditWX::SaveFile (const char* filename)
{
  aresed3d->SaveFile (filename);
}

void AppAresEditWX::OnMenuNew (wxCommandEvent& event)
{
  csRef<NewProjectCallbackImp> cb;
  cb.AttachNew (new NewProjectCallbackImp (this));
  uiManager->GetNewProjectDialog ()->Show (cb);
}

void AppAresEditWX::OnMenuCells (wxCommandEvent& event)
{
  uiManager->GetCellDialog ()->Show ();
}

void AppAresEditWX::OnMenuOpen (wxCommandEvent& event)
{
  csRef<LoadCallback> cb;
  cb.AttachNew (new LoadCallback (this));
  uiManager->GetFileReqDialog ()->Show (cb);
}

void AppAresEditWX::OnMenuSave (wxCommandEvent& event)
{
  csRef<SaveCallback> cb;
  cb.AttachNew (new SaveCallback (this));
  uiManager->GetFileReqDialog ()->Show (cb);
}

void AppAresEditWX::OnMenuQuit (wxCommandEvent& event)
{
}

void AppAresEditWX::OnNotebookChange (wxNotebookEvent& event)
{
  //oldPageIdx = event.GetOldSelection ();
}

void AppAresEditWX::OnNotebookChanged (wxNotebookEvent& event)
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  int pageIdx = event.GetSelection ();
  wxString pageName = notebook->GetPageText (pageIdx);

  EditingMode* newMode = 0;
  if (pageName == wxT ("Main")) newMode = mainMode;
  else if (pageName == wxT ("Curve")) newMode = curveMode;
  else if (pageName == wxT ("Foliage")) newMode = foliageMode;
  else if (pageName == wxT ("Entity")) newMode = entityMode;
  if (editMode != newMode)
  {
    if (editMode) editMode->Stop ();
    editMode = newMode;
    editMode->Start ();
  }
  SetMenuState ();

  //if (page && !page->IsEnabled () && oldPageIdx != csArrayItemNotFound)
  //{
    //printf ("REVERT %d\n", oldPageIdx);
    //notebook->ChangeSelection (oldPageIdx);
  //}
  //oldPageIdx = csArrayItemNotFound;
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
      if (but == csmbRight)
      {
	wxMenu contextMenu;
	bool camwinVis = camwin->IsVisible ();
	if (camwinVis)
	{
	  if (aresed3d->GetSelection ()->HasSelection ())
	    contextMenu.Append (ID_Delete, wxT ("&Delete"));
	  camwin->AddContextMenu (&contextMenu, mouseX, mouseY);
	}
	editMode->AddContextMenu (&contextMenu, mouseX, mouseY);

	PopupMenu (&contextMenu);
      }
      else
      {
        if (aresed3d->OnMouseDown (ev))
	  return true;
        else
	  return editMode->OnMouseDown (ev, but, mouseX, mouseY);
      }
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
  }

  return false;
}

bool AppAresEditWX::SimpleEventHandler (iEvent& ev)
{
  return aresed ? aresed->HandleEvent (ev) : false;
}

bool AppAresEditWX::LoadResourceFile (const char* filename, wxString& searchPath)
{
  wxString resourceLocation;
  wxFileSystem wxfs;
  if (!wxfs.FindFileInPath (&resourceLocation, searchPath,
	wxString (filename, wxConvUTF8))
      || !wxXmlResource::Get ()->Load (resourceLocation))
    return ReportError ("Could not load XRC resource file: %s!", filename);
  return true;
}


bool AppAresEditWX::Initialize ()
{
  if (!celInitializer::SetupConfigManager (object_reg,
      "/ares/AppAresEdit.cfg", "ares"))
    return ReportError ("Failed to setup configmanager!");

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
    return ReportError ("Can't initialize plugins!");

  //csEventNameRegistry::Register (object_reg);
  if (!csInitializer::SetupEventHandler (object_reg, SimpleEventHandler))
    return ReportError ("Can't initialize event handler!");

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
  if (vc == 0) return ReportError ("Can't find the virtual clock!");

  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  if (g3d == 0) return ReportError ("Can't find the iGraphics3D plugin!");

  vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs) return ReportError ("Can't find the iVFS plugin!");

  if (!InitWX ())
    return false;

  printer.AttachNew (new FramePrinter (object_reg));

  return true;
}

bool AppAresEditWX::InitWX ()
{
  // Load the frame from an XRC file
  wxXmlResource::Get ()->InitAllHandlers ();

  wxString searchPath (wxT ("data/windows"));
  if (!LoadResourceFile ("AresMainFrame.xrc", searchPath)) return false;
  if (!LoadResourceFile ("FileRequester.xrc", searchPath)) return false;
  if (!LoadResourceFile ("MainModePanel.xrc", searchPath)) return false;
  if (!LoadResourceFile ("CurveModePanel.xrc", searchPath)) return false;
  if (!LoadResourceFile ("FoliageModePanel.xrc", searchPath)) return false;
  if (!LoadResourceFile ("EntityModePanel.xrc", searchPath)) return false;
  if (!LoadResourceFile ("CameraPanel.xrc", searchPath)) return false;
  if (!LoadResourceFile ("NewProjectDialog.xrc", searchPath)) return false;
  if (!LoadResourceFile ("CellDialog.xrc", searchPath)) return false;
  if (!LoadResourceFile ("PropertyClassDialog.xrc", searchPath)) return false;

  wxPanel* mainPanel = wxXmlResource::Get ()->LoadPanel (this, wxT ("AresMainPanel"));
  if (!mainPanel) return ReportError ("Can't find main panel!");

  // Find the panel where to place the wxgl canvas
  wxPanel* panel = XRCCTRL (*this, "main3DPanel", wxPanel);
  if (!panel) return ReportError ("Can't find main3DPanel!");
  panel->DragAcceptFiles (false);

  // Create the wxgl canvas
  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->AllowResize (true);
  wxwindow = scfQueryInterface<iWxWindow> (g2d);
  if (!wxwindow) return ReportError ("Canvas is no iWxWindow plugin!");

  wxPanel* panel1 = new AppAresEditWX::Panel (panel, this);
  panel->GetSizer ()->Add (panel1, 1, wxALL | wxEXPAND);
  wxwindow->SetParent (panel1);

  Show (true);

  // Open the main system. This will open all the previously loaded plug-ins.
  if (!csInitializer::OpenApplication (object_reg))
    return ReportError ("Error opening system!");

  /* Manually focus the GL canvas.
     This is so it receives keyboard events (and conveniently translate these
     into CS keyboard events/update the CS keyboard state).
   */
  wxwindow->GetWindow ()->SetFocus ();

  aresed3d = new AresEdit3DView (this, object_reg);
  if (!aresed3d->Setup ())
    return false;
 
  uiManager = new UIManager (this, wxwindow->GetWindow ());

  wxPanel* mainModeTabPanel = XRCCTRL (*this, "mainModeTabPanel", wxPanel);
  mainMode = new MainMode (mainModeTabPanel, aresed3d);
  mainMode->AllocContextHandlers (this);

  wxPanel* curveModeTabPanel = XRCCTRL (*this, "curveModeTabPanel", wxPanel);
  curveMode = new CurveMode (curveModeTabPanel, aresed3d);
  curveMode->AllocContextHandlers (this);

  roomMode = new RoomMode (aresed3d);

  wxPanel* foliageModeTabPanel = XRCCTRL (*this, "foliageModeTabPanel", wxPanel);
  foliageMode = new FoliageMode (foliageModeTabPanel, aresed3d);
  foliageMode->AllocContextHandlers (this);

  wxPanel* entityModeTabPanel = XRCCTRL (*this, "entityModeTabPanel", wxPanel);
  entityMode = new EntityMode (entityModeTabPanel, aresed3d);
  entityMode->AllocContextHandlers (this);

  editMode = 0;

  wxPanel* leftPanePanel = XRCCTRL (*this, "leftPanePanel", wxPanel);
  camwin = new CameraWindow (leftPanePanel, aresed3d);
  camwin->AllocContextHandlers (this);

  SelectionListener* listener = new AppSelectionListener (this);
  aresed3d->GetSelection ()->AddSelectionListener (listener);

  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  mainMode->SetupItems (categories);

  SetupMenuBar ();

  SwitchToMainMode ();

  return true;
}

void AppAresEditWX::SetStatus (const char* statusmsg, ...)
{
  va_list args;
  va_start (args, statusmsg);
  csString str;
  str.FormatV (statusmsg, args);
  va_end (args);
  if (GetStatusBar ())
    GetStatusBar ()->SetStatusText (wxString (str, wxConvUTF8), 0);
}

void AppAresEditWX::ClearStatus ()
{
  if (editMode)
    SetStatus ("%s", editMode->GetStatusLine ().GetData ());
  else
    SetStatus ("");
}

void AppAresEditWX::SetupMenuBar ()
{
  wxMenu* fileMenu = new wxMenu ();
  fileMenu->Append (ID_New, wxT ("&New project..."));
  fileMenu->Append (ID_Cells, wxT ("&Manage Cells..."));
  fileMenu->AppendSeparator ();
  fileMenu->Append (ID_Open, wxT ("&Open...\tCtrl+O"));
  fileMenu->Append (ID_Save, wxT ("&Save...\tCtrl+S"));
  fileMenu->Append (ID_Quit, wxT ("&Exit..."));

  wxMenu* editMenu = new wxMenu ();
  editMenu->Append (ID_Copy, wxT ("&Copy\tCtrl+C"));
  editMenu->Append (ID_Paste, wxT ("&Paste\tCtrl+V"));
  editMenu->Append (ID_Delete, wxT ("&Delete"));

  wxMenuBar* menuBar = new wxMenuBar ();
  menuBar->Append (fileMenu, wxT ("&File"));
  menuBar->Append (editMenu, wxT ("&Edit"));
  SetMenuBar (menuBar);
  menuBar->Reparent (this);

  CreateStatusBar ();
  SetMenuState ();
}

void AppAresEditWX::SetMenuState ()
{
  ClearStatus ();
  wxMenuBar* menuBar = GetMenuBar ();
  if (!menuBar) return;

  // Should menus be globally disabled?
  bool dis = mainMode ? mainMode->IsPasteSelectionActive () : TRUE;
  if (dis)
  {
    menuBar->EnableTop (0, false);
    return;
  }
  menuBar->EnableTop (0, true);

  // Is there a selection?
  csArray<iDynamicObject*> objects = aresed3d->GetSelection ()->GetObjects ();
  bool sel = objects.GetSize () > 0;

  if (editMode == mainMode)
  {
    menuBar->Enable (ID_Paste, mainMode->IsPasteBufferFull ());
    menuBar->Enable (ID_Delete, sel);
    menuBar->Enable (ID_Copy, sel);
  }
  else
  {
    menuBar->Enable (ID_Paste, false);
    menuBar->Enable (ID_Delete, false);
    menuBar->Enable (ID_Copy, false);
  }
}

void AppAresEditWX::PushFrame ()
{
  static bool lock = false;
  if (lock) return;
  lock = true;
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  csRef<iVirtualClock> vc (csQueryRegistry<iVirtualClock> (object_reg));

  if (vc)
    vc->Advance();
  q->Process();
  lock = false;
}

void AppAresEditWX::OnSize (wxSizeEvent& event)
{
  if (!wxwindow->GetWindow ()) return;

  wxSize size = event.GetSize();
  printf ("OnSize %d,%d\n", size.x, size.y); fflush (stdout);
  wxwindow->GetWindow ()->SetSize (size);
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

static size_t FindNotebookPage (wxNotebook* notebook, const char* name)
{
  wxString iname (name, wxConvUTF8);
  for (size_t i = 0 ; i < notebook->GetPageCount () ; i++)
  {
    wxString pageName = notebook->GetPageText (i);
    if (pageName == iname) return i;
  }
  return csArrayItemNotFound;
}

void AppAresEditWX::SwitchToMainMode ()
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  size_t pageIdx = FindNotebookPage (notebook, "Main");
  notebook->ChangeSelection (pageIdx);
  if (editMode) editMode->Stop ();
  editMode = mainMode;
  editMode->Start ();
  SetMenuState ();
}

void AppAresEditWX::SetCurveModeEnabled (bool cm)
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  size_t pageIdx = FindNotebookPage (notebook, "Curve");
  wxNotebookPage* page = notebook->GetPage (pageIdx);
  if (cm) page->Enable ();
  else page->Disable ();
}

void AppAresEditWX::SwitchToCurveMode ()
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  size_t pageIdx = FindNotebookPage (notebook, "Curve");
  notebook->ChangeSelection (pageIdx);
  if (editMode) editMode->Stop ();
  editMode = curveMode;
  editMode->Start ();
  SetMenuState ();
}

void AppAresEditWX::SwitchToRoomMode ()
{
  SetMenuState ();
}

void AppAresEditWX::SwitchToFoliageMode ()
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  size_t pageIdx = FindNotebookPage (notebook, "Foliage");
  notebook->ChangeSelection (pageIdx);
  if (editMode) editMode->Stop ();
  editMode = foliageMode;
  editMode->Start ();
  SetMenuState ();
}

void AppAresEditWX::SwitchToEntityMode ()
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  size_t pageIdx = FindNotebookPage (notebook, "Entity");
  notebook->ChangeSelection (pageIdx);
  if (editMode) editMode->Stop ();
  editMode = entityMode;
  editMode->Start ();
  SetMenuState ();
}

