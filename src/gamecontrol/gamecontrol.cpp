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
#include "gamecontrol.h"
#include "physicallayer/pl.h"
#include "physicallayer/entity.h"
#include "propclass/camera.h"
#include "propclass/dynworld.h"
#include "propclass/dynmove.h"
#include "propclass/prop.h"
#include "propclass/inv.h"
#include "ivaria/dynamics.h"

//---------------------------------------------------------------------------

CEL_IMPLEMENT_FACTORY (GameController, "ares.gamecontrol")

//---------------------------------------------------------------------------

csStringID celPcGameController::id_message = csInvalidStringID;
csStringID celPcGameController::id_timeout = csInvalidStringID;
csStringID celPcGameController::id_name = csInvalidStringID;
csStringID celPcGameController::id_template = csInvalidStringID;
csStringID celPcGameController::id_factory = csInvalidStringID;

PropertyHolder celPcGameController::propinfo;

celPcGameController::celPcGameController (iObjectRegistry* object_reg)
	: scfImplementationType (this, object_reg)
{
  // For SendMessage parameters.
  if (id_message == csInvalidStringID)
  {
    id_message = pl->FetchStringID ("message");
    id_timeout = pl->FetchStringID ("timeout");
    id_name = pl->FetchStringID ("name");
    id_template = pl->FetchStringID ("template");
    id_factory = pl->FetchStringID ("factory");
  }

  propholder = &propinfo;

  // For actions.
  if (!propinfo.actions_done)
  {
    SetActionMask ("ares.controller.");
    AddAction (action_message, "Message");
    AddAction (action_startdrag, "StartDrag");
    AddAction (action_stopdrag, "StopDrag");
    AddAction (action_examine, "Examine");
    AddAction (action_pickup, "PickUp");
    AddAction (action_activate, "Activate");
    AddAction (action_spawn, "Spawn");
    AddAction (action_createentity, "CreateEntity");
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

  messageColor = g3d->GetDriver2D ()->FindRGB (255, 255, 255);
  iFontServer* fontsrv = g3d->GetDriver2D ()->GetFontServer ();
  //font = fontsrv->LoadFont (CSFONT_COURIER);
  font = fontsrv->LoadFont ("DejaVuSansBold", 10);
  font->GetMaxSize (fontW, fontH);

  classNoteID = pl->FetchStringID ("ares.note");
  classInfoID = pl->FetchStringID ("ares.info");
  classPickUpID = pl->FetchStringID ("ares.pickup");
  attrDragType = pl->FetchStringID ("ares.dragtype");

  LoadIcons ();
}

celPcGameController::~celPcGameController ()
{
  pl->RemoveCallbackEveryFrame ((iCelTimerListener*)this, CEL_EVENT_POST);
  delete iconCursor;
  delete iconEye;
  delete iconBook;
  delete iconDot;
  delete iconCheck;
}

void celPcGameController::Activate ()
{
  iRigidBody* hitBody;
  csVector3 start, isect;
  iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
  if (obj)
  {
    iCelEntity* ent = obj->GetEntity ();
    if (ent) printf ("ent=%s\n",ent->GetName ());
    if (ent && ent->HasClass (classPickUpID))
      PickUpDynObj (obj);
    else StartDrag ();
  }
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
    printf ("Error! Game controller: cannot find entity template '%s'!\n", tmpname);
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
  if (ent->IsModifiedSinceBaseline ())
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
    iCelEntityTemplate* tpl = pl->FindEntityTemplate (
	dynobj->GetFactory ()->GetName ());
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
  if (dynworld) return;

  // Not very clean. We should only depend on the dynworld plugin
  // to be in this entity but for the editor we actually have the
  // dynworld plugin in the 'Zone' entity. Need to find a way for this
  // that is cleaner.
  csRef<iCelEntity> zone = pl->FindEntity ("Zone");
  if (!zone)
  {
    printf ("Can't find entity 'Zone' and current entity has no dynworld PC!\n");
    return;
  }
  dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (zone);
  iDynamicSystem* dynSys = dynworld->GetCurrentCell ()->GetDynamicSystem ();
  bullet_dynSys = scfQueryInterface<CS::Physics::Bullet::iDynamicSystem> (dynSys);
}

void celPcGameController::TryGetCamera ()
{
  if (player && pccamera && pcdynmove) return;
  if (!player)
    player = pl->FindEntity ("Player");
  if (!player)
  {
    printf ("Can't find entity 'Player'!\n");
    return;
  }
  pccamera = celQueryPropertyClassEntity<iPcCamera> (player);
  pcdynmove = celQueryPropertyClassEntity<iPcDynamicMove> (player);
}

bool celPcGameController::PerformActionIndexed (int idx,
	iCelParameterBlock* params,
	celData& ret)
{
  switch (idx)
  {
    case action_message:
      {
        CEL_FETCH_STRING_PAR (msg,params,id_message);
        if (!p_msg) return false;
        CEL_FETCH_FLOAT_PAR (timeout,params,id_timeout);
        if (!p_timeout) timeout = 2.0f;
        Message (msg, timeout);
        return true;
      }
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
    case action_activate:
      Activate ();
      return true;
    case action_spawn:
      {
        CEL_FETCH_STRING_PAR (factory,params,id_factory);
        if (!p_factory) return false;
        Spawn (factory);
      }
      return true;
    case action_createentity:
      {
        CEL_FETCH_STRING_PAR (temp,params,id_template);
        if (!p_temp) return false;
        CEL_FETCH_STRING_PAR (name,params,id_name);
        if (!p_name) return false;
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
        Message ("ERROR: Entity has no properties!");
	return;
      }
      size_t idx = prop->GetPropertyIndex ("ares.info");
      if (idx == csArrayItemNotFound)
      {
        Message ("ERROR: Entity has no 'ares.info' property!");
	return;
      }
      Message (prop->GetPropertyString (idx));
    }
    else
    {
      Message ("I see nothing special!");
    }
  }
  else
  {
    Message ("Nothing to examine!");
  }
}

