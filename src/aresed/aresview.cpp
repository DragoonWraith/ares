/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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
#include "aresview.h"
#include "include/imarker.h"
#include "ui/uimanager.h"
#include "ui/filereq.h"
#include "ui/celldialog.h"
#include "ui/entityparameters.h"
#include "camera.h"
#include "editor/imode.h"
#include "editor/iplugin.h"

#include "celtool/initapp.h"
#include "cstool/simplestaticlighter.h"
#include "celtool/persisthelper.h"
#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/parameters.h"
#include "tools/questmanager.h"

#include <csgeom/math3d.h>
#include "camerawin.h"
#include "selection.h"
#include "models/dynfactmodel.h"
#include "models/factories.h"
#include "models/objects.h"
#include "models/templates.h"
#include "models/assets.h"
#include "models/resources.h"
#include "models/quests.h"
#include "iassetmanager.h"
#include "edcommon/transformtools.h"
#include "edcommon/inspect.h"


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
  do_simulation = true;
  currentTime = 31000;
  do_auto_time = false;
  curvedFactoryCounter = 0;
  roomFactoryCounter = 0;
  selection = 0;
  FocusLost = csevFocusLost (object_reg);
  modelRepository.AttachNew (new ModelRepository (this, app));
  camera.AttachNew (new Camera (this));
  dyncell = 0;
  sector = 0;
  terrainMesh = 0;
}

AresEdit3DView::~AresEdit3DView()
{
  delete selection;
}

void AresEdit3DView::Frame (iEditingMode* editMode)
{
  if (paster->IsPasteSelectionActive ()) paster->PlacePasteMarker ();

  g3d->BeginDraw( CSDRAW_3DGRAPHICS);
  if (GetCsCamera ()->GetSector () == 0)
    g3d->GetDriver2D ()->Clear (0);
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

  iCamera* cam = GetCsCamera ();
  if (cam && cam->GetSector ())
  {
    csSectorHitBeamResult result = cam->GetSector()->HitBeamPortals (
      beam.Start (), beam.End (), true);
    if (result.mesh)
    {
      csVector3 normal = cam->GetTransform().This2OtherRelative (csVector3 (0,0,-1));
      csVector3 up = cam->GetTransform ().This2OtherRelative (csVector3 (0,1,0));
      cursorDecal = decalMgr->CreateDecal (cursorDecalTemplate,
	  /*result.mesh,*/
          cam->GetSector (),
	  result.isect, up, normal, 1.0f, 1.0f, cursorDecal);
    }
  }
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
  app->RegisterModification ();
}

void AresEdit3DView::ChangeNameSelectedObject (const char* name)
{
  if (selection->GetSize () < 1) return;
  selection->GetFirst ()->SetEntityName (name);
  modelRepository->RefreshObjectsValue ();
  app->RegisterModification ();
}

iEditorCamera* AresEdit3DView::GetEditorCamera () const
{
  return static_cast<iEditorCamera*> (camera);
}

