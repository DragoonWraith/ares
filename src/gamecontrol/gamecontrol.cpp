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

#include "cssysdef.h"
#include "csutil/csinput.h"
#include "cstool/cspixmap.h"
#include "csgeom/math3d.h"
#include "iutil/objreg.h"
#include "iutil/virtclk.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "ivideo/fontserv.h"
#include "ivideo/txtmgr.h"
#include "iengine/engine.h"
#include "iengine/texture.h"
#include "iengine/camera.h"
#include "iengine/movable.h"
#include "iengine/sector.h"
#include "gamecontrol.h"
#include "physicallayer/pl.h"
#include "physicallayer/entity.h"
#include "physicallayer/entitytpl.h"
#include "propclass/camera.h"
#include "propclass/cameras/tracking.h"
#include "propclass/dynworld.h"
#include "propclass/dynmove.h"
#include "propclass/prop.h"
#include "propclass/inv.h"
#include "propclass/mesh.h"
#include "propclass/mechsys.h"
#include "propclass/messenger.h"
#include "ivaria/dynamics.h"

//---------------------------------------------------------------------------

CEL_IMPLEMENT_FACTORY (GameController, "ares.gamecontrol")

//-----------------------------------------------------------------------

class GameTestDefaultInfo : public scfImplementation1<GameTestDefaultInfo,iUIInventoryInfo>
{
private:
  celPcGameController* ctrl;
  iCelPlLayer* pl;
  iEngine* engine;
  csRef<iUIInventoryInfo> pInfo;

public:
  GameTestDefaultInfo (celPcGameController* ctrl, iCelPlLayer* pl,
      iEngine* engine, iUIInventoryInfo* pInfo) :
    scfImplementationType (this), ctrl (ctrl), pl (pl), engine (engine),
    pInfo( pInfo) { }
  virtual ~GameTestDefaultInfo () { }

  virtual csRef<iString> GetName (iCelEntity* entity)
  {
    if (entity->GetName ())
      return pInfo->GetName (entity);
    iMeshFactoryWrapper* fact = GetMeshFactory (entity);
    if (fact)
    {
      csRef<scfString> str;
      str.AttachNew (new scfString (fact->QueryObject ()->GetName ()));
      return str;
    }
    else
      return pInfo->GetName (entity);
  }
  virtual csRef<iString> GetName (iCelEntityTemplate* tpl, int count) { return pInfo->GetName (tpl, count); }

  virtual csRef<iString> GetDescription (iCelEntity* entity) { return pInfo->GetDescription (entity); }
  virtual csRef<iString> GetDescription (iCelEntityTemplate* tpl, int count) { return pInfo->GetDescription (tpl, count); }

  virtual iMeshFactoryWrapper* GetMeshFactory (iCelEntity* entity) { return pInfo->GetMeshFactory (entity); }
  virtual iMeshFactoryWrapper* GetMeshFactory (iCelEntityTemplate* tpl, int count)
  {
    return engine->FindMeshFactory (tpl->GetName ());
  }

  virtual iTextureHandle* GetTexture (iCelEntity* entity) { return pInfo->GetTexture (entity); }
  virtual iTextureHandle* GetTexture (iCelEntityTemplate* tpl, int count) { return pInfo->GetTexture (tpl, count); }
};

//-----------------------------------------------------------------------

class GameSelectionCallback : public scfImplementation1<GameSelectionCallback,
  iUIInventorySelectionCallback>
{
private:
  celPcGameController* game;

public:
  GameSelectionCallback (celPcGameController* game) :
    scfImplementationType (this), game (game) { }
  virtual ~GameSelectionCallback () { }

  virtual void SelectEntity (iCelEntity* entity, const char* command)
  {
    game->SelectEntity (entity, command);
  }

  virtual void SelectTemplate (iCelEntityTemplate* tpl, const char* command)
  {
    game->SelectTemplate (tpl, command);
  }
};

//---------------------------------------------------------------------------

csStringID celPcGameController::id_name = csInvalidStringID;
csStringID celPcGameController::id_template = csInvalidStringID;
csStringID celPcGameController::id_factory = csInvalidStringID;
csStringID celPcGameController::id_destination = csInvalidStringID;

PropertyHolder celPcGameController::propinfo;

