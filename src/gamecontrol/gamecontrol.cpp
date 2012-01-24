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
#include "iutil/objreg.h"
#include "ivideo/graph3d.h"
#include "ivideo/graph2d.h"
#include "iengine/camera.h"
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

  mouse = csQueryRegistry<iMouseDriver> (object_reg);
  csRef<iGraphics3D> g3d = csQueryRegistry<iGraphics3D> (object_reg);
  g2d = g3d->GetDriver2D ();
}

celPcGameController::~celPcGameController ()
{
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
  printf ("MSG: %s\n", message);
  fflush (stdout);
}

bool celPcGameController::StartDrag ()
{
  TryGetCamera ();
  TryGetDynworld ();
  iCamera* cam = pccamera->GetCamera ();
  if (!cam) return false;
  int x = mouse->GetLastX ();
  int y = mouse->GetLastY ();
  csVector2 v2d (x, g2d->GetHeight () - y);
  csVector3 v3d = cam->InvPerspective (v2d, 3.0f);
  csVector3 start = cam->GetTransform ().GetOrigin ();
  csVector3 end = cam->GetTransform ().This2Other (v3d);
  // Trace the physical beam
  iRigidBody* hitBody = 0;
  CS::Physics::Bullet::HitBeamResult result = bullet_dynSys->HitBeam (start, end);
  if (result.body)
  {
    hitBody = result.body->QueryRigidBody ();
    csVector3 isect = result.isect;
    printf ("Hit something!\n"); fflush (stdout);
    return true;
  }
  return false;
}

void celPcGameController::StopDrag ()
{
}

#if 0
void celPcGameController::Print (const char* msg)
{
  printf ("Print: %s\n", msg);
  fflush (stdout);
  params->GetParameter (0).Set (msg);
  iCelBehaviour* ble = entity->GetBehaviour ();
  if (ble)
  {
    celData ret;
    ble->SendMessage ("pcmisc.test_print", this, ret, params);
  }

  if (!dispatcher_print)
  {
    dispatcher_print = entity->QueryMessageChannel ()->
      CreateMessageDispatcher (this, pl->FetchStringID ("cel.test.print"));
    if (!dispatcher_print) return;
  }
  dispatcher_print->SendMessage (params);

  counter++;
  size_t l = strlen (msg);
  if (l > max) max = l;
}
#endif

//---------------------------------------------------------------------------

