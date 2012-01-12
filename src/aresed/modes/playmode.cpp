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
#include "playmode.h"

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
  snapshot = new DynworldSnapshot (dynworld);

  csRef<iDynamicCellIterator> cellIt = dynworld->GetCells ();
  while (cellIt->HasNext ())
  {
    iDynamicCell* cell = cellIt->NextCell ();
    for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    {
      iDynamicObject* dynobj = cell->GetObject (i);
      dynobj->SetEntity (0, 0, 0);
    }
  }

  iCelPlLayer* pl = aresed3d->GetPL ();
  world = pl->CreateEntity (pl->FindEntityTemplate ("World"), "World", 0);
  player = pl->CreateEntity (pl->FindEntityTemplate ("Player"), "Player", 0);

  csRef<iPcCamera> pccamera = celQueryPropertyClassEntity<iPcCamera> (player);
  csRef<iPcMesh> pcmesh = celQueryPropertyClassEntity<iPcMesh> (player);
  pcmesh->MoveMesh (dynworld->GetCurrentCell ()->GetSector (), csVector3 (0, 3, 0));
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
  snapshot->Restore (dynworld);
  delete snapshot;
  snapshot = 0;
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