celPcGameController::celPcGameController (iObjectRegistry* object_reg)
	: scfImplementationType (this, object_reg)
{
  // For SendMessage parameters.
  if (id_name == csInvalidStringID)
  {
    id_name = pl->FetchStringID ("name");
    id_template = pl->FetchStringID ("template");
    id_factory = pl->FetchStringID ("factory");
    id_destination = pl->FetchStringID ("destination");
  }

  propholder = &propinfo;

  // For actions.
  if (!propinfo.actions_done)
  {
    SetActionMask ("ares.controller.");
    AddAction (action_startdrag, "StartDrag");
    AddAction (action_stopdrag, "StopDrag");
    AddAction (action_examine, "Examine");
    AddAction (action_pickup, "PickUp");
    AddAction (action_activate, "Activate");
    AddAction (action_spawn, "Spawn");
    AddAction (action_createentity, "CreateEntity");
    AddAction (action_inventory, "Inventory");
    AddAction (action_teleport, "Teleport");
  }

  // For properties.
  propinfo.SetCount (0);
  //AddProperty (propid_counter, "counter",
	//CEL_DATA_LONG, false, "Print counter.", &counter);
  //AddProperty (propid_max, "max",
	//CEL_DATA_LONG, false, "Max length.", 0);

  dragobj = 0;

  mouse = csQueryRegistry<iMouseDriver> (object_reg);
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  g2d = g3d->GetDriver2D ();
  engine = csQueryRegistry<iEngine> (object_reg);
  pl->CallbackEveryFrame ((iCelTimerListener*)this, CEL_EVENT_POST);

  classNoteID = pl->FetchStringID ("ares.note");
  classInfoID = pl->FetchStringID ("ares.info");
  classPickUpID = pl->FetchStringID ("ares.pickup");
  classNoActivate = pl->FetchStringID ("ares.noactivate");
  attrDragType = pl->FetchStringID ("ares.dragtype");
  msgActivate = pl->FetchStringID ("ares.Activate");

  LoadIcons ();

  uiInventory = csQueryRegistryOrLoad<iUIInventory> (object_reg,
      "cel.ui.inventory.grid");
  if (!uiInventory)
  {
    printf ("Can't find UI grid inventory plugin!\n");
    return;
  }

  uiInventory->SetStyleOption ("background.image", "/appdata/textures/buttonback.png");
  uiInventory->SetStyleOption ("background.image.hi", "/appdata/textures/buttonback_hi.png");
  uiInventory->SetStyleOption ("name.font", "DejaVuSans");
  uiInventory->SetStyleOption ("name.font.size", "10");
  uiInventory->Bind ("MouseButton0", "select", 0);
  uiInventory->Bind ("i", "cancel", 0);

  csRef<GameSelectionCallback> cb;
  cb.AttachNew (new GameSelectionCallback (this));
  uiInventory->AddSelectionListener (cb);

  csRef<GameTestDefaultInfo> info;
  info.AttachNew (new GameTestDefaultInfo (this, pl, engine, uiInventory->GetInfo ()));
  uiInventory->SetInfo (info);
}

celPcGameController::~celPcGameController ()
{
  if (uiInventory)
    uiInventory->Close ();
  pl->RemoveCallbackEveryFrame ((iCelTimerListener*)this, CEL_EVENT_POST);
  delete iconCursor;
  delete iconEye;
  delete iconBook;
  delete iconDot;
  delete iconCheck;
}

void celPcGameController::SelectEntity (iCelEntity* entity, const char* command)
{
  //csString cmd = "cancel";
  //if (command && cmd == command)
    //return;
  //iPcInventory* inv = uiInventory->GetInventory ();
  //if (!inv) return;
}

void celPcGameController::SelectTemplate (iCelEntityTemplate* entity, const char* command)
{
  //csString cmd = "cancel";
  //if (command && cmd == command)
    //return;
  //iPcInventory* inv = uiInventory->GetInventory ();
  //if (!inv) return;
}

void celPcGameController::FindSiblingPropertyClasses ()
{
  if (HavePropertyClassesChanged ())
  {
    messenger = celQueryPropertyClassEntity<iPcMessenger> (entity);
  }
}

void celPcGameController::Inventory ()
{
  csRef<iPcInventory> inventory = celQueryPropertyClassEntity<iPcInventory> (
	  player);
  if (inventory->GetEntityCount () == 0 && inventory->GetEntityTemplateCount () == 0)
    messenger->Message ("error", 0, "You have nothing!", (const char*)0);
  else
    uiInventory->Open ("Inventory for player", inventory);
}