void AresEdit3DView::SelectionChanged (const csArray<iDynamicObject*>& current_objects)
{
  modelRepository->GetObjectsValueInt ()->RefreshModel ();

  bool curveTabEnable = false;
  if (selection->GetSize () == 1)
  {
    iDynamicObject* dynobj = selection->GetFirst ();
    csString name = dynobj->GetFactory ()->GetName ();
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
  csVector2 v2d (x, y);
  csVector3 v3d = view->InvProject (v2d, maxdist);
  csVector3 start = cam->GetTransform ().GetOrigin ();
  csVector3 end = cam->GetTransform ().This2Other (v3d);
  return csSegment3 (start, end);
}

csSegment3 AresEdit3DView::GetMouseBeam (float maxdist)
{
  return GetBeam (mouseX, mouseY, maxdist);
}

iMeshWrapper* AresEdit3DView::TraceBeamHit (const csSegment3& beam, csVector3& isect)
{
  iSector* sector = GetCsCamera ()->GetSector ();
  if (!sector) return 0;

  csSectorHitBeamResult result = sector->HitBeamPortals (
      beam.Start (), beam.End (), true);
  if (result.mesh)
  {
    isect = result.isect;
    return result.mesh;
  }
  return 0;
}

iDynamicObject* AresEdit3DView::TraceBeam (const csSegment3& beam, csVector3& isect)
{
  iSector* sector = GetCsCamera ()->GetSector ();
  if (!sector) return 0;

  csVector3 isect1, isect2;
  iDynamicObject* obj1 = 0, * obj2 = 0;

  // Trace the physical beam
  if (dyncell && dyncell->GetDynamicSector ())
  {
    CS::Collisions::HitBeamResult result = dyncell->GetDynamicSector ()->
      HitBeam (beam.Start (), beam.End ());
    if (result.object)
    {
      CS::Physics::iRigidBody* hitBody =  result.object->QueryPhysicalBody ()->QueryRigidBody ();
      if (hitBody)
      {
        isect1 = result.isect;
        obj1 = dynworld->FindObject (hitBody);
      }
    }
  }

  csSectorHitBeamResult result2 = sector->HitBeamPortals (
      beam.Start (), beam.End (), true);
  if (result2.mesh)
  {
    obj2 = dynworld->FindObject (result2.mesh);
    isect2 = result2.isect;
  }

  if (!obj2) { isect = isect1; return obj1; }
  if (!obj1) { isect = isect2; return obj2; }

  // If both beams hit something but the object they hit differs then
  // it will be the physics hit that gets precendence. Otherwise we
  // will pick the hit from the mesh since that one respects backface culling.
  if (obj1 != obj2) { isect = isect1; return obj1; }
  isect = isect2;
  return obj2;
}

bool AresEdit3DView::OnMouseDown (iEvent& ev)
{
  uint but = csMouseEventHelper::GetButton (&ev);
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);

  if (paster->IsPasteSelectionActive ())
  {
    if (but == csmbLeft)
    {
      paster->PasteSelection ();
      paster->StopPasteMode ();
    }
    else if (but == csmbRight)
    {
      paster->StopPasteMode ();
    }
    else camera->OnMouseDown (ev, but, mouseX, mouseY);
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
  modelRepository->GetObjectsValueInt ()->RefreshModel ();
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
  elcm->DeleteAll ();

  //engine->DeleteAll ();
  //@@@ CLUMSY: this would be better done by removing everything loaded from an iCollection.
  engine->GetMeshes ()->RemoveAll ();
  engine->GetMeshFactories ()->RemoveAll ();
  engine->GetSectors ()->RemoveAll ();
  engine->GetMaterialList ()->RemoveAll ();
  engine->GetTextureList ()->RemoveAll ();

  pl->RemoveEntityTemplates ();
  csRef<iQuestManager> questMgr = csQueryRegistryOrLoad<iQuestManager> (object_reg,
      "cel.manager.quests");
  questMgr->RemoveQuestFactories ();

  sector = 0;
  terrainMesh = 0;

  paster->Cleanup ();

  dynSys = 0;
  if (dyn)
    dyn->DeleteCollisionSectors ();

  camera->Init (view->GetCamera (), 0, csVector3 (0, 10, 0), 0);
}

void AresEdit3DView::OnExit ()
{
}

void AresEdit3DView::AddItem (const char* category, const char* itemname)
{
  if (!categories.In (category))
    categories.Put (category, csStringArray());
  csStringArray a;
  csStringArray& items = categories.Get (category, a);
  if (items.FindSortedKey (itemname) == csArrayItemNotFound)
    items.InsertSorted (itemname);
}

void AresEdit3DView::RemoveItem (const char* category, const char* itemname)
{
  if (!categories.In (category)) return;
  csStringArray a;
  csStringArray& items = categories.Get (category, a);
  size_t idx = items.FindSortedKey (itemname);
  if (idx != csArrayItemNotFound)
    items.DeleteIndex (idx);
}

void AresEdit3DView::RemoveResources (const csSet<csPtrKey<iObject> >& resources)
{
  iAssetManager* assetMgr = app->GetAssetManager ();
  csSet<csPtrKey<iObject> >::GlobalIterator it = resources.GetIterator ();
  while (it.HasNext ())
  {
    iObject* resource = it.Next ();
    csRef<iDynamicFactory> fact = scfQueryInterface<iDynamicFactory> (resource);
    if (fact)
    {
      csString category = fact->GetAttribute ("category");
      if (!category) category = "Nodes";
      RemoveItem (category, fact->GetName ());
    }
    assetMgr->RegisterRemoval (resource);
    engine->RemoveObject (resource);
  }
  modelRepository->GetDynfactCollectionValue ()->Refresh ();
  app->UpdateTitle ();
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
  iMaterialWrapper* material = engine->GetMaterialList ()->FindByName ("ares_cursor");
  if (!material)
  {
    loader->LoadTexture ("ares_cursor", "/icons/cursor.png");
    material = engine->GetMaterialList ()->FindByName ("ares_cursor");
    if (!material)
      return app->ReportError("Can't find cursor decal material!");
  }
  cursorDecalTemplate = decalMgr->CreateDecalTemplate (material);
  cursorDecalTemplate->SetTopClipping (false);
  cursorDecalTemplate->SetBottomClipping (false, 0.0f);
  //cursorDecalTemplate->SetMainColor (csColor4 (2.0f, 0.0f, 0.0f, 1.0f));
  //cursorDecalTemplate->SetMixMode (CS_FX_ADD);
  cursorDecal = 0;
  return true;
}
#endif

void AresEdit3DView::ResizeView (int width, int height)
{
  view_width = width;
  view_height = height;

  view->SetWidth (view_width);
  view->SetHeight (view_height);
  view->SetRectangle (0, 0, view_width, view_height, false);
  view->GetPerspectiveCamera ()->SetAspectRatio
    ((float) (width) / (float) (height));
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

  selection = new Selection (this);

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

  paster.AttachNew (new Paster ());
  paster->Setup (app, this);

  colorWhite = g3d->GetDriver2D ()->FindRGB (255, 255, 255);
  font = g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_COURIER);

  // We need a View to the virtual world.
  view.AttachNew (new csView (engine, g3d));
  view->SetAutoResize (false);
  // We use the full window to draw the world.
  //view_width = (int)(g2d->GetWidth () * 0.86);
  //view_width = g2d->GetWidth ();
  //view_height = g2d->GetHeight ();
  view_width = 640;
  view_height = 480;
  view->SetWidth (view_width);
  view->SetHeight (view_height);
  view->SetRectangle (0, 0, view_width, view_height);
  view->GetPerspectiveCamera ()->SetAspectRatio
    ((float) (view_width) / (float) (view_height));

  markerMgr->SetView (view);

  if (!SetupWorld ())
    return false;

  if (!PostLoadMap ())
    return app->ReportError ("Error during PostLoadMap()!");

#if USE_DECAL
  if (!SetupDecal ())
    return false;
#endif

  SelectionListener* listener = new AresEditSelectionListener (this);
  selection->AddSelectionListener (listener);
  listener->DecRef ();

  // Start the default run/event loop.  This will return only when some code,
  // such as OnKeyboard(), has asked the run loop to terminate.
  //Run();

  modelRepository->GetObjectsValue ()->FireValueChanged ();
  modelRepository->GetTemplatesValue ()->FireValueChanged ();

  return true;
}

