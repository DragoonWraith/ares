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
#include "camera.h"
#include "editor/imode.h"
#include "editor/iplugin.h"

#include "celtool/initapp.h"
#include "cstool/simplestaticlighter.h"
#include "celtool/persisthelper.h"
#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/parameters.h"

#include <csgeom/math3d.h>
#include "camerawin.h"
#include "selection.h"
#include "models/dynfactmodel.h"
#include "models/objects.h"
#include "common/worldload.h"
#include "edcommon/transformtools.h"


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

class AresDynamicCellCreator : public scfImplementation1<AresDynamicCellCreator,
  iDynamicCellCreator>
{
private:
  AresEdit3DView* aresed3d;

public:
  AresDynamicCellCreator (AresEdit3DView* aresed3d) :
    scfImplementationType (this), aresed3d (aresed3d) { }
  virtual ~AresDynamicCellCreator () { }
  virtual iDynamicCell* CreateCell (const char* name)
  {
    return aresed3d->CreateCell (name);
  }
  virtual void FillCell (iDynamicCell* cell) { }
};


// =========================================================================

AresEdit3DView::AresEdit3DView (AppAresEditWX* app, iObjectRegistry* object_reg) :
  scfImplementationType (this),
  app (app), object_reg (object_reg)
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
  dynfactCollectionValue.AttachNew (new DynfactCollectionValue (this));
  objectsValue.AttachNew (new ObjectsValue (app));
  camera.AttachNew (new Camera (this));
  pasteMarker = 0;
  constrainMarker = 0;
  pasteConstrainMode = CONSTRAIN_NONE;
  gridMode = false;
  gridSize = 0.1;
}

AresEdit3DView::~AresEdit3DView()
{
  delete worldLoader;
  delete selection;
}

void AresEdit3DView::Frame (iEditingMode* editMode)
{
  if (IsPasteSelectionActive ()) PlacePasteMarker ();

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
  SelectionIterator it = selection->GetIteratorInt ();
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

void AresEdit3DView::ChangeNameSelectedObject (const char* name)
{
  if (selection->GetSize () < 1) return;
  selection->GetFirst ()->SetEntityName (name);
}

iEditorCamera* AresEdit3DView::GetEditorCamera () const
{
  return static_cast<iEditorCamera*> (camera);
}

void AresEdit3DView::SelectionChanged (const csArray<iDynamicObject*>& current_objects)
{
  objectsValue->RefreshModel ();
  app->GetCameraWindow ()->CurrentObjectsChanged (current_objects);

  bool curveTabEnable = false;
  if (selection->GetSize () == 1)
  {
    iDynamicObject* dynobj = selection->GetFirst ();
    csString name = dynobj->GetFactory ()->GetName ();
    if (curvedMeshCreator->GetCurvedFactory (name)) curveTabEnable = true;
  }
  app->SetCurveModeEnabled (curveTabEnable);
}

void AresEdit3DView::StopPasteMode ()
{
  todoSpawn.Empty ();
  pasteMarker->SetVisible (false);
  constrainMarker->SetVisible (false);
  app->SetMenuState ();
  app->ClearStatus ();
}

void AresEdit3DView::CopySelection ()
{
  pastebuffer.Empty ();
  csRef<iSelectionIterator> it = selection->GetIterator ();
  while (it->HasNext ())
  {
    iDynamicObject* dynobj = it->Next ();
    iDynamicFactory* dynfact = dynobj->GetFactory ();
    PasteContents apc;
    apc.useTransform = true;	// Use the transform defined in this paste buffer.
    apc.dynfactName = dynfact->GetName ();
    apc.trans = dynobj->GetTransform ();
    apc.isStatic = dynobj->IsStatic ();
    pastebuffer.Push (apc);
  }
}

void AresEdit3DView::PasteSelection ()
{
  if (todoSpawn.GetSize () <= 0) return;
  csReversibleTransform trans = todoSpawn[0].trans;
  for (size_t i = 0 ; i < todoSpawn.GetSize () ; i++)
  {
    csReversibleTransform tr = todoSpawn[i].trans;
    csReversibleTransform* transPtr = 0;
    if (todoSpawn[i].useTransform)
    {
      tr.SetOrigin (tr.GetOrigin () - trans.GetOrigin ());
      transPtr = &tr;
    }
    iDynamicObject* dynobj = SpawnItem (todoSpawn[i].dynfactName, transPtr);
    if (todoSpawn[i].useTransform)
    {
      if (todoSpawn[i].isStatic)
        dynobj->MakeStatic ();
      else
        dynobj->MakeDynamic ();
    }
  }
}

void AresEdit3DView::CreatePasteMarker ()
{
  if (currentPasteMarkerContext != todoSpawn[0].dynfactName)
  {
    iMarkerColor* white = markerMgr->FindMarkerColor ("white");

    // We need to recreate the mesh in the paste marker.
    pasteMarker->Clear ();
    currentPasteMarkerContext = todoSpawn[0].dynfactName;
    bool error = true;
    iMeshFactoryWrapper* factory = engine->FindMeshFactory (currentPasteMarkerContext);
    if (factory)
    {
      iMeshObjectFactory* fact = factory->GetMeshObjectFactory ();
      iObjectModel* model = fact->GetObjectModel ();
      if (model)
      {
 	csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet> (
 	   object_reg, "crystalspace.shared.stringset");
 	csStringID baseId = strings->Request ("base");
	iTriangleMesh* triangles = model->GetTriangleData (baseId);
	if (triangles)
	{
	  error = false;
	  pasteMarker->Mesh (MARKER_OBJECT, triangles, white);
	}
      }
    }

    if (error)
    {
      // @@@ Is this needed?
      pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (1,0,0), white, true);
      pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,1,0), white, true);
      pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,0,1), white, true);
    }
  }
}

void AresEdit3DView::ToggleGridMode ()
{
  gridMode = !gridMode;
}