void celPcGameController::Activate ()
{
printf ("#######################################################\n");
  iRigidBody* hitBody;
  csVector3 start, isect;
  iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
  if (obj)
  {
    iCelEntity* ent = obj->GetEntity ();
    if (ent && !ent->HasClass (classNoActivate))
    {
      printf ("ent=%s\n",ent->GetName ());
      iMessageChannel* channel = ent->QueryMessageChannel ();
      channel->SendMessage (msgActivate, 0, 0);
    }
    if (ent && ent->HasClass (classPickUpID))
      PickUpDynObj (obj);
    else StartDrag ();
  }
}

void celPcGameController::Teleport (const char* entityname)
{
  printf ("Teleport to '%s'\n", entityname); fflush (stdout);
  iDynamicObject* dynobj = dynworld->FindObject (entityname);
  if (!dynobj)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR, "ares.gamecontroller",
	"Teleport: Cannot find dynamic object for entity '%s'!\n", entityname);
    return;
  }

  iDynamicCell* cell = dynobj->GetCell ();
  csReversibleTransform trans = dynobj->GetTransform ();

  if (cell != dynworld->GetCurrentCell ())
    dynworld->SetCurrentCell (cell);

  csRef<iPcMesh> pcmesh = celQueryPropertyClassEntity<iPcMesh> (player);
  csRef<iPcMechanicsObject> mechPlayer = celQueryPropertyClassEntity<iPcMechanicsObject> (player);
  iRigidBody* body = 0;
  if (mechPlayer)
    body = mechPlayer->GetBody ();
  pcmesh->MoveMesh (cell->GetSector (), trans.GetOrigin ());
  if (body)
  {
    trans.RotateThis (csVector3 (0, 1, 0), M_PI);
    body->SetTransform (trans);
  }
  csRef<iPcTrackingCamera> trackcam = celQueryPropertyClassEntity<iPcTrackingCamera> (player);
  if (trackcam)
  {
    trackcam->ResetCamera ();
  }

  iCamera* cam = pccamera->GetCamera ();
  dynworld->ForceView (cam);
}

void celPcGameController::Spawn (const char* factname)
{
  TryGetCamera ();
  TryGetDynworld ();
  iCamera* cam = pccamera->GetCamera ();
  if (!cam) return;
  int x = mouse->GetLastX ();
  int y = mouse->GetLastY ();
  csVector2 v2d (x, g2d->GetHeight () - y);
  csVector3 v3d = cam->InvPerspective (v2d, 0.5f);
  csVector3 end = cam->GetTransform ().This2Other (v3d);
  csReversibleTransform trans = cam->GetTransform ();
  trans.SetOrigin (end);
  dynworld->GetCurrentCell ()->AddObject (factname, trans);
}

void celPcGameController::CreateEntity (const char* tmpname, const char* name)
{
  iCelEntityTemplate* temp = pl->FindEntityTemplate (tmpname);
  if (!temp)
  {
    csReport (object_reg, CS_REPORTER_SEVERITY_ERROR, "ares.gamecontroller",
	"CreateEntity: Cannot find entity template '%s'!\n", tmpname);
    return;
  }
  pl->CreateEntity (temp, name);
}

void celPcGameController::PickUp ()
{
  iRigidBody* hitBody;
  csVector3 start, isect;
  iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
  if (obj)
  {
    iCelEntity* ent = obj->GetEntity ();
    if (ent && ent->HasClass (classPickUpID))
      PickUpDynObj (obj);
  }
}