void AresEdit3DView::RefreshFactorySettings (iDynamicFactory* fact)
{
  csString category = fact->GetAttribute ("category");
  if (!category) category = "Nodes";
  AddItem (category, fact->GetName ());

  csBox3 bbox = fact->GetPhysicsBBox ();
  factory_to_origin_offset.PutUnique (fact->GetName (), bbox.MinY ());
  const char* st = fact->GetAttribute ("defaultstatic");
  if (st && *st == 't') static_factories.Add (fact->GetName ());
  else static_factories.Delete (fact->GetName ());
}

bool AresEdit3DView::SetupDynWorld ()
{
  // See if we have to disable physics.
  iCelEntityTemplate* worldTpl = pl->FindEntityTemplate ("World");
  bool physics = true;
  if (worldTpl)
  {
    for (size_t i = 0 ; i < worldTpl->GetPropertyClassTemplateCount () ; i++)
    {
      iCelPropertyClassTemplate* pctpl = worldTpl->GetPropertyClassTemplate (i);
      csString pcName = pctpl->GetName ();
      if (pcName == "pcworld.dynamic")
      {
	bool valid;
	physics = InspectTools::GetPropertyValueBool (pl, pctpl, "physics", &valid);
	if (!valid) physics = true;
      }
    }
  }
  dynworld->EnablePhysics (physics);

  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    if (curvedFactories.Find (fact) != csArrayItemNotFound) continue;
    if (roomFactories.Find (fact) != csArrayItemNotFound) continue;
    RefreshFactorySettings (fact);
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

    DeleteFactoryCreator (creator.name);
    curvedFactoryCreators.Push (creator);
  }
  for (size_t i = 0 ; i < roomMeshCreator->GetRoomFactoryTemplateCount () ; i++)
  {
    iRoomFactoryTemplate* cft = roomMeshCreator->GetRoomFactoryTemplate (i);
    const char* category = cft->GetAttribute ("category");
    AddItem (category, cft->GetName ());

    RoomFactoryCreator creator;
    creator.name = cft->GetName ();

    DeleteRoomFactoryCreator (creator.name);
    roomFactoryCreators.Push (creator);
  }
  return true;
}