void AresEdit3DView::ConstrainTransform (csReversibleTransform& tr,
    int mode, const csVector3& constrain,
    bool grid)
{
  csVector3 origin = tr.GetOrigin ();
  if (grid)
  {
    float m;
    m = fmod (origin.x, gridSize);
    origin.x -= m;
    m = fmod (origin.y, gridSize);
    origin.y -= m;
    m = fmod (origin.z, gridSize);
    origin.z -= m;
  }
  switch (mode)
  {
    case CONSTRAIN_NONE:
      break;
    case CONSTRAIN_XPLANE:
      origin.y = constrain.y;
      origin.z = constrain.z;
      break;
    case CONSTRAIN_YPLANE:
      origin.x = constrain.x;
      origin.z = constrain.z;
      break;
    case CONSTRAIN_ZPLANE:
      origin.x = constrain.x;
      origin.y = constrain.y;
      break;
  }
  tr.SetOrigin (origin);
}

void AresEdit3DView::PlacePasteMarker ()
{
  pasteMarker->SetVisible (true);
  constrainMarker->SetVisible (true);
  CreatePasteMarker ();

  csReversibleTransform tr = GetSpawnTransformation ();
  ConstrainTransform (tr, pasteConstrainMode, pasteConstrain, gridMode);
  pasteMarker->SetTransform (tr);
  csReversibleTransform ctr;
  ctr.SetOrigin (tr.GetOrigin ());
  constrainMarker->SetTransform (ctr);
}

void AresEdit3DView::StartPasteSelection ()
{
  pasteConstrainMode = CONSTRAIN_NONE;
  ShowConstrainMarker (false, true, false);
  todoSpawn = pastebuffer;
  if (IsPasteSelectionActive ())
    PlacePasteMarker ();
  app->SetMenuState ();
  app->SetStatus ("Left mouse to place objects. Right button to cancel. x/z to constrain placement. # for grid");
}

