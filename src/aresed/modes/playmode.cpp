/*
The MIT License

Copyright (c) 2011 by Jorrit Tyberghein

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

#include "../apparesed.h"
#include "physicallayer/pl.h"
#include "physicallayer/entity.h"
#include "physicallayer/propclas.h"
#include "propclass/camera.h"
#include "propclass/mesh.h"
#include "propclass/mechsys.h"
#include "playmode.h"

//---------------------------------------------------------------------------

DynworldSnapshot::DynworldSnapshot (iPcDynamicWorld* dynworld)
{
  csRef<iDynamicCellIterator> cellIt = dynworld->GetCells ();
  while (cellIt->HasNext ())
  {
    iDynamicCell* cell = cellIt->NextCell ();
    for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    {
      iDynamicObject* dynobj = cell->GetObject (i);
      Obj obj;
      obj.cell = dynobj->GetCell ();
      obj.fact = dynobj->GetFactory ();
      obj.isStatic = dynobj->IsStatic ();
      obj.trans = dynobj->GetTransform ();
      objects.Push (obj);
    }
  }
}

void DynworldSnapshot::Restore (iPcDynamicWorld* dynworld)
{
  csRef<iDynamicCellIterator> cellIt = dynworld->GetCells ();
  while (cellIt->HasNext ())
  {
    iDynamicCell* cell = cellIt->NextCell ();
    cell->DeleteObjects ();
  }
  for (size_t i = 0 ; i < objects.GetSize () ; i++)
  {
    Obj& obj = objects[i];
    iDynamicObject* dynobj = obj.cell->AddObject (obj.fact->GetName (), obj.trans);
    if (obj.isStatic)
      dynobj->MakeStatic ();
    else
      dynobj->MakeDynamic ();
  }
}

//---------------------------------------------------------------------------

PlayMode::PlayMode (AresEdit3DView* aresed3d)
  : EditingMode (aresed3d, "Play")
{
  snapshot = 0;
}

PlayMode::~PlayMode ()
{
  delete snapshot;
}

void PlayMode::Start ()
{
  aresed3d->GetSelection ()->SetCurrentObject (0);
  delete snapshot;
  iPcDynamicWorld* dynworld = aresed3d->GetDynamicWorld ();
  dynworld->InhibitEntities (false);
  snapshot = new DynworldSnapshot (dynworld);

  // Set entities for all dynamic objects and find the player object.
  iDynamicFactory* playerFact = dynworld->FindFactory ("Player");
  csReversibleTransform playerTrans;
  iDynamicCell* foundCell = 0;
  iDynamicObject* foundPlayerDynobj = 0;
  csRef<iDynamicCellIterator> cellIt = dynworld->GetCells ();
  while (cellIt->HasNext ())
  {
    iDynamicCell* cell = cellIt->NextCell ();
    for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    {
      iDynamicObject* dynobj = cell->GetObject (i);
      if (dynobj->GetFactory () == playerFact)
      {
	playerTrans = dynobj->GetTransform ();
	foundCell = cell;
	foundPlayerDynobj = dynobj;
	break;
      }

      dynobj->ForceEntity ();
    }
  }

  if (foundPlayerDynobj)
  {
    dynworld->SetCurrentCell (foundCell);
    foundCell->DeleteObject (foundPlayerDynobj);
  }
  else
  {
    playerTrans.SetOrigin (csVector3 (0, 3, 0));
  }

  iCelPlLayer* pl = aresed3d->GetPL ();

  iCelEntity* zoneEntity = pl->FindEntity ("Zone");
  csRef<iPcMechanicsSystem> mechsys = celQueryPropertyClassEntity<iPcMechanicsSystem> (zoneEntity);
  mechsys->SetDynamicSystem (dynworld->GetCurrentCell ()->GetDynamicSystem ());

  world = pl->CreateEntity (pl->FindEntityTemplate ("World"), "World", 0);
  player = pl->CreateEntity (pl->FindEntityTemplate ("Player"), "Player", 0);
  csRef<iPcMechanicsObject> mechPlayer = celQueryPropertyClassEntity<iPcMechanicsObject> (player);
  iRigidBody* body = mechPlayer->GetBody ();
  csRef<CS::Physics::Bullet::iRigidBody> bulletBody = scfQueryInterface<CS::Physics::Bullet::iRigidBody> (body);
  //bulletBody->MakeKinematic ();

  csRef<iPcCamera> pccamera = celQueryPropertyClassEntity<iPcCamera> (player);
  csRef<iPcMesh> pcmesh = celQueryPropertyClassEntity<iPcMesh> (player);
  // @@@ Need support for setting transform on pcmesh.
  pcmesh->MoveMesh (dynworld->GetCurrentCell ()->GetSector (), playerTrans.GetOrigin ());
  body->SetTransform (playerTrans);

  iELCM* elcm = aresed3d->GetELCM ();
  elcm->SetPlayer (player);
}

void PlayMode::Stop ()
{
  if (!snapshot) return;

  iCelPlLayer* pl = aresed3d->GetPL ();
  pl->RemoveEntity (world);
  world = 0;
  pl->RemoveEntity (player);
  player = 0;
  iELCM* elcm = aresed3d->GetELCM ();
  elcm->SetPlayer (0);

  iPcDynamicWorld* dynworld = aresed3d->GetDynamicWorld ();
  dynworld->InhibitEntities (true);
  snapshot->Restore (dynworld);
  delete snapshot;
  snapshot = 0;

  aresed3d->GetG2D ()->SetMouseCursor (csmcArrow);
}

bool PlayMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  if (code == CSKEY_ESC)
  {
    aresed3d->GetApp ()->SwitchToMainMode ();
    return true;
  }
  return false;
}

