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
#include "paster.h"

// =========================================================================

Paster::Paster () : scfImplementationType (this)
{
  pasteMarker = 0;
  constrainMarker = 0;
  pasteConstrainMode = CONSTRAIN_NONE;
  gridMode = false;
  gridSize = 0.1;
}

Paster::~Paster()
{
}

void Paster::StopPasteMode ()
{
  todoSpawn.Empty ();
  pasteMarker->SetVisible (false);
  constrainMarker->SetVisible (false);
  app->SetMenuState ();
  app->ClearStatus ();
}

void Paster::CopySelection ()
{
  pastebuffer.Empty ();
  csRef<iSelectionIterator> it = view3d->GetSelection ()->GetIterator ();
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
  app->SetFocus3D ();
}

void Paster::PasteSelection ()
{
  if (todoSpawn.GetSize () <= 0) return;
  app->RegisterModification ();
  csReversibleTransform trans = todoSpawn[0].trans;
  csArray<iDynamicObject*> newobjects;
  for (size_t i = 0 ; i < todoSpawn.GetSize () ; i++)
  {
    csReversibleTransform tr = todoSpawn[i].trans;
    csReversibleTransform* transPtr = 0;
    if (todoSpawn[i].useTransform)
    {
      tr.SetOrigin (tr.GetOrigin () - trans.GetOrigin ());
      transPtr = &tr;
    }
    iDynamicObject* dynobj = view3d->SpawnItem (todoSpawn[i].dynfactName, transPtr);
    if (todoSpawn[i].useTransform)
    {
      if (todoSpawn[i].isStatic)
        dynobj->MakeStatic ();
      else
        dynobj->MakeDynamic ();
    }
    newobjects.Push (dynobj);
  }

  view3d->GetSelection ()->SetCurrentObject (0);
  for (size_t i = 0 ; i < newobjects.GetSize () ; i++)
    view3d->GetSelection ()->AddCurrentObject (newobjects[i]);
}

void Paster::CreatePasteMarker ()
{
  if (currentPasteMarkerContext != todoSpawn[0].dynfactName)
  {
    iMarkerColor* white = view3d->GetMarkerManager ()->FindMarkerColor ("white");

    // We need to recreate the mesh in the paste marker.
    pasteMarker->Clear ();
    currentPasteMarkerContext = todoSpawn[0].dynfactName;
    bool error = true;
    iMeshFactoryWrapper* factory = view3d->GetEngine ()->FindMeshFactory (currentPasteMarkerContext);
    if (factory)
    {
      iMeshObjectFactory* fact = factory->GetMeshObjectFactory ();
      iObjectModel* model = fact->GetObjectModel ();
      if (model)
      {
 	csRef<iStringSet> strings = csQueryRegistryTagInterface<iStringSet> (
 	   app->GetObjectRegistry (), "crystalspace.shared.stringset");
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

void Paster::ToggleGridMode ()
{
  gridMode = !gridMode;
}


void Paster::ConstrainTransform (csReversibleTransform& tr)
{
  csVector3 origin = tr.GetOrigin ();
  if (gridMode)
  {
    float m;
    m = fmod (origin.x, gridSize);
    origin.x -= m;
    m = fmod (origin.y, gridSize);
    origin.y -= m;
    m = fmod (origin.z, gridSize);
    origin.z -= m;
  }
  switch (pasteConstrainMode)
  {
    case CONSTRAIN_NONE:
      break;
    case CONSTRAIN_XPLANE:
      origin.y = pasteConstrain.y;
      origin.z = pasteConstrain.z;
      break;
    case CONSTRAIN_YPLANE:
      origin.x = pasteConstrain.x;
      origin.z = pasteConstrain.z;
      break;
    case CONSTRAIN_ZPLANE:
      origin.x = pasteConstrain.x;
      origin.y = pasteConstrain.y;
      break;
  }
  tr.SetOrigin (origin);
}

void Paster::PlacePasteMarker ()
{
  pasteMarker->SetVisible (true);
  constrainMarker->SetVisible (true);
  CreatePasteMarker ();

  csReversibleTransform tr = GetSpawnTransformation ();
  ConstrainTransform (tr);
  pasteMarker->SetTransform (tr);
  csReversibleTransform ctr;
  ctr.SetOrigin (tr.GetOrigin ());
  constrainMarker->SetTransform (ctr);
}

void Paster::StartPasteSelection ()
{
  if (!view3d->GetSector () || !view3d->GetDynamicCell ())
  {
    app->ReportError ("Can't paste when no cell is active!");
    return;
  }
  pasteConstrainMode = CONSTRAIN_NONE;
  ShowConstrainMarker (false, true, false);
  todoSpawn = pastebuffer;
  if (IsPasteSelectionActive ())
    PlacePasteMarker ();
  app->SetMenuState ();
  app->SetStatus ("Left mouse to place objects. Right button to cancel. x/z to constrain placement. # for grid");
  app->SetFocus3D ();
}

void Paster::StartPasteSelection (const char* name)
{
  if (!view3d->GetSector () || !view3d->GetDynamicCell ())
  {
    app->ReportError ("Can't paste when no cell is active!");
    return;
  }
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
  app->SetFocus3D ();
}

void Paster::Cleanup ()
{
  pastebuffer.DeleteAll ();
  todoSpawn.DeleteAll ();
}

bool Paster::Setup (AppAresEditWX* app, AresEdit3DView* view3d)
{
  Paster::app = app;
  Paster::view3d = view3d;

  iMarkerManager* markerMgr = view3d->GetMarkerManager ();
  iMarkerColor* white = markerMgr->FindMarkerColor ("white");

  pasteMarker = markerMgr->CreateMarker ();
  pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (1,0,0), white, true);
  pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,1,0), white, true);
  pasteMarker->Line (MARKER_OBJECT, csVector3 (0), csVector3 (0,0,1), white, true);
  pasteMarker->SetVisible (false);

  constrainMarker = markerMgr->CreateMarker ();
  HideConstrainMarker ();

  return true;
}

csReversibleTransform Paster::GetSpawnTransformation ()
{
  csReversibleTransform tr = todoSpawn[0].trans;
  if (!todoSpawn[0].useTransform)
  {
    tr = view3d->GetCsCamera ()->GetTransform ();
    csVector3 front = tr.GetFront ();
    front.y = 0;
    tr.LookAt (front, csVector3 (0, 1, 0));
  }
  tr.SetOrigin (csVector3 (0));

  const char* name = todoSpawn[0].dynfactName;
  csVector3 newPosition = view3d->GetBeamPosition (name);

  csReversibleTransform tc = view3d->GetCsCamera ()->GetTransform ();
  csVector3 front = tc.GetFront ();
  front.y = 0;
  tc.LookAt (front, csVector3 (0, 1, 0));
  tc = tr;
  tc.SetOrigin (tc.GetOrigin () + newPosition);
  return tc;
}

void Paster::ShowConstrainMarker (bool constrainx, bool constrainy, bool constrainz)
{
  constrainMarker->SetVisible (true);
  constrainMarker->Clear ();
  iMarkerManager* markerMgr = view3d->GetMarkerManager ();
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

void Paster::MoveConstrainMarker (const csReversibleTransform& trans)
{
  constrainMarker->SetTransform (trans);
}

void Paster::HideConstrainMarker ()
{
  constrainMarker->SetVisible (false);
}

void Paster::SetPasteConstrain (int mode)
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

// =========================================================================