void AresEdit3DView::StartPasteSelection (const char* name)
{
  pasteConstrainMode = CONSTRAIN_NONE;
  ShowConstrainMarker (false, true, false);
  todoSpawn.Empty ();
  PasteContents apc;
  apc.useTransform = false;
  apc.dynfactName = name;
  todoSpawn.Push (apc);
  PlacePasteMarker ();
  app->SetMenuState ();
  app->SetStatus ("Left mouse to place objects. Right button to cancel. x/z to constrain placement. # for grid");
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

bool AresEdit3DView::TraceBeamHit (const csSegment3& beam, csVector3& isect)
{
  csVector3 isect1, isect2;

  // Trace the physical beam
  CS::Physics::Bullet::HitBeamResult result = GetBulletSystem ()->HitBeam (
      beam.Start (), beam.End ());
  if (result.body)
    isect1 = result.isect;

  csSectorHitBeamResult result2 = GetCsCamera ()->GetSector ()->HitBeamPortals (
      beam.Start (), beam.End ());
  if (result2.mesh)
    isect2 = result2.isect;

  if (!result2.mesh && !result.body) return false;
  if (!result2.mesh) { isect = isect1; return true; }
  if (!result.body) { isect = isect2; return true; }
  float sqdist1 = csSquaredDist::PointPoint (beam.Start (), isect1);
  float sqdist2 = csSquaredDist::PointPoint (beam.Start (), isect2);
  if (sqdist1 < sqdist2)
  {
    isect = isect1;
    return true;
  }
  else
  {
    isect = isect2;
    return true;
  }
}

iDynamicObject* AresEdit3DView::TraceBeam (const csSegment3& beam, csVector3& isect)
{
  csVector3 isect1, isect2;
  iDynamicObject* obj1 = 0, * obj2 = 0;

  // Trace the physical beam
  CS::Physics::Bullet::HitBeamResult result = GetBulletSystem ()->HitBeam (
      beam.Start (), beam.End ());
  if (result.body)
  {
    iRigidBody* hitBody = result.body->QueryRigidBody ();
    if (hitBody)
    {
      isect1 = result.isect;
      obj1 = dynworld->FindObject (hitBody);
    }
  }

  csSectorHitBeamResult result2 = GetCsCamera ()->GetSector ()->HitBeamPortals (
      beam.Start (), beam.End ());
  if (result2.mesh)
  {
    obj2 = dynworld->FindObject (result2.mesh);
    isect2 = result2.isect;
  }

  if (!obj2) { isect = isect1; return obj1; }
  if (!obj1) { isect = isect2; return obj2; }
  float sqdist1 = csSquaredDist::PointPoint (beam.Start (), isect1);
  float sqdist2 = csSquaredDist::PointPoint (beam.Start (), isect2);
  if (sqdist1 < sqdist2)
  {
    isect = isect1;
    return obj1;
  }
  else
  {
    isect = isect2;
    return obj2;
  }
}

bool AresEdit3DView::OnMouseDown (iEvent& ev)
{
  uint but = csMouseEventHelper::GetButton (&ev);
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

  if (IsPasteSelectionActive ())
  {
    if (but == csmbLeft)
    {
      PasteSelection ();
      StopPasteMode ();
    }
    else if (but == csmbRight)
    {
      StopPasteMode ();
    }
    return true;
  }

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

//---------------------------------------------------------------------------
/*
void AresEdit3DView::EnableRagdoll ()
{
  using namespace CS::Animation;
  using namespace CS::Mesh;

  csArray<iDynamicObject*> objects = selection->GetObjects ();
  SelectionIterator it = objects.GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    if (!dynobj->GetMesh ()) continue;
    csString factName = dynobj->GetFactory ()->GetName ();
    iMeshFactoryWrapper* factory = engine->FindMeshFactory (factName);
    CS_ASSERT (factory != 0);

    csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory = scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
      (factory->GetMeshObjectFactory ());
    if (!animeshFactory)
    {
      app->GetUIManager ()->Error ("'%s' is not an animesh!", factName.GetData ());
      return;
    }

    csRef<iBodyManager> bodyManager = csQueryRegistry<iBodyManager> (object_reg);
    csRef<iBodySkeleton> bodySkeleton = bodyManager->FindBodySkeleton (factName);
    if (!bodySkeleton)
    {
      app->GetUIManager ()->Error ("'%s' has no body skeleton!", factName.GetData ());
      return;
    }

    csRef<iSkeletonAnimPacketFactory> animPacketFactory = animeshFactory->GetSkeletonFactory ()->GetAnimationPacket ();

    csRef<iSkeletonRagdollNodeManager> ragdollManager = csQueryRegistryOrLoad<iSkeletonRagdollNodeManager>
      (object_reg, "crystalspace.mesh.animesh.animnode.ragdoll");

    csRef<iSkeletonRagdollNodeFactory> ragdollNodeFactory =
      ragdollManager->CreateAnimNodeFactory ("ragdoll");
    ragdollNodeFactory->SetBodySkeleton (bodySkeleton);

    animPacketFactory->SetAnimationRoot (ragdollNodeFactory);

    iBodyChain* bodyChain = bodySkeleton->CreateBodyChain ("chain", animeshFactory->GetSkeletonFactory ()->FindBone ("BoneLid"));
    bodyChain->AddAllSubChains ();
    ragdollNodeFactory->AddBodyChain (bodyChain, CS::Animation::STATE_KINEMATIC);

    csRef<CS::Mesh::iAnimatedMesh> animesh = scfQueryInterface<CS::Mesh::iAnimatedMesh> (dynobj->GetMesh ()->GetMeshObject ());
    animesh->GetSkeleton ()->RecreateAnimationTree ();
    iSkeletonAnimNode* rootNode = animesh->GetSkeleton ()->GetAnimationPacket ()->GetAnimationRoot ();
    csRef<iSkeletonRagdollNode> ragdollNode =
      scfQueryInterface<iSkeletonRagdollNode> (rootNode->FindNode ("ragdoll"));
    ragdollNode->SetDynamicSystem (dynSys);
  }
}
*/
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
  objectsValue->RefreshModel ();
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
  pl->RemoveEntityTemplates ();
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

  iSectorList* sl = engine->GetSectors ();
  if (sl->GetCount () > 0)
  {
    sector = sl->Get (0);
    CreateCell (sector->QueryObject ()->GetName ());
  }
  else
  {
    sector = 0;
  }

  // @@@ Error handling.
  SetupDynWorld ();

  // @@@ Error handling.
  PostLoadMap ();
}

bool AresEdit3DView::LoadFile (const char* filename)
{
  CleanupWorld ();
  SetupWorld ();

  if (!worldLoader->LoadFile (filename))
  {
    PostLoadMap ();
    app->GetUIManager ()->Error ("Error loading file '%s'!",filename);
    return false;
  }

  // @@@ Hardcoded sector name!
  //sector = engine->FindSector ("outside");

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

  if (!SetupDynWorld ())
    return false;

  if (!PostLoadMap ())
    return false;

  objectsValue->BuildModel ();

  return true;
}

void AresEdit3DView::OnExit ()
{
}

Ares::Value* AresEdit3DView::GetDynfactCollectionValue () const
{
  return dynfactCollectionValue;
}

Ares::Value* AresEdit3DView::GetObjectsValue () const
{
  return objectsValue;
}

iDynamicObject* AresEdit3DView::GetDynamicObjectFromObjects (Ares::Value* value)
{
  DynobjValue* dv = static_cast<DynobjValue*> (value);
  return dv->GetDynamicObject ();
}

size_t AresEdit3DView::GetDynamicObjectIndexFromObjects (iDynamicObject* dynobj)
{
  return objectsValue->FindDynObj (dynobj);
}

void AresEdit3DView::AddItem (const char* category, const char* itemname)
{
  if (!categories.In (category))
    categories.Put (category, csStringArray());
  csStringArray a;
  categories.Get (category, a).Push (itemname);
}

void AresEdit3DView::RemoveItem (const char* category, const char* itemname)
{
  if (!categories.In (category)) return;
  csStringArray a;
  csStringArray& items = categories.Get (category, a);
  size_t idx = items.Find (itemname);
  if (idx != csArrayItemNotFound)
    items.DeleteIndex (idx);
}

void AresEdit3DView::ChangeCategory (const char* newCategory, const char* itemname)
{
  csHash<csStringArray,csString>::GlobalIterator it = categories.GetIterator ();
  while (it.HasNext ())
  {
    csString category;
    csStringArray& items = it.Next (category);
    size_t idx = items.Find (itemname);
    if (idx != csArrayItemNotFound)
    {
      items.DeleteIndex (idx);
      break;
    }
  }
  AddItem (newCategory, itemname);
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
printf ("view_wh=%d,%d new_wh=%d,%d\n", view_width, view_height, width, height);

#if 0
  // We use the full window to draw the world.
  float scale_x = ((float)width)  / ((float)view_width);
  float scale_y = ((float)height) / ((float)view_height);

  view->GetPerspectiveCamera()->SetPerspectiveCenter (
      view->GetPerspectiveCamera()->GetShiftX() * scale_x,
      view->GetPerspectiveCamera()->GetShiftY() * scale_y);

  view->GetPerspectiveCamera()->SetFOVAngle (
      view->GetPerspectiveCamera()->GetFOVAngle(), width);
#endif

  view_width = width;
  view_height = height;

  //view->GetPerspectiveCamera ()->SetFOV ((float) (width) / (float) (height), 1.0f);
  view->SetRectangle (0, 0, view_width, view_height, false);
}

iAresEditor* AresEdit3DView::GetApplication  ()
{
  return static_cast<iAresEditor*> (app);
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

  zoneEntity = pl->CreateEntity ("World", 0, 0,
      "pcworld.dynamic", "pcphysics.system", CEL_PROPCLASS_END);
  if (!zoneEntity) return app->ReportError ("Failed to create zone entity!");
  dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (zoneEntity);
  {
    csRef<iDynamicCellCreator> cellCreator;
    cellCreator.AttachNew (new AresDynamicCellCreator (this));
    dynworld->SetDynamicCellCreator (cellCreator);
  }

  elcm = csQueryRegistry<iELCM> (r);
  dynworld->SetELCM (elcm);
  dynworld->InhibitEntities (true);
  dynworld->EnableGameMode (false);

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

  pasteMarker = markerMgr->CreateMarker ();
  pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (1,0,0), white, true);
  pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,1,0), white, true);
  pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,0,1), white, true);
  pasteMarker->SetVisible (false);

  constrainMarker = markerMgr->CreateMarker ();
  HideConstrainMarker ();

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

