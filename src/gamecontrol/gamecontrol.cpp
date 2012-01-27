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
#include "ivaria/dynamics.h"

//---------------------------------------------------------------------------

CEL_IMPLEMENT_FACTORY (GameController, "ares.gamecontrol")

//---------------------------------------------------------------------------

csStringID celPcGameController::id_message = csInvalidStringID;
csStringID celPcGameController::id_timeout = csInvalidStringID;

PropertyHolder celPcGameController::propinfo;

celPcGameController::celPcGameController (iObjectRegistry* object_reg)
	: scfImplementationType (this, object_reg)
{
  // For SendMessage parameters.
  if (id_message == csInvalidStringID)
  {
    id_message = pl->FetchStringID ("message");
    id_timeout = pl->FetchStringID ("timeout");
  }

  propholder = &propinfo;

  // For actions.
  if (!propinfo.actions_done)
  {
    SetActionMask ("ares.controller.");
    AddAction (action_message, "Message");
    AddAction (action_startdrag, "StartDrag");
    AddAction (action_stopdrag, "StopDrag");
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
  font = g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_COURIER);
  font->GetMaxSize (fontW, fontH);

  LoadIcons ();
}

celPcGameController::~celPcGameController ()
{
  pl->RemoveCallbackEveryFrame ((iCelTimerListener*)this, CEL_EVENT_POST);
  delete iconCursor;
  delete iconDot;
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
  if (pccamera) return;
  csRef<iCelEntity> player = pl->FindEntity ("Player");
  if (!player)
  {
    printf ("Can't find entity 'Player'!\n");
    return;
  }
  pccamera = celQueryPropertyClassEntity<iPcCamera> (player);
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
    default:
      return false;
  }
  return false;
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
    dragDistance = (isect - start).Norm ();
    dragobj->MakeKinematic ();
    iMeshWrapper* mesh = dragobj->GetMesh ();
    csVector3 meshpos = mesh->GetMovable ()->GetTransform ().GetOrigin ();
    dragOffset = isect - meshpos;
    printf ("Hit something!\n"); fflush (stdout);
    return true;
  }
  return false;
}

void celPcGameController::StopDrag ()
{
  if (!dragobj) return;
  printf ("Stop drag!\n"); fflush (stdout);
  dragobj->UndoKinematic ();
  dragobj = 0;
}

void celPcGameController::TickEveryFrame ()
{
  csSimplePixmap* icon = iconDot;
  int iconW = 8, iconH = 8;

  int sw = g2d->GetWidth ();
  int sh = g2d->GetHeight ();
  if (dragobj)
  {
    iCamera* cam = pccamera->GetCamera ();
    if (!cam) return;
    int x = mouse->GetLastX ();
    int y = mouse->GetLastY ();
    csVector2 v2d (x, sh - y);
    csVector3 v3d = cam->InvPerspective (v2d, 3.0f);
    csVector3 start = cam->GetTransform ().GetOrigin ();
    csVector3 end = cam->GetTransform ().This2Other (v3d);
    csVector3 newPosition = end - start;
    newPosition.Normalize ();
    newPosition = cam->GetTransform ().GetOrigin () + newPosition * dragDistance;
    iMeshWrapper* mesh = dragobj->GetMesh ();
    if (mesh)
    {
      iMovable* mov = mesh->GetMovable ();
      csVector3 np = newPosition - dragOffset;
      mov->GetTransform ().SetOrigin (np);
      mov->UpdateMove ();
    }
    icon = iconCursor;
    iconW = 16; iconH = 16;
  }
  else
  {
    iRigidBody* hitBody;
    csVector3 start, isect;
    iDynamicObject* obj = FindCenterObject (hitBody, start, isect);
    if (obj)
    {
      if (!obj->IsStatic ())
      {
        icon = iconCursor;
        iconW = 16; iconH = 16;
      }
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
	g2d->Write (font, 20, y, messageColor, -1, m.message.GetData ());
	y += fontH + 2;
        i++;
      }
    }
  }
  icon->DrawScaled (g3d, sw / 2, sh / 2, iconW, iconH);
}

//---------------------------------------------------------------------------