csBox3 AresEdit3DView::ComputeTotalBox ()
{
  if (!dyncell) return csBox3 ();
  csBox3 totalbox;
  for (size_t i = 0 ; i < dyncell->GetObjectCount () ; i++)
  {
    iDynamicObject* dynobj = dyncell->GetObject (i);
    const csBox3& box = dynobj->GetFactory ()->GetBBox ();
    const csReversibleTransform& tr = dynobj->GetTransform ();
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (0)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (1)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (2)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (3)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (4)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (5)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (6)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (7)));
  }
  return totalbox;
}

iDynamicObject* AresEdit3DView::FindPlayerObject (iDynamicCell* cell)
{
  if (!dyncell) return 0;
  iDynamicFactory* playerFact = dynworld->FindFactory ("Player");
  for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
  {
    iDynamicObject* dynobj = cell->GetObject (i);
    if (dynobj->GetFactory () == playerFact)
      return dynobj;
  }
  return 0;
}

bool AresEdit3DView::PostLoadMap ()
{
  if (!dynworld->FindFactory ("Node"))
  {
    iDynamicFactory* fact = dynworld->AddLogicFactory ("Node", 1.0, -1,
	csBox3 (csVector3 (-0.1f), csVector3 (0.1f, .02f, .03f)));
    fact->AddRigidBox (csVector3 (0.0f), csVector3 (0.2f), 1.0f);
  }
  if (!dynworld->FindFactory ("Player"))
  {
    iDynamicFactory* fact = dynworld->AddFactory ("Player", 1.0, -1);
    fact->AddRigidBox (csVector3 (0.0f), csVector3 (0.2f), 1.0f);
  }

  // Make a default cell if there isn't one already.
  {
    csRef<iDynamicCellIterator> cellit = dynworld->GetCells ();
    if (!cellit->HasNext ())
    {
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
    }
  }

  if (!SetupDynWorld ())
    return false;

  if (app->GetAssetManager ())
  {
    for (size_t i = 0 ; i < curvedMeshCreator->GetCurvedFactoryCount () ; i++)
    {
      iCurvedFactory* cfact = curvedMeshCreator->GetCurvedFactory (i);
      static_factories.Add (cfact->GetName ());
    }
    curvedFactories = app->GetAssetManager ()->GetCurvedFactories ();

    for (size_t i = 0 ; i < roomMeshCreator->GetRoomFactoryCount () ; i++)
    {
      iRoomFactory* cfact = roomMeshCreator->GetRoomFactory (i);
      static_factories.Add (cfact->GetName ());
    }
    roomFactories = app->GetAssetManager ()->GetRoomFactories ();
  }

  dyncell = 0;
  // Pick the first cell containing the player or else
  // the first cell.
  csRef<iDynamicCellIterator> it = dynworld->GetCells ();
  while (it->HasNext ())
  {
    csRef<iDynamicCell> cell = it->NextCell ();
    if (!dyncell) dyncell = cell;
    if (FindPlayerObject (cell)) { dyncell = cell; break; }
  }

  if (dyncell)
  {
    dynworld->SetCurrentCell (dyncell);
    sector = dyncell->GetSector ();
    dynSys = dyncell->GetDynamicSector ();
  }
  else
  {
    dynSys = 0;
    sector = 0;
  }

  // Initialize collision objects for all loaded objects.
  csColliderHelper::InitializeCollisionWrappers (cdsys, engine);
  CS::Collisions::CollisionHelper helper;
  helper.Initialize (object_reg);
  helper.InitializeCollisionObjects (engine);

  // @@@ Bad: hardcoded terrain name! Don't do this at home!
  if (sector)
    terrainMesh = sector->GetMeshes ()->FindByName ("Terrain");
  else
    terrainMesh = 0;

#if 0
  // @@@@@@@@@@@@@@
  if (terrainMesh)
  {
    // @@@ How to prevent adding these colliders again! This should be done in
    // the map!
    csRef<iTerrainSystem> terrain =
      scfQueryInterface<iTerrainSystem> (terrainMesh->GetMeshObject ());
    if (!terrain)
      return app->ReportError("Error cannot find the terrain interface!");

    // Create a terrain collider for each cell of the terrain
    for (size_t i = 0; i < terrain->GetCellCount (); i++)
      bullet_dynSys->AttachColliderTerrain (terrain->GetCell (i));
  }
#endif

  engine->Prepare ();

  // Setup the cell.
  InitCell ();

  // Make the 'Player', 'World', and 'Node' entity templates if they don't already
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
  if (!pl->FindEntityTemplate ("Node"))
  {
    if (!LoadLibrary ("/appdata/", "node.xml"))
      return app->ReportError ("Error loading node library!");
  }

  return true;
}