void AresEdit3DView::RefreshFactorySettings (iDynamicFactory* fact)
{
  csBox3 bbox = fact->GetPhysicsBBox ();
  factory_to_origin_offset.PutUnique (fact->GetName (), bbox.MinY ());
  const char* st = fact->GetAttribute ("defaultstatic");
  if (st && *st == 't') static_factories.Add (fact->GetName ());
  else static_factories.Delete (fact->GetName ());
}

bool AresEdit3DView::SetupDynWorld ()
{
  csString playerName = "Player";
  csString nodeName = "Node";
  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    if (playerName == fact->GetName () || nodeName == fact->GetName ()) continue;
    if (curvedFactories.Find (fact) != csArrayItemNotFound) continue;
    if (roomFactories.Find (fact) != csArrayItemNotFound) continue;
    printf ("%d %s\n", int (i), fact->GetName ()); fflush (stdout);
    RefreshFactorySettings (fact);
    const char* category = fact->GetAttribute ("category");
    AddItem (category, fact->GetName ());
  }

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
  csRef<iDynamicCellIterator> it = dynworld->GetCells ();
  if (it->HasNext ())
  {
    csRef<iDynamicCell> cell = it->NextCell ();
    dynworld->SetCurrentCell (cell);
    sector = cell->GetSector ();
    dyncell = cell;
    dynSys = dyncell->GetDynamicSystem ();
    bullet_dynSys = scfQueryInterface<CS::Physics::Bullet::iDynamicSystem> (dynSys);
  }
  else
  {
    dyncell = 0;
    dynSys = 0;
    bullet_dynSys = 0;
    sector = 0;
  }

  // Initialize collision objects for all loaded objects.
  csColliderHelper::InitializeCollisionWrappers (cdsys, engine);

  // @@@ Bad: hardcoded terrain name! Don't do this at home!
  if (sector)
    terrainMesh = sector->GetMeshes ()->FindByName ("Terrain");
  else
    terrainMesh = 0;

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


  if (sector)
  {
    nature->InitSector (sector);

    iLightList* lightList = sector->GetLights ();
    camlight = engine->CreateLight(0, csVector3(0.0f, 0.0f, 0.0f), 10, csColor (0.8f, 0.9f, 1.0f));
    lightList->Add (camlight);
  }
  else
    camlight = 0;

  engine->Prepare ();
  //CS::Lighting::SimpleStaticLighter::ShineLights (sector, engine, 4);

  // Setup the camera.
  camera->Init (view->GetCamera (), sector, csVector3 (0, 10, 0));

  // Force the update of the clock.
  nature->UpdateTime (currentTime+100, GetCsCamera ());
  nature->UpdateTime (currentTime, GetCsCamera ());

  // Make the 'Player' and 'World' entity templates if they don't already
  // exist. It is possible that they exist because we loaded a project that defined
  // them.
  if (!pl->FindEntityTemplate ("Player"))
  {
    if (!LoadLibrary ("/appdata/", "player.xml"))
      return app->ReportError ("Error loading player library!");
  }
  if (!pl->FindEntityTemplate ("World"))
  {
    if (!LoadLibrary ("/appdata/", "world.xml"))
      return app->ReportError ("Error loading world library!");
  }

  return true;
}

void AresEdit3DView::WarpCell (iDynamicCell* cell)
{
  if (cell == dynworld->GetCurrentCell ()) return; 
  dyncell = cell;
  dynSys = dyncell->GetDynamicSystem ();
  bullet_dynSys = scfQueryInterface<CS::Physics::Bullet::iDynamicSystem> (dynSys);

  if (sector && camlight) sector->GetLights ()->Remove (camlight);
  camlight = 0;
  dynworld->SetCurrentCell (cell);
  sector = engine->FindSector (cell->GetName ());
  nature->InitSector (sector);
  camlight = engine->CreateLight(0, csVector3(0.0f, 0.0f, 0.0f), 10, csColor (0.8f, 0.9f, 1.0f));
  iLightList* lightList = sector->GetLights ();
  lightList->Add (camlight);

  camera->Init (view->GetCamera (), sector, csVector3 (0, 10, 0));
}

bool AresEdit3DView::SetupWorld ()
{
  if (!LoadLibrary ("/aresnode/", "library"))
    return app->ReportError ("Error loading library!");
  vfs->PopDir ();
  vfs->Unmount ("/aresnode", "data$/node.zip");

  //sector = engine->CreateSector ("outside");

  //dyncell = dynworld->AddCell ("outside", sector, dynSys);
  //dynworld->SetCurrentCell (dyncell);
  dyncell = 0;
  dynSys = 0;
  bullet_dynSys = 0;
  sector = 0;

  ClearItems ();
  if (!dynworld->FindFactory ("Node"))
  {
    iDynamicFactory* fact = dynworld->AddFactory ("Node", 1.0, -1);
    fact->AddRigidBox (csVector3 (0.0f), csVector3 (0.2f), 1.0f);
    AddItem ("Nodes", "Node");
  }
  if (!dynworld->FindFactory ("Player"))
  {
    iDynamicFactory* fact = dynworld->AddFactory ("Player", 1.0, -1);
    fact->AddRigidBox (csVector3 (0.0f), csVector3 (0.2f), 1.0f);
    AddItem ("Nodes", "Player");
  }

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

csVector3 AresEdit3DView::GetBeamPosition (const char* fname)
{
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
  return newPosition;
}

void AresEdit3DView::ShowConstrainMarker (bool constrainx, bool constrainy, bool constrainz)
{
  constrainMarker->SetVisible (true);
  constrainMarker->Clear ();
  if (!constrainx)
  {
    iMarkerColor* red = markerMgr->FindMarkerColor ("red");
    constrainMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (1,0,0), red, true);
    constrainMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (-1,0,0), red, true);
  }
  if (!constrainy)
  {
    iMarkerColor* green = markerMgr->FindMarkerColor ("green");
    constrainMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,1,0), green, true);
    constrainMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,-1,0), green, true);
  }
  if (!constrainz)
  {
    iMarkerColor* blue = markerMgr->FindMarkerColor ("blue");
    constrainMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,0,1), blue, true);
    constrainMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,0,-1), blue, true);
  }
}

