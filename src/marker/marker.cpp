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

#include "csutil/event.h"
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

static float SqDistance2d (const csVector2& p1, const csVector2& p2)
{
  csVector2 d = p1 - p2;
  return d*d;
}

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

void MarkerHitArea::DefineDrag (uint button, bool shift, bool ctrl, bool alt,
      MarkerSpace constrainSpace,
      bool constrainXplane, bool constrainYplane, bool constrainZplane,
      iMarkerCallback* cb)
{
  MarkerDraggingMode* dm = new MarkerDraggingMode ();
  dm->cb = cb;
  dm->button = button;
  dm->shift = shift;
  dm->ctrl = ctrl;
  dm->alt = alt;
  dm->constrainSpace = constrainSpace;
  dm->constrainXplane = constrainXplane;
  dm->constrainYplane = constrainYplane;
  dm->constrainZplane = constrainZplane;
  draggingModes.Push (dm);
}

MarkerDraggingMode* MarkerHitArea::FindDraggingMode (uint button,
    bool shift, bool ctrl, bool alt) const
{
  for (size_t i = 0 ; i < draggingModes.GetSize () ; i++)
  {
    MarkerDraggingMode* dm = draggingModes[i];
    if (dm->button == button && dm->shift == shift && dm->ctrl == ctrl && dm->alt == alt)
    {
      return dm;
    }
  }
  return 0;
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
  if (!visible) return;

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
      csPen* pen = line.color->GetPen (selectionLevel);
      csVector2 s1 = mgr->camera->Perspective (v1);
      csVector2 s2 = mgr->camera->Perspective (v2);
 
      int x1 = int (s1.x);
      int y1 = mgr->g2d->GetHeight () - int (s1.y);
      int x2 = int (s2.x);
      int y2 = mgr->g2d->GetHeight () - int (s2.y);

      pen->DrawLine (x1, y1, x2, y2);

      if (line.arrow)
      {
        //float d = sqrt (SqDistance2d (s1, s2));
	int dx = (x2-x1) / 4;
	int dy = (y2-y1) / 4;
	int dxr = -(y2-y1) / 4;
	int dyr = (x2-x1) / 4;
        pen->DrawLine (x2, y2, x2-dx+dxr, y2-dy+dyr);
        pen->DrawLine (x2, y2, x2-dx-dxr, y2-dy-dyr);
      }
    }
  }
}

void Marker::Render2D ()
{
}

void Marker::Line (MarkerSpace space,
      const csVector3& v1, const csVector3& v2, iMarkerColor* color,
      bool arrow)
{
  MarkerLine line;
  line.space = space;
  line.v1 = v1;
  line.v2 = v2;
  line.color = static_cast<MarkerColor*> (color);
  line.arrow = arrow;
  lines.Push (line);
}

void Marker::Clear ()
{
  lines.Empty ();
}

iMarkerHitArea* Marker::HitArea (MarkerSpace space, const csVector3& center,
      float radius, int data)
{
  csRef<MarkerHitArea> hitArea;
  hitArea.AttachNew (new MarkerHitArea ());
  hitArea->SetSpace (space);
  hitArea->SetCenter (center);
  hitArea->SetSqRadius (radius * radius);
  hitArea->SetData (data);
  hitAreas.Push (hitArea);
  return hitArea;
}

void Marker::ClearHitAreas ()
{
  hitAreas.Empty ();
}

float Marker::CheckHitAreas (int x, int y, MarkerHitArea*& bestHitArea)
{
  const csOrthoTransform& camtrans = mgr->camera->GetTransform ();
  const csReversibleTransform& meshtrans = GetTransform ();
  csVector2 f (float (x), float (mgr->g2d->GetHeight () - y));
  bestHitArea = 0;
  float bestSqRadius = 10000000.0f;
  bool hit = false;
  for (size_t i = 0 ; i < hitAreas.GetSize () ; i++)
  {
    MarkerHitArea* hitArea = hitAreas[i];
    csVector3 c = TransPoint (camtrans, meshtrans, hitArea->GetSpace (), hitArea->GetCenter ());
    if (c.z > .1)
    {
      csVector2 s = mgr->camera->Perspective (c);
      float sqd = SqDistance2d (s, f);
      if (sqd <= hitArea->GetSqRadius () && sqd <= bestSqRadius)
      {
	bestSqRadius = sqd;
	bestHitArea = hitArea;
	hit = true;
      }
    }
  }
  if (hit) return bestSqRadius;
  else return -1.0f;
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

bool MarkerManager::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  MarkerHitArea* hitArea = FindHitArea (mouseX, mouseY);
  if (hitArea)
  {
    uint32 mod = csMouseEventHelper::GetModifiers (&ev);
    bool shift = (mod & CSMASK_SHIFT) != 0;
    bool ctrl = (mod & CSMASK_CTRL) != 0;
    bool alt = (mod & CSMASK_ALT) != 0;
    MarkerDraggingMode* dm = hitArea->FindDraggingMode (but, shift, ctrl, alt);
    if (dm)
    {
      printf ("Start dragging mode!\n");
      fflush (stdout);
      return true;
    }
  }

  return false;
}

bool MarkerManager::OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

bool MarkerManager::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return false;
}

MarkerHitArea* MarkerManager::FindHitArea (int x, int y)
{
  MarkerHitArea* bestHitArea = 0;
  float bestSqRadius = 10000000.0f;
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    if (markers[i]->IsVisible ())
    {
      MarkerHitArea* hitArea;
      float sqd = markers[i]->CheckHitAreas (x, y, hitArea);
      if (sqd >= 0.0f && sqd < bestSqRadius)
      {
        bestSqRadius = sqd;
        bestHitArea = hitArea;
      }
    }
  return bestHitArea;
}

iMarker* MarkerManager::FindHitMarker (int x, int y, int& data)
{
  iMarker* bestMarker = 0;
  float bestSqRadius = 10000000.0f;
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    if (markers[i]->IsVisible ())
    {
      MarkerHitArea* hitArea;
      float sqd = markers[i]->CheckHitAreas (x, y, hitArea);
      if (sqd >= 0.0f && sqd < bestSqRadius)
      {
        bestSqRadius = sqd;
        bestMarker = markers[i];
        data = hitArea->GetData ();
      }
    }
  return bestMarker;
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