void celPcGameController::PickUpDynObj (iDynamicObject* dynobj)
{
  TryGetCamera ();
  iCelEntity* ent = dynobj->GetEntity ();
  csRef<iPcInventory> inventory = celQueryPropertyClassEntity<iPcInventory> (
	  player);

  bool hasState = false;
  if (ent->IsModifiedSinceBaseline ())
    hasState = true;
  else if (dynobj->GetEntityTemplate ())
  {
    csString tplName = dynobj->GetFactory ()->GetName ();
    if (tplName != dynobj->GetEntityTemplate ()->GetName ())
    {
      // This entity uses another template then the standard template. So we
      // have to see this entity as having state as well.
      hasState = true;
    }
  }


  if (hasState)
  {
    // This entity has state. We cannot convert it to a template in the
    // inventory so we have to add the actual entity.
    inventory->AddEntity (ent);
    dynworld->ForceInvisible (dynobj);
    dynobj->UnlinkEntity ();
    dynworld->GetCurrentCell ()->DeleteObject (dynobj);
  }
  else
  {
    iCelEntityTemplate* tpl = dynobj->GetEntityTemplate ();
    if (tpl)
    {
      inventory->AddEntityTemplate (tpl, 1);
      dynworld->GetCurrentCell ()->DeleteObject (dynobj);
    }
  }
}

void celPcGameController::LoadIcons ()
{
  iTextureWrapper* txt;

  txt = engine->CreateTexture ("icon_cursor",
      "/icons/iconic_cursor_32x32.png", 0, CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS);
  txt->Register (g3d->GetTextureManager ());
  iconCursor = new csSimplePixmap (txt->GetTextureHandle ());

  txt = engine->CreateTexture ("icon_dot",
      "/icons/icon_dot.png", 0, CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS);
  txt->Register (g3d->GetTextureManager ());
  iconDot = new csSimplePixmap (txt->GetTextureHandle ());

  txt = engine->CreateTexture ("icon_eye",
      "/icons/iconic_eye_32x24.png", 0, CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS);
  txt->Register (g3d->GetTextureManager ());
  iconEye = new csSimplePixmap (txt->GetTextureHandle ());

  txt = engine->CreateTexture ("icon_book",
      "/icons/iconic_book_alt2_32x28.png", 0, CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS);
  txt->Register (g3d->GetTextureManager ());
  iconBook = new csSimplePixmap (txt->GetTextureHandle ());

  txt = engine->CreateTexture ("icon_check",
      "/icons/iconic_check_32x26.png", 0, CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS);
  txt->Register (g3d->GetTextureManager ());
  iconCheck = new csSimplePixmap (txt->GetTextureHandle ());
}

void celPcGameController::TryGetDynworld ()
{
  if (dynworld) return;
  dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (entity);
  if (!dynworld)
  {
    csRef<iCelEntity> world = pl->FindEntity ("World");
    if (!world)
    {
      printf ("Can't find entity 'World' and current entity has no dynworld PC!\n");
      fflush (stdout);
      return;
    }
    dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (world);
  }
  if (dynworld->IsPhysicsEnabled ())
  {
    iDynamicSystem* dynSys = dynworld->GetCurrentCell ()->GetDynamicSystem ();
    bullet_dynSys = scfQueryInterface<CS::Physics::Bullet::iDynamicSystem> (dynSys);
  }
  else
  {
    bullet_dynSys = 0;
  }
}

bool celPcGameController::FindPlayer ()
{
  if (player) return true;
  player = pl->FindEntity ("Player");
  if (!player)
  {
    printf ("Can't find entity 'Player'!\n");
    fflush (stdout);
    return false;
  }
  return true;
}

void celPcGameController::TryGetCamera ()
{
  if (player && pccamera) return;
  if (!FindPlayer ()) return;
  pccamera = celQueryPropertyClassEntity<iPcCamera> (player);
}

void celPcGameController::TryGetDynmove ()
{
  if (player && pcdynmove) return;
  if (!FindPlayer ()) return;
  pcdynmove = celQueryPropertyClassEntity<iPcDynamicMove> (player);
}

bool celPcGameController::PerformActionIndexed (int idx,
	iCelParameterBlock* params,
	celData& ret)
{
  switch (idx)
  {
    case action_startdrag:
      return StartDrag ();
    case action_stopdrag:
      StopDrag ();
      return true;
    case action_examine:
      Examine ();
      return true;
    case action_pickup:
      PickUp ();
      return true;
    case action_inventory:
      Inventory ();
      return true;
    case action_teleport:
      {
	csString destination;
	if (!Fetch (destination, params, id_destination)) return false;
        Teleport (destination);
      }
      return true;
    case action_activate:
      Activate ();
      return true;
    case action_spawn:
      {
	csString factory;
	if (!Fetch (factory, params, id_factory)) return false;
        Spawn (factory);
      }
      return true;
    case action_createentity:
      {
	csString temp, name;
	if (!Fetch (temp, params, id_template)) return false;
	if (!Fetch (name, params, id_name)) return false;
        CreateEntity (temp, name);
      }
      return true;
    default:
      return false;
  }
  return false;
}