void AresEdit3DView::MoveConstrainMarker (const csReversibleTransform& trans)
{
  constrainMarker->SetTransform (trans);
}

void AresEdit3DView::HideConstrainMarker ()
{
  constrainMarker->SetVisible (false);
}

void AresEdit3DView::SetPasteConstrain (int mode)
{
  pasteConstrainMode = mode;
  ShowConstrainMarker (mode & CONSTRAIN_ZPLANE, true, mode & CONSTRAIN_XPLANE);
  if (todoSpawn[0].useTransform)
    pasteConstrain = todoSpawn[0].trans.GetOrigin ();
  else
  {
    csReversibleTransform tr = GetSpawnTransformation ();
    pasteConstrain = tr.GetOrigin ();
  }
}

csReversibleTransform AresEdit3DView::GetSpawnTransformation ()
{
  csReversibleTransform tr = todoSpawn[0].trans;
  if (!todoSpawn[0].useTransform)
  {
    tr = GetCsCamera ()->GetTransform ();
    csVector3 front = tr.GetFront ();
    front.y = 0;
    tr.LookAt (front, csVector3 (0, 1, 0));
  }
  tr.SetOrigin (csVector3 (0));

  const char* name = todoSpawn[0].dynfactName;
  csString fname;
  CurvedFactoryCreator* cfc = FindFactoryCreator (name);
  RoomFactoryCreator* rfc = FindRoomFactoryCreator (name);
  if (cfc)
    fname.Format("%s%d", name, curvedFactoryCounter+1);
  else if (rfc)
    fname.Format("%s%d", name, roomFactoryCounter+1);
  else
    fname = name;

  csVector3 newPosition = GetBeamPosition (fname);

  csReversibleTransform tc = GetCsCamera ()->GetTransform ();
  csVector3 front = tc.GetFront ();
  front.y = 0;
  tc.LookAt (front, csVector3 (0, 1, 0));
  tc = tr;
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

  csVector3 newPosition = GetBeamPosition (fname);

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
  ConstrainTransform (tc, pasteConstrainMode, pasteConstrain, gridMode);
  //pasteConstrainMode = CONSTRAIN_NONE;

  iDynamicObject* dynobj = dyncell->AddObject (fname, tc);
  if (!dynobj)
  {
    app->GetUIManager ()->Error ("Could not create object for '%s'!", fname.GetData ());
    return 0;
  }
  dynobj->SetEntity (0, fname, 0);
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
    app->SwitchToMode ("Curve");
  else if (roomFactory)
    app->SwitchToMode ("Room");
  return dynobj;
}

iDynamicCell* AresEdit3DView::CreateCell (const char* name)
{
  iSector* s = engine->FindSector (name);
  if (!s)
    s = engine->CreateSector (name);

  dyn = csQueryRegistry<iDynamics> (GetObjectRegistry ());
  if (!dyn) { app->ReportError ("Error loading bullet plugin!"); return 0; }

  csString systemname = "ares.dynamics.system.";
  systemname += name;
  csRef<iDynamicSystem> ds = dyn->FindSystem (systemname);
  if (!ds)
  {
    ds = dyn->CreateSystem ();
    ds->QueryObject ()->SetName (systemname);
  }
  if (!ds) { app->ReportError ("Error creating dynamic system!"); return 0; }

  //ds->SetLinearDampener(.3f);
  ds->SetRollingDampener(.995f);
  ds->SetGravity (csVector3 (0.0f, -19.81f, 0.0f));

  csRef<CS::Physics::Bullet::iDynamicSystem> bullet_ds = scfQueryInterface<
    CS::Physics::Bullet::iDynamicSystem> (ds);
  //@@@ (had to disable because bodies might alredy exist!) bullet_ds->SetInternalScale (1.0f);
  bullet_ds->SetStepParameters (0.005f, 2, 10);

  iDynamicCell* cell = dynworld->AddCell (name, s, ds);
  return cell;
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

iParameterManager* AresEdit3DView::GetPM ()
{
  if (pm) return pm;
  pm = csQueryRegistryOrLoad<iParameterManager> (object_reg, "cel.parameters.manager");
  return pm;
}

void AresEdit3DView::JoinObjects ()
{
  if (selection->GetSize () != 2)
  {
    app->GetUIManager ()->Error ("Select two objects to join!");
    return;
  }
  const csArray<iDynamicObject*>& ob = selection->GetObjects ();
  if (ob[0]->GetFactory ()->GetJointCount () == 0)
  {
    app->GetUIManager ()->Error ("The first object has no joints!");
    return;
  }
  // In this function all joints are connected between the two same objects.
  for (size_t i = 0 ; i < ob[0]->GetFactory ()->GetJointCount () ; i++)
    ob[0]->Connect (i, ob[1]);
}

void AresEdit3DView::UnjoinObjects ()
{
  if (selection->GetSize () == 0)
  {
    app->GetUIManager ()->Error ("Select at least one object to unjoin!");
    return;
  }
  const csArray<iDynamicObject*>& ob = selection->GetObjects ();
  size_t count = ob[0]->GetFactory ()->GetJointCount ();
  if (count == 0)
  {
    app->GetUIManager ()->Error ("The first object has no joints!");
    return;
  }
  if (selection->GetSize () == 1)
  {
    int removed = 0;
    for (size_t i = 0 ; i < count ; i++)
      if (ob[0]->GetConnectedObject (i))
      {
	ob[0]->Connect (i, 0);
	removed++;
      }
    app->GetUIManager ()->Message ("Disconnected %d objects.", removed);
  }
  else
  {
    for (size_t i = 1 ; i < ob.GetSize () ; i++)
    {
      bool found = false;
      for (size_t j = 0 ; j < ob[0]->GetFactory ()->GetJointCount () ; j++)
	if (ob[0]->GetConnectedObject (j) == ob[i]) { found = true; break; }
      if (!found)
      {
        app->GetUIManager ()->Error ("Some of the objects are not connected!");
        return;
      }
    }
    for (size_t i = 1 ; i < ob.GetSize () ; i++)
    {
      for (size_t j = 0 ; j < ob[0]->GetFactory ()->GetJointCount () ; j++)
	if (ob[0]->GetConnectedObject (j) == ob[i])
	{
	  ob[0]->Connect (j, 0);
	  break;
	}
    }
  }
}

void AresEdit3DView::UpdateObjects ()
{
  if (!app->GetUIManager ()->Ask ("Updating all objects in this cell? Are you sure?")) return;
  dynworld->UpdateObjects (dyncell);
}

// =========================================================================

BEGIN_EVENT_TABLE(AppAresEditWX, wxFrame)
  EVT_SHOW (AppAresEditWX::OnShow)
  EVT_ICONIZE (AppAresEditWX::OnIconize)
  EVT_MENU (wxID_ANY, AppAresEditWX :: OnMenuItem)
  EVT_NOTEBOOK_PAGE_CHANGING (XRCID("mainNotebook"), AppAresEditWX :: OnNotebookChange)
  EVT_NOTEBOOK_PAGE_CHANGED (XRCID("mainNotebook"), AppAresEditWX :: OnNotebookChanged)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AppAresEditWX::Panel, wxPanel)
  EVT_SIZE(AppAresEditWX::Panel::OnSize)