void AresEdit3DView::WarpCell (iDynamicCell* cell)
{
  if (cell == dynworld->GetCurrentCell ()) return; 
  dyncell = cell;
  dynSys = dyncell->GetDynamicSector ();

  dynworld->SetCurrentCell (cell);
  sector = engine->FindSector (cell->GetName ());

  InitCell ();

  modelRepository->GetObjectsValueInt ()->BuildModel ();
}

void AresEdit3DView::InitCell ()
{
  if (camlight)
  {
    engine->RemoveObject (camlight);
    camlight = 0;
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

  // Force the update of the clock.
  nature->UpdateTime (currentTime+100, GetCsCamera ());
  nature->UpdateTime (currentTime, GetCsCamera ());

  // Put the camera at the position of the player if possible.
  iDynamicObject* player = FindPlayerObject (dyncell);
  csBox3 box = ComputeTotalBox ();
  csVector3 origin (0, 10, 0);
  if (!box.Empty ()) origin = box.GetCenter ();
  camera->Init (view->GetCamera (), sector, origin, player);
}

bool AresEdit3DView::SetupWorld ()
{
  CleanupWorld ();

  if (!LoadLibrary ("/aresnode/", "library"))
    return app->ReportError ("Error loading library!");

  dyncell = 0;
  dynSys = 0;
  sector = 0;

  ClearItems ();

  return true;
}

void AresEdit3DView::DeleteFactoryCreator (const char* name)
{
  for (size_t i = 0 ; i < curvedFactoryCreators.GetSize () ; i++)
    if (curvedFactoryCreators[i].name == name)
    {
      curvedFactoryCreators.DeleteIndex (i);
      return;
    }
}

CurvedFactoryCreator* AresEdit3DView::FindFactoryCreator (const char* name)
{
  for (size_t i = 0 ; i < curvedFactoryCreators.GetSize () ; i++)
    if (curvedFactoryCreators[i].name == name)
      return &curvedFactoryCreators[i];
  return 0;
}

void AresEdit3DView::DeleteRoomFactoryCreator (const char* name)
{
  for (size_t i = 0 ; i < roomFactoryCreators.GetSize () ; i++)
    if (roomFactoryCreators[i].name == name)
    {
      roomFactoryCreators.DeleteIndex (i);
      return;
    }
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
  csSectorHitBeamResult result = sector->HitBeamPortals (beam.Start (), beam.End (), true);

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
  paster->ConstrainTransform (tc);
  //pasteConstrainMode = CONSTRAIN_NONE;

  iDynamicObject* dynobj = dyncell->AddObject (fname, tc);
  if (!dynobj)
  {
    app->GetUIManager ()->Error ("Could not create object for '%s'!", fname.GetData ());
    return 0;
  }

  csString tplName = dynobj->GetFactory ()->GetDefaultEntityTemplate ();
  if (tplName.IsEmpty ())
    tplName = fname;
  dynobj->SetEntity (tplName == "Player" ? "Player" : 0, tplName, 0);
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

  dyn = csQueryRegistry<CS::Physics::iPhysicalSystem> (GetObjectRegistry ());
  if (!dyn) { app->ReportError ("Error loading bullet plugin!"); return 0; }

  csString systemname = "ares.dynamics.system.";
  systemname += name;

  csRef<CS::Collisions::iCollisionSector> cs = dyn->FindCollisionSector (s);
  if (!cs)
  {
    cs = dyn->CreateCollisionSector (s);
    cs->QueryObject ()->SetName (systemname);
  }
  csRef<CS::Physics::iPhysicalSector> ds = cs->QueryPhysicalSector ();

  if (!ds) { app->ReportError ("Error creating dynamic sector!"); return 0; }

  //ds->SetLinearDampener(.3f);
  ds->SetAngularDamping (.995f);
  ds->SetGravity (csVector3 (0.0f, -19.81f, 0.0f));

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

void AresEdit3DView::EntityParameters ()
{
  app->GetUIManager ()->GetEntityParameterDialog ()->Show (
      selection->GetFirst ());
}

void AresEdit3DView::UpdateObjects ()
{
  if (!app->GetUIManager ()->Ask ("Updating all objects in this cell? Are you sure?")) return;
  dynworld->UpdateObjects (dyncell);
}

void AresEdit3DView::EnablePhysics (bool e)
{
  iCelEntityTemplate* worldTpl = pl->FindEntityTemplate ("World");
  if (worldTpl)
  {
    app->RegisterModification (worldTpl->QueryObject ());
    for (size_t i = 0 ; i < worldTpl->GetPropertyClassTemplateCount () ; i++)
    {
      iCelPropertyClassTemplate* pctpl = worldTpl->GetPropertyClassTemplate (i);
      csString pcName = pctpl->GetName ();
      if (pcName == "pcworld.dynamic")
      {
	pctpl->SetProperty (pl->FetchStringID ("physics"), e);
	break;
      }
    }
  }
  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    fact->SetColliderEnabled (!e);
    app->RegisterModification (fact->QueryObject ());
  }
  dynworld->EnablePhysics (e);
}

void AresEdit3DView::RemovePlayerMovementPropertyClasses ()
{
  iCelEntityTemplate* playerTpl = pl->FindEntityTemplate ("Player");
  if (!playerTpl) return;
  app->RegisterModification (playerTpl->QueryObject ());
  csRefArray<iCelPropertyClassTemplate> toDelete;
  for (size_t i = 0 ; i < playerTpl->GetPropertyClassTemplateCount () ; i++)
  {
    iCelPropertyClassTemplate* pctpl = playerTpl->GetPropertyClassTemplate (i);
    csString pcName = pctpl->GetName ();
    csString pcTag = pctpl->GetTag ();
    if ((pcName == "pclogic.wire" && pcTag == "mousemove") || pcName == "pcmove.actor.standard"
	|| pcName == "pcmove.linear" || pcName == "pcphysics.object"
	|| pcName == "pcmove.actor.dynamic")
      toDelete.Push (pctpl);
  }
  for (size_t i = 0 ; i < toDelete.GetSize () ; i++)
    playerTpl->RemovePropertyClassTemplate (toDelete[i]);
  toDelete.DeleteAll ();
}

void AresEdit3DView::ConvertPhysics ()
{
  if (!app->GetUIManager ()->Ask ("Converting the game to physics? Are you sure?")) return;

  selection->SetCurrentObject (0);

  RemovePlayerMovementPropertyClasses ();

  iCelEntityTemplate* playerTpl = pl->FindEntityTemplate ("Player");
  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (object_reg,
      "cel.parameters.manager");
  {
    iCelPropertyClassTemplate* pctpl = playerTpl->CreatePropertyClassTemplate ();
    pctpl->SetName ("pcmove.actor.dynamic");
    pctpl->SetProperty (pl->FetchStringID ("speed"), .5f);
    pctpl->SetProperty (pl->FetchStringID ("jumpspeed"), 1.0f);
    pctpl->SetProperty (pl->FetchStringID ("rotspeed"), .5f);
  }

  EnablePhysics (true);
}

void AresEdit3DView::ConvertOpcode ()
{
  if (!app->GetUIManager ()->Ask ("Converting the game to Opcode? Are you sure?")) return;

  selection->SetCurrentObject (0);

  RemovePlayerMovementPropertyClasses ();

  iCelEntityTemplate* playerTpl = pl->FindEntityTemplate ("Player");

  csRef<iParameterManager> pm = csQueryRegistryOrLoad<iParameterManager> (object_reg,
      "cel.parameters.manager");
  {
    iCelPropertyClassTemplate* pctpl = playerTpl->CreatePropertyClassTemplate ();
    pctpl->SetName ("pclogic.wire");
    pctpl->SetTag ("mousemove");
    InspectTools::AddAction (pl, pm, pctpl, "AddInput",
	CEL_DATA_STRING, "mask", "cel.input.mouselook",
	CEL_DATA_NONE);
    InspectTools::AddAction (pl, pm, pctpl, "AddOutput",
	CEL_DATA_STRING, "msgid", "cel.move.actor.action.MouseMove",
	CEL_DATA_NONE);
    InspectTools::AddAction (pl, pm, pctpl, "MapParameter",
	CEL_DATA_LONG, "id", "0",
	CEL_DATA_STRING, "source", "x",
	CEL_DATA_STRING, "dest", "x",
	CEL_DATA_NONE);
    InspectTools::AddAction (pl, pm, pctpl, "MapParameter",
	CEL_DATA_LONG, "id", "0",
	CEL_DATA_STRING, "source", "y",
	CEL_DATA_STRING, "dest", "y",
	CEL_DATA_NONE);
  }
  {
    iCelPropertyClassTemplate* pctpl = playerTpl->CreatePropertyClassTemplate ();
    pctpl->SetName ("pcmove.linear");
    InspectTools::AddAction (pl, pm, pctpl, "InitCD",
	CEL_DATA_VECTOR3, "body", ".5,.8,.5",
	CEL_DATA_VECTOR3, "legs", ".5,.4,.5",
	CEL_DATA_VECTOR3, "offset", "0,0,0",
	CEL_DATA_NONE);
  }
  {
    iCelPropertyClassTemplate* pctpl = playerTpl->CreatePropertyClassTemplate ();
    pctpl->SetName ("pcmove.actor.standard");
    InspectTools::AddAction (pl, pm, pctpl, "SetSpeed",
	CEL_DATA_FLOAT, "movement", "5",
	CEL_DATA_FLOAT, "running", "8",
	CEL_DATA_FLOAT, "rotation", "2",
	CEL_DATA_NONE);
    InspectTools::AddAction (pl, pm, pctpl, "Subscribe", CEL_DATA_NONE);
    pctpl->SetProperty (pl->FetchStringID ("mousemove"), true);
  }

  EnablePhysics (false);
}


// =========================================================================