void celPcGameController::Examine ()
{
  FindSiblingPropertyClasses ();
  iRigidBody* hitBody;
  csVector3 start, isect;
  iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
  if (obj)
  {
    iCelEntity* ent = obj->GetEntity ();
    if (ent && ent->HasClass (classInfoID))
    {
      csRef<iPcProperties> prop = celQueryPropertyClassEntity<iPcProperties> (ent);
      if (!prop)
      {
        messenger->Message ("error", 0, "Entity has no properties!",
	    (const char*)0);
	return;
      }
      size_t idx = prop->GetPropertyIndex ("ares.info");
      if (idx == csArrayItemNotFound)
      {
        messenger->Message ("error", 0,
	    "Entity has no 'ares.info' property!", (const char*)0);
	return;
      }
      messenger->Message ("std", 0, prop->GetPropertyString (idx),
	  (const char*)0);
    }
    else
    {
      messenger->Message ("std", 0, "I see nothing special!", (const char*)0);
    }
  }
  else
  {
    messenger->Message ("error", 0, "Nothing to examine!", (const char*)0);
  }
}

iDynamicObject* celPcGameController::FindCenterObject (iRigidBody*& hitBody,
    csVector3& start, csVector3& isect)
{
  TryGetCamera ();
  TryGetDynworld ();
  iCamera* cam = pccamera->GetCamera ();
  if (!cam) return 0;
  start = cam->GetTransform ().GetOrigin ();
  csVector3 end = cam->GetTransform ().This2Other (csVector3 (0.0f, 0.0f, 3.0f));
  //printf ("end=%s\n", end.Description().GetData());
  // Trace the physical beam
  if (bullet_dynSys)
  {
    CS::Physics::Bullet::HitBeamResult result = bullet_dynSys->HitBeam (start, end);
    if (!result.body) return 0;
    hitBody = result.body->QueryRigidBody ();
    isect = result.isect;
    if (!hitBody) return 0;
    return dynworld->FindObject (hitBody);
  }
  else if (cam->GetSector ())
  {
    csSectorHitBeamResult result = cam->GetSector ()->HitBeamPortals (start, end);
    if (!result.mesh) return 0;
    isect = result.isect;
    return dynworld->FindObject (result.mesh);
  }
  else return 0;
}

bool celPcGameController::StartDrag ()
{
  iRigidBody* hitBody;
  csVector3 start, isect;
  iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
  if (obj)
  {
    if (obj->IsStatic ())
      return true;	// No dragging.
    if (obj->GetEntity ())
    {
      csRef<iPcProperties> prop = celQueryPropertyClassEntity<iPcProperties> (obj->GetEntity ());
      if (prop)
      {
        size_t idx = prop->GetPropertyIndex ("ares.inhibitdrag");
        if (idx != csArrayItemNotFound && prop->GetPropertyBool (idx))
	  return true;	// Don't allow dragging.
      }
    }

    dragobj = obj;

    csString dt = obj->GetFactory ()->GetAttribute (attrDragType);
    dragOrigin = obj->GetMesh ()->GetMovable ()->GetTransform ().GetOrigin ();
    dragAnchor = isect;
    if (dt == "roty")
    {
      printf ("Start roty drag!\n"); fflush (stdout);
      dragType = DRAGTYPE_ROTY;
      TryGetDynmove ();
      pcdynmove->EnableMouselook (false);
      isect.y = dragOrigin.y;
      dragAnchor.y = dragOrigin.y;
      dragDistance = (isect - dragOrigin).Norm ();
    }
    else if (dt == "none")
    {
      // No dragging allowed.
      printf ("Inhibit drag!\n"); fflush (stdout);
      dragobj = 0;
      return true;
    }
    else
    {
      printf ("Start normal drag!\n"); fflush (stdout);
      dragType = DRAGTYPE_NORMAL;
      dragDistance = (isect - start).Norm ();
    }

    if (bullet_dynSys)
    {
      dragJoint = bullet_dynSys->CreatePivotJoint ();
      dragJoint->SetParameters (1.0f, 0.001f, 1.0f);
      dragJoint->Attach (hitBody, isect);

      // Set some dampening on the rigid body to have a more stable dragging
      csRef<CS::Physics::Bullet::iRigidBody> csBody =
            scfQueryInterface<CS::Physics::Bullet::iRigidBody> (hitBody);
      oldLinearDampening = csBody->GetLinearDampener ();
      oldAngularDampening = csBody->GetRollingDampener ();
      csBody->SetLinearDampener (0.9f);
      csBody->SetRollingDampener (0.9f);
    }
    else
    {
      // @@@ TODO: implement dragging for opcode based world?
    }
    return true;
  }
  return false;
}