END_EVENT_TABLE()

// The global pointer to AresEd
AppAresEditWX* aresed = 0;

AppAresEditWX::AppAresEditWX (iObjectRegistry* object_reg, int w, int h)
  : wxFrame (0, -1, wxT ("AresEd"), wxDefaultPosition, wxSize (w, h)),
    scfImplementationType (this)
{
  AppAresEditWX::object_reg = object_reg;
  camwin = 0;
  editMode = 0;
  //oldPageIdx = csArrayItemNotFound;
  FocusLost = csevFocusLost (object_reg);
  config.AttachNew (new AresConfig (this));
  wantsFocus3D = 0;
}

AppAresEditWX::~AppAresEditWX ()
{
  delete camwin;
}

iUIManager* AppAresEditWX::GetUI () const
{
  return static_cast<iUIManager*> (uiManager);
}

i3DView* AppAresEditWX::Get3DView () const
{
  return static_cast<i3DView*> (aresed3d);
}

void AppAresEditWX::FindObject ()
{
  UIDialog* dialog = new UIDialog (this, "Select an object", 500, 300);
  dialog->AddRow (1);
  dialog->AddListIndexed ("objects", aresed3d->GetObjectsValue (), 0,
      "ID,Entity,Factory,X,Y,Z,Distance",
      DYNOBJ_COL_ID,
      DYNOBJ_COL_ENTITY,
      DYNOBJ_COL_FACTORY,
      DYNOBJ_COL_X,
      DYNOBJ_COL_Y,
      DYNOBJ_COL_Z,
      DYNOBJ_COL_DISTANCE);
  if (dialog->Show (0))
  {
    const DialogResult& rc = dialog->GetFieldContents ();
    csString idValue = rc.Get ("objects", (const char*)0);
    if (!idValue.IsEmpty ())
    {
      uint id;
      csScanStr (idValue, "%d", &id);
      iDynamicObject* dynobj = aresed3d->GetDynamicWorld ()->FindObject (id);
      printf ("Find object %p with id %d\n", dynobj, id);
      if (dynobj)
      {
	aresed3d->GetSelection ()->SetCurrentObject (dynobj);
      }
    }
  }
  delete dialog;
}

void AppAresEditWX::RefreshModes ()
{
  for (size_t i = 0 ; i < plugins.GetSize () ; i++)
  {
    csRef<iEditingMode> mode = scfQueryInterface<iEditingMode> (plugins.Get (i));
    if (mode)
      mode->Refresh ();
  }
}

void AppAresEditWX::NewProject (const csArray<Asset>& assets)
{
  aresed3d->NewProject (assets);
  RefreshModes ();
}

bool AppAresEditWX::LoadFile (const char* filename)
{
  if (!aresed3d->LoadFile (filename))
    return false;
  RefreshModes ();
  return true;
}

void AppAresEditWX::SaveFile (const char* filename)
{
  aresed3d->SaveFile (filename);
}

void AppAresEditWX::NewProject ()
{
  csRef<NewProjectCallbackImp> cb;
  cb.AttachNew (new NewProjectCallbackImp (this));
  uiManager->GetNewProjectDialog ()->Show (cb);
}

bool AppAresEditWX::IsPlaying () const
{
  return editMode == playMode;
}

void AppAresEditWX::OpenFile ()
{
  csRef<LoadCallback> cb;
  cb.AttachNew (new LoadCallback (this));
  uiManager->GetFileReqDialog ()->Show (cb);
}

void AppAresEditWX::SaveFile ()
{
  csRef<SaveCallback> cb;
  cb.AttachNew (new SaveCallback (this));
  uiManager->GetFileReqDialog ()->Show (cb);
}

