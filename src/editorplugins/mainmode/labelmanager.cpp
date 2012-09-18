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

#include <crystalspace.h>
#include "labelmanager.h"
#include "imarker.h"
#include "propclass/dynworld.h"

//---------------------------------------------------------------------------

LabelManager::LabelManager (iObjectRegistry* object_reg) : object_reg (object_reg)
{
  markerMgr = csQueryRegistry<iMarkerManager> (object_reg);
  engine = csQueryRegistry<iEngine> (object_reg);
  labelRadius = 20;
  updatecounter = UPDATECOUNTER;

  entityColor = markerMgr->CreateMarkerColor ("entityColor");
  entityColor->SetRGBColor (SELECTION_NONE, 0, .6, 0, 1);
  entityColor->SetRGBColor (SELECTION_SELECTED, 0, 1, 0, 1);
  entityColor->SetRGBColor (SELECTION_ACTIVE, 0, 1, 0, 1);
  entityColor->SetPenWidth (SELECTION_NONE, 1.2f);
  entityColor->SetPenWidth (SELECTION_SELECTED, 2.0f);
  entityColor->SetPenWidth (SELECTION_ACTIVE, 2.0f);

  factoryColor = markerMgr->CreateMarkerColor ("factoryColor");
  factoryColor->SetRGBColor (SELECTION_NONE, .6, .6, 0, 1);
  factoryColor->SetRGBColor (SELECTION_SELECTED, 1, 1, 0, 1);
  factoryColor->SetRGBColor (SELECTION_ACTIVE, 1, 1, 0, 1);
  factoryColor->SetPenWidth (SELECTION_NONE, 1.2f);
  factoryColor->SetPenWidth (SELECTION_SELECTED, 2.0f);
  factoryColor->SetPenWidth (SELECTION_ACTIVE, 2.0f);
}

LabelManager::~LabelManager ()
{
  Cleanup ();
}

void LabelManager::FramePre (iPcDynamicWorld* dynworld, iCamera* camera)
{
  updatecounter--;
  if (updatecounter > 0) return;
  updatecounter = UPDATECOUNTER;

  if (!camera->GetSector ()) return;
  csHash<iMarker*, csPtrKey<iMeshWrapper> > oldMarkers = markers;
  markers.Empty ();

  csRef<iMeshWrapperIterator> it = engine->GetNearbyMeshes (camera->GetSector (),
      camera->GetTransform ().GetOrigin (), labelRadius, false);
  while (it->HasNext ())
  {
    iMeshWrapper* mesh = it->Next ();
    iDynamicObject* dynobj = dynworld->FindObject (mesh);
    if (dynobj)
    {
      if (oldMarkers.Contains (mesh))
      {
	iMarker* marker = oldMarkers.Get (mesh, 0);
	marker->SetSelectionLevel (dynobj->IsHilight () ? SELECTION_ACTIVE : SELECTION_NONE);
        markers.Put (mesh, marker);
        oldMarkers.DeleteAll (mesh);
      }
      else
      {
        iMarker* marker = markerMgr->CreateMarker ();
	marker->SetSelectionLevel (dynobj->IsHilight () ? SELECTION_ACTIVE : SELECTION_NONE);
        marker->AttachMesh (mesh);
        csStringArray t;
	iMarkerColor* color;
	if (dynobj->GetEntityName ())
	{
          t.Push (dynobj->GetEntityName ());
	  color = entityColor;
	}
	else
	{
	  t.Push (dynobj->GetFactory ()->GetName ());
	  color = factoryColor;
	}
        marker->Text (MARKER_OBJECT, csVector3 (0), t, color);
        markers.Put (mesh, marker);
      }
    }
  }
  csHash<iMarker*, csPtrKey<iMeshWrapper> >::GlobalIterator mIt = oldMarkers.GetIterator ();
  while (mIt.HasNext ())
  {
    iMarker* marker = mIt.Next ();
    markerMgr->DestroyMarker (marker);
  }
}

void LabelManager::Cleanup ()
{
  csHash<iMarker*, csPtrKey<iMeshWrapper> >::GlobalIterator mIt = markers.GetIterator ();
  while (mIt.HasNext ())
  {
    iMarker* marker = mIt.Next ();
    markerMgr->DestroyMarker (marker);
  }
  markers.Empty ();
}

//---------------------------------------------------------------------------

