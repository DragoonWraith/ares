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

#include <crystalspace.h>
#include "edcommon/editmodes.h"
#include "editor/iapp.h"

//---------------------------------------------------------------------------

void MarkerCallback::StartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos, uint button, uint32 modifiers)
{
  editmode->MarkerStartDragging (marker, area, pos, button, modifiers);
}

void MarkerCallback::MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
{
  editmode->MarkerWantsMove (marker, area, pos);
}

void MarkerCallback::MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform)
{
  editmode->MarkerWantsRotate (marker, area, transform);
}

void MarkerCallback::StopDragging (iMarker* marker, iMarkerHitArea* area)
{
  editmode->MarkerStopDragging (marker, area);
}

//---------------------------------------------------------------------------

EditingMode::EditingMode (i3DView* view, iObjectRegistry* object_reg,
    const char* name) :
  scfImplementationType (this), object_reg (object_reg), view3d (view), name (name)
{
  Initialize (object_reg);
}

EditingMode::EditingMode (iBase* parent) : scfImplementationType (this, parent)
{
}

bool EditingMode::Initialize (iObjectRegistry* object_reg)
{
  EditingMode::object_reg = object_reg;
  markerMgr = csQueryRegistry<iMarkerManager> (object_reg);
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  kbd = csQueryRegistry<iKeyboardDriver> (object_reg);
  return true;
}

void EditingMode::SetApplication (iAresEditor* app)
{
  EditingMode::app = app;
  view3d = app->Get3DView ();
}

//---------------------------------------------------------------------------