void AppAresEditWX::Quit ()
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

  csRef<iEditingMode> newMode;
  csRef<iEditorPlugin> plugin = FindPlugin (pageName.mb_str (wxConvUTF8));
  if (!plugin) return;
  newMode = scfQueryInterface<iEditingMode> (plugin);
  if (!newMode) return;

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
    if (aresed3d) DoFrame ();
    return true;
  }
  else if (CS_IS_KEYBOARD_EVENT(object_reg, ev))
  {
    utf32_char code = csKeyEventHelper::GetCookedCode (&ev);
    if (csKeyEventHelper::GetEventType (&ev) == csKeyEventTypeDown)
      return editMode->OnKeyboard (ev, code);
  }
  else if ((ev.Name == MouseMove))
  {
    int mouseX = csMouseEventHelper::GetX (&ev);
    int mouseY = csMouseEventHelper::GetY (&ev);
    if (aresed3d)
    {
      if (!IsPlaying () && aresed3d->OnMouseMove (ev))
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
      if (!IsPlaying () &&
	  but == csmbRight &&
	  editMode->IsContextMenuAllowed () &&
	  !aresed3d->IsPasteSelectionActive ())
      {
	wxMenu contextMenu;
	bool camwinVis = camwin->IsVisible ();
	if (camwinVis)
	{
	  camwin->AddContextMenu (&contextMenu, mouseX, mouseY);
	}
	editMode->AddContextMenu (&contextMenu, mouseX, mouseY);

	PopupMenu (&contextMenu);
      }
      else
      {
        if (!IsPlaying () && aresed3d->OnMouseDown (ev))
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
      if (!IsPlaying () && aresed3d->OnMouseUp (ev))
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
	wxString::FromUTF8 (filename))
      || !wxXmlResource::Get ()->Load (resourceLocation))
    return ReportError ("Could not load XRC resource file: %s!", filename);
  return true;
}

bool AppAresEditWX::Initialize ()
{
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

  engine = csQueryRegistry<iEngine> (object_reg);
  if (!engine) return ReportError ("Can't find the engine plugin!");

  if (!config->ReadConfig ())
    return false;

  if (!InitWX ())
    return false;

  printer.AttachNew (new FramePrinter (object_reg));

  if (!ParseCommandLine ())
    return false;
  return true;
}

bool AppAresEditWX::ParseCommandLine ()
{
  csRef<iCommandLineParser> cmdline (
  	csQueryRegistry<iCommandLineParser> (object_reg));
  const char* val = cmdline->GetName ();
  if (val)
  {
    if (!LoadFile (val))
      return false;
  }
  return true;
}

bool AppAresEditWX::InitPlugins ()
{
  const csArray<PluginConfig>& configplug = config->GetPlugins ();
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  for (size_t i = 0 ; i < configplug.GetSize () ; i++)
  {
    const PluginConfig& pc = configplug.Get (i);
    csString plugin = pc.plugin;
    printf ("Loading editor plugin %s\n", plugin.GetData ());
    fflush (stdout);

    csRef<iDataBuffer> buf = vfs->GetRealPath ("/ares/data/windows");
    csString path (buf->GetData ());
    wxString searchPath (wxString::FromUTF8 (path));
    for (size_t j = 0 ; j < pc.resources.GetSize () ; j++)
    {
      if (!LoadResourceFile (pc.resources.Get (j), searchPath)) return false;
    }

    csRef<iEditorPlugin> plug = csLoadPluginCheck<iEditorPlugin> (object_reg, plugin);
    if (!plug)
    {
      return ReportError ("Could not load plugin '%s'!", plugin.GetData ());
    }
    csString pluginName = plug->GetPluginName ();

    plug->SetApplication (static_cast<iAresEditor*> (this));

    if (pc.addToNotebook)
    {
      wxPanel* panel = new wxPanel (notebook, wxID_ANY, wxDefaultPosition,
	wxDefaultSize, wxTAB_TRAVERSAL);
      notebook->AddPage (panel, wxString::FromUTF8 (pluginName), false);
      panel->SetToolTip (wxString::FromUTF8 (pc.tooltip));
      wxBoxSizer* sizer = new wxBoxSizer (wxVERTICAL);
      panel->SetSizer (sizer);
      panel->Layout ();
      plug->SetParent (panel);
    }
    else
    {
      plug->SetParent (this);
    }

    csRef<iEditingMode> emode = scfQueryInterface<iEditingMode> (plug);
    if (emode)
    {
      emode->AllocContextHandlers (this);
      if (pc.mainMode)
      {
	mainMode = emode;
	mainModeName = pluginName;
      }
      else if (pluginName == "Play")
	playMode = emode;
    }
    if (pluginName == "DynfactEditor")
      dynfactDialog = plug;

    plugins.Push (plug);
  }

  return true;
}

bool AppAresEditWX::InitWX ()
{
  // Load the frame from an XRC file
  wxXmlResource::Get ()->InitAllHandlers ();

  csRef<iDataBuffer> buf = vfs->GetRealPath ("/ares/data/windows");
  csString path (buf->GetData ());
  wxString searchPath (wxString::FromUTF8 (path));
  if (!LoadResourceFile ("AresMainFrame.xrc", searchPath)) return false;
  if (!LoadResourceFile ("FileRequester.xrc", searchPath)) return false;
  if (!LoadResourceFile ("CameraPanel.xrc", searchPath)) return false;
  if (!LoadResourceFile ("NewProjectDialog.xrc", searchPath)) return false;
  if (!LoadResourceFile ("CellDialog.xrc", searchPath)) return false;

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

  uiManager.AttachNew (new UIManager (this, wxwindow->GetWindow ()));

  if (!InitPlugins ()) return false;
  editMode = 0;

  wxPanel* leftPanePanel = XRCCTRL (*this, "leftPanePanel", wxPanel);
  leftPanePanel->SetMinSize (wxSize (270,-1));
  camwin = new CameraWindow (leftPanePanel, aresed3d);
  camwin->AllocContextHandlers (this);

  SelectionListener* listener = new AppSelectionListener (this);
  aresed3d->GetSelectionInt ()->AddSelectionListener (listener);

  RefreshModes ();

  if (!SetupMenuBar ())
    return false;

  SwitchToMainMode ();

  return true;
}

void AppAresEditWX::SetFocus3D ()
{
  wantsFocus3D = 10;
}

void AppAresEditWX::SetStatus (const char* statusmsg, ...)
{
  va_list args;
  va_start (args, statusmsg);
  csString str;
  str.FormatV (statusmsg, args);
  va_end (args);
  if (GetStatusBar ())
    GetStatusBar ()->SetStatusText (wxString::FromUTF8 (str), 0);
}

void AppAresEditWX::ClearStatus ()
{
  if (editMode)
  {
    SetStatus ("%s", editMode->GetStatusLine ()->GetData ());
  }
  else
    SetStatus ("");
}

