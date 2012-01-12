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
}

void PlayMode::Stop ()
{
  if (!snapshot) return;
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

