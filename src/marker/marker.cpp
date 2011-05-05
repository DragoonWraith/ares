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
#include "cssysdef.h"

#include "marker.h"

#include "iutil/objreg.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "iengine/sector.h"
#include "iengine/camera.h"
#include "iengine/movable.h"
#include "iengine/mesh.h"
#include "cstool/pen.h"


CS_PLUGIN_NAMESPACE_BEGIN(MarkerManager)
{

//---------------------------------------------------------------------------------------

csPen* MarkerColor::GetOrCreatePen (int level)
{
  csPen* pen;
  if (size_t (level) >= pens.GetSize () || pens[level] == 0)
  {
    pen = new csPen (mgr->g2d, mgr->g3d);
    pens.Put (level, pen);
  }
  else
  {
    pen = pens[level];
  }
  return pen;
}

void MarkerColor::SetRGBColor (int selectionLevel, float r, float g, float b, float a)
{
  csPen* pen = GetOrCreatePen (selectionLevel);
  pen->SetColor (r, g, b, a);
}

void MarkerColor::SetPenWidth (int selectionLevel, float width)
{
  csPen* pen = GetOrCreatePen (selectionLevel);
  pen->SetPenWidth (width);
}

//---------------------------------------------------------------------------------------

const csReversibleTransform& Marker::GetTransform () const
{
  if (attachedMesh) return attachedMesh->GetMovable ()->GetTransform ();
  else return trans;
}

static csVector3 TransPoint (
    const csOrthoTransform& camtrans,
    const csReversibleTransform& meshtrans,
    MarkerSpace space,
    const csVector3& point)
{
  if (space == MARKER_CAMERA)
    return point;
  else if (space == MARKER_OBJECT)
    return camtrans.Other2This (meshtrans.This2Other (point));
  else if (space == MARKER_WORLD)
    return camtrans.Other2This (meshtrans.This2Other (point));
  else if (space == MARKER_CAMERA_AT_MESH)
  {
    csReversibleTransform tr;
    tr.SetOrigin (meshtrans.GetOrigin ());
    return camtrans.Other2This (tr.This2Other (point));
  }
  else
    return point;
}

void Marker::Render3D ()
{
  const csOrthoTransform& camtrans = mgr->camera->GetTransform ();
  const csReversibleTransform& meshtrans = GetTransform ();
  for (size_t i = 0 ; i < lines.GetSize () ; i++)
  {
    MarkerLine& line = lines[i];
    csVector3 v1 = TransPoint (camtrans, meshtrans, line.space, line.v1);
    csVector3 v2 = TransPoint (camtrans, meshtrans, line.space, line.v2);
    // @@@ Do proper clipping?
    if (v1.z > .1 && v2.z > .1)
    {
      //mgr->g3d->DrawLine (v1, v2, mgr->g3d->GetPerspectiveAspect (),
	//line.color->GetColor (selectionLevel));
      csPen* pen = line.color->GetPen (selectionLevel);
      csVector2 s1 = mgr->camera->Perspective (v1);
      csVector2 s2 = mgr->camera->Perspective (v2);
      pen->DrawLine (int (s1.x), mgr->g2d->GetHeight () - int (s1.y),
	  int (s2.x), mgr->g2d->GetHeight () - int (s2.y));
    }
  }
}

void Marker::Render2D ()
{
}

void Marker::Line (MarkerSpace space,
      const csVector3& v1, const csVector3& v2, iMarkerColor* color)
{
  MarkerLine line;
  line.space = space;
  line.v1 = v1;
  line.v2 = v2;
  line.color = static_cast<MarkerColor*> (color);
  lines.Push (line);
}

void Marker::Clear ()
{
  lines.Empty ();
}

//---------------------------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (MarkerManager)

MarkerManager::MarkerManager (iBase *iParent)
  : scfImplementationType (this, iParent)
{  
  object_reg = 0;
  camera = 0;
}

MarkerManager::~MarkerManager ()
{
}

bool MarkerManager::Initialize (iObjectRegistry *object_reg)
{
  MarkerManager::object_reg = object_reg;
  engine = csQueryRegistry<iEngine> (object_reg);
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  g2d = g3d->GetDriver2D ();

  return true;
}

void MarkerManager::Frame2D ()
{
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    markers[i]->Render2D ();
}

void MarkerManager::Frame3D ()
{
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    markers[i]->Render3D ();
}

void MarkerManager::SetSelectionLevel (int level)
{
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    markers[i]->SetSelectionLevel (level);
}

iMarkerColor* MarkerManager::FindMarkerColor (const char* name)
{
  for (size_t i = 0 ; i < markerColors.GetSize () ; i++)
    if (markerColors[i]->GetNameString () == name)
      return markerColors[i];
  return 0;
}

}
CS_PLUGIN_NAMESPACE_END(MarkerManager)