void celPcGameController::Message (const char* message, float timeout)
{
  TimedMessage m;
  m.message = message;
  m.timeleft = timeout;
  messages.Push (m);
  printf ("MSG: %s\n", message);
  fflush (stdout);
}

iDynamicObject* celPcGameController::FindCenterObject (iRigidBody*& hitBody,
    csVector3& start, csVector3& isect)
{
  TryGetCamera ();
  TryGetDynworld ();
  iCamera* cam = pccamera->GetCamera ();
  if (!cam) return 0;
  int x = mouse->GetLastX ();
  int y = mouse->GetLastY ();
  csVector2 v2d (x, g2d->GetHeight () - y);
  csVector3 v3d = cam->InvPerspective (v2d, 3.0f);
  start = cam->GetTransform ().GetOrigin ();
  csVector3 end = cam->GetTransform ().This2Other (v3d);
  // Trace the physical beam
  CS::Physics::Bullet::HitBeamResult result = bullet_dynSys->HitBeam (start, end);
  if (!result.body) return 0;
  hitBody = result.body->QueryRigidBody ();
  isect = result.isect;
  return dynworld->FindObject (hitBody);
}

bool celPcGameController::StartDrag ()
{
  iRigidBody* hitBody;
  csVector3 start, isect;
  iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
  if (obj)
  {
    dragobj = obj;
    csString dt = obj->GetFactory ()->GetAttribute (attrDragType);
    if (dt == "roty")
    {
      printf ("Start roty drag!\n"); fflush (stdout);
      dragType = DRAGTYPE_ROTY;
      pcdynmove->EnableMouselook (false);
      dragOrigin = obj->GetMesh ()->GetMovable ()->GetTransform ().GetOrigin ();
      //dragOrigin.y = isect.y;
      isect.y = dragOrigin.y;
      dragAnchor = isect;
      dragDistance = (isect - dragOrigin).Norm ();
    }
    else if (dt == "none")
    {
      // No dragging allowed.
      printf ("Inhibit drag!\n"); fflush (stdout);
      return true;
    }
    else
    {
      printf ("Start normal drag!\n"); fflush (stdout);
      dragType = DRAGTYPE_NORMAL;
      dragDistance = (isect - start).Norm ();
    }

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
    return true;
  }
  return false;
}

void celPcGameController::StopDrag ()
{
  if (!dragobj) return;
  printf ("Stop drag!\n"); fflush (stdout);
  csRef<CS::Physics::Bullet::iRigidBody> csBody =
    scfQueryInterface<CS::Physics::Bullet::iRigidBody> (dragJoint->GetAttachedBody ());
  csBody->SetLinearDampener (oldLinearDampening);
  csBody->SetRollingDampener (oldAngularDampening);
  bullet_dynSys->RemovePivotJoint (dragJoint);
  dragJoint = 0;
  dragobj = 0;
  if (dragType == DRAGTYPE_ROTY)
    pcdynmove->EnableMouselook (true);
}

void celPcGameController::TickEveryFrame ()
{
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
        dragJoint->SetPosition (newPosition);
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
      dragJoint->SetPosition (newPosition);
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
  if (messages.GetSize () > 0)
  {
    float elapsed = vc->GetElapsedSeconds ();
    int y = 20;
    size_t i = 0;
    while (i < messages.GetSize ())
    {
      TimedMessage& m = messages[i];
      m.timeleft -= elapsed;
      if (m.timeleft <= 0)
	messages.DeleteIndex (i);
      else
      {
	int alpha = 255;
	if (m.timeleft < 1.0f) alpha = int (255.0f * (m.timeleft));
	messageColor = g3d->GetDriver2D ()->FindRGB (255, 255, 255, alpha);
	g2d->Write (font, 20, y, messageColor, -1, m.message.GetData ());
	y += fontH + 2;
        i++;
      }
    }
  }

  icon->Draw (g3d, sw / 2, sh / 2);
}

//---------------------------------------------------------------------------