void celPcGameController::StopDrag ()
{
  if (!dragobj) return;
  printf ("Stop drag!\n"); fflush (stdout);
  if (bullet_dynSys)
  {
    csRef<CS::Physics::Bullet::iRigidBody> csBody =
      scfQueryInterface<CS::Physics::Bullet::iRigidBody> (dragJoint->GetAttachedBody ());
    csBody->SetLinearDampener (oldLinearDampening);
    csBody->SetRollingDampener (oldAngularDampening);
    bullet_dynSys->RemovePivotJoint (dragJoint);
    dragJoint = 0;
  }
  else
  {
    // @@@ TODO: implement dragging for opcode based world?
  }
  dragobj = 0;
  if (dragType == DRAGTYPE_ROTY)
  {
    TryGetDynmove ();
    pcdynmove->EnableMouselook (true);
  }
}

void celPcGameController::TickEveryFrame ()
{
  FindSiblingPropertyClasses ();

  csSimplePixmap* icon = iconDot;

  int sw = g2d->GetWidth ();
  int sh = g2d->GetHeight ();
  if (dragobj)
  {
    iCamera* cam = pccamera->GetCamera ();
    if (!cam) return;
    int x = mouse->GetLastX ();
    int y = mouse->GetLastY ();
    csVector3 newPosition;
    if (dragType == DRAGTYPE_ROTY)
    {
      int sx = x - sw / 2;
      int sy = y - sh / 2;
      g2d->SetMousePosition (sw / 2, sh / 2);
      csVector3 v (float (sx) / 200.0f, 0, - float (sy) / 200.0f);
      float len = v.Norm ();
      v = cam->GetTransform ().This2OtherRelative (v);
      v.y = 0;
      if (v.Norm () > .0001f)
      {
	v.Normalize ();
	v *= len;
        dragAnchor += v;
        newPosition = dragAnchor - dragOrigin;
        newPosition.Normalize ();
        newPosition = dragOrigin + newPosition * dragDistance;
	if (dragJoint)
          dragJoint->SetPosition (newPosition);
	else
	{
	  csReversibleTransform tr = dragobj->GetTransform ();
	  tr.SetOrigin (newPosition - (dragAnchor-dragOrigin));
	  dragobj->SetTransform (tr);
	}
      }
      icon = iconDot;
    }
    else
    {
      csVector2 v2d (x, sh - y);
      csVector3 v3d = cam->InvPerspective (v2d, 3.0f);
      csVector3 start = cam->GetTransform ().GetOrigin ();
      csVector3 end = cam->GetTransform ().This2Other (v3d);
      newPosition = end - start;
      newPosition.Normalize ();
      newPosition = cam->GetTransform ().GetOrigin () + newPosition * dragDistance;
      if (dragJoint)
        dragJoint->SetPosition (newPosition);
      else
      {
	csReversibleTransform tr = dragobj->GetTransform ();
	tr.SetOrigin (newPosition - (dragAnchor-dragOrigin));
	dragobj->SetTransform (tr);
      }
      icon = iconCursor;
    }
  }
  else
  {
    iRigidBody* hitBody;
    csVector3 start, isect;
    iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
    if (obj)
    {
      iCelEntity* ent = obj->GetEntity ();
      if (ent && ent->HasClass (classNoteID))
        icon = iconBook;
      else if (ent && ent->HasClass (classInfoID))
        icon = iconEye;
      else if (ent && ent->HasClass (classPickUpID))
	icon = iconCheck;
      else if (!obj->IsStatic ())
        icon = iconCursor;
    }
  }

  g3d->BeginDraw (CSDRAW_2DGRAPHICS);

  icon->Draw (g3d, sw / 2, sh / 2);
}

//---------------------------------------------------------------------------