bool AppAresEditWX::Command (const char* name, const char* args)
{
  csString c = name;
  if (c == "NewProject") NewProject ();
  else if (c == "Open") OpenFile ();
  else if (c == "Save") SaveFile ();
  else if (c == "Exit") Quit ();
  else if (c == "Copy") aresed3d->CopySelection ();
  else if (c == "Paste") aresed3d->StartPasteSelection ();
  else if (c == "Delete") aresed3d->DeleteSelectedObjects ();
  else if (c == "UpdateObjects") aresed3d->UpdateObjects ();
  else if (c == "FindObjectDialog") FindObject ();
  else if (c == "Join") aresed3d->JoinObjects ();
  else if (c == "Unjoin") aresed3d->UnjoinObjects ();
  else if (c == "ManageCells") uiManager->GetCellDialog ()->Show ();
  else if (c == "SwitchMode") SwitchToMode (args);
  else return false;
  return true;
}

bool AppAresEditWX::IsCommandValid (const char* name, const char* args,
      iSelection* selection, bool haspaste,
      const char* currentmode)
{
  size_t selsize = selection->GetSize ();
  csString mode = currentmode;
  csString c = name;
  if (c == "Copy") return selsize > 0 && mode == "Main";
  if (c == "Paste") return haspaste && mode == "Main";
  if (c == "Delete") return selsize > 0 && mode == "Main";
  if (c == "Join") return selsize == 2 && mode == "Main";
  if (c == "Unjoin") return selsize > 0 && mode == "Main";
  return true;
}

void AppAresEditWX::OnMenuItem (wxCommandEvent& event)
{
  int id = event.GetId ();
  MenuCommand mc = menuCommands.Get (id, MenuCommand ());
  if (mc.command.IsEmpty ())
  {
    printf ("Unhandled menu item %d!\n", id);
    fflush (stdout);
    return;
  }
  mc.target->Command (mc.command, mc.args);
}

void AppAresEditWX::AppendMenuItem (wxMenu* menu, int id, const char* label,
    const char* targetName, const char* command, const char* args)
{
  csRef<iCommandHandler> target;
  if (targetName && *targetName)
  {
    iEditorPlugin* plug = FindPlugin (targetName);
    target = scfQueryInterface<iCommandHandler> (plug);
    if (!target)
      ReportError ("Target '%s' has no command handler!", targetName);
  }
  if (!target) target = static_cast<iCommandHandler*> (this);

  menu->Append (id, wxString::FromUTF8 (label));
  MenuCommand mc;
  mc.target = target;
  mc.command = command;
  mc.args = args;

  menuCommands.Put (id, mc);
}

bool AppAresEditWX::SetupMenuBar ()
{
  wxMenuBar* menuBar = config->BuildMenuBar ();
  if (!menuBar) return false;
  SetMenuBar (menuBar);
  menuBar->Reparent (this);

  CreateStatusBar ();
  SetMenuState ();
  return true;
}

void AppAresEditWX::ShowCameraWindow ()
{
  camwin->Show ();
}

void AppAresEditWX::HideCameraWindow ()
{
  camwin->Hide ();
}

void AppAresEditWX::SetMenuState ()
{
  ClearStatus ();
  wxMenuBar* menuBar = GetMenuBar ();
  if (!menuBar) return;

  // Should menus be globally disabled?
  bool dis = aresed3d ? aresed3d->IsPasteSelectionActive () : true;
  if (IsPlaying ()) dis = true;
  if (dis)
  {
    menuBar->EnableTop (0, false);
    return;
  }
  menuBar->EnableTop (0, true);

  csString n;
  if (editMode)
  {
    csRef<iEditorPlugin> plug = scfQueryInterface<iEditorPlugin> (editMode);
    n = plug->GetPluginName ();
  }

  csHash<MenuCommand,int>::GlobalIterator it = menuCommands.GetIterator ();
  while (it.HasNext ())
  {
    int id;
    const MenuCommand& mc = it.Next (id);
    bool enabled = mc.target->IsCommandValid (mc.command, mc.args,
	aresed3d->GetSelection (), aresed3d->IsClipboardFull (), n);
    menuBar->Enable (id, enabled);
  }
}

void AppAresEditWX::PushFrame ()
{
  static bool lock = false;
  if (lock) return;
  lock = true;

  if (wantsFocus3D)
  {
    wantsFocus3D--;
    if (wantsFocus3D <= 0)
    {
      printf ("Setting focus for real!\n");
      wxwindow->GetWindow ()->SetFocus ();
    }
  }

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

iEditorConfig* AppAresEditWX::GetConfig () const
{
  return static_cast<iEditorConfig*> (config);
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

void AppAresEditWX::SwitchToMainMode ()
{
  SwitchToMode (mainModeName);
}

void AppAresEditWX::SetCurveModeEnabled (bool cm)
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  size_t pageIdx = FindNotebookPage (notebook, "Curve");
  wxNotebookPage* page = notebook->GetPage (pageIdx);
  if (cm) page->Enable ();
  else page->Disable ();
}

iEditorPlugin* AppAresEditWX::FindPlugin (const char* name)
{
  csString n = name;
  for (size_t i = 0 ; i < plugins.GetSize () ; i++)
  {
    if (n == plugins.Get (i)->GetPluginName ())
      return plugins.Get (i);
  }
  return 0;
}

void AppAresEditWX::SwitchToMode (const char* name)
{
  printf ("SwitchToMode %s\n", name); fflush (stdout);
  iEditorPlugin* plugin = FindPlugin (name);
  csRef<iEditingMode> mode;
  if (plugin) mode = scfQueryInterface<iEditingMode> (plugin);
  if (mode)
  {
    wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
    size_t pageIdx = FindNotebookPage (notebook, name);
    if (pageIdx != csArrayItemNotFound)
      notebook->ChangeSelection (pageIdx);
    if (editMode) editMode->Stop ();
    editMode = mode;
    editMode->Start ();
    SetMenuState ();
  }
  else
  {
    CS_ASSERT (false);
    printf ("Can't find mode '%s'!\n", name);
    fflush (stdout);
  }
}


