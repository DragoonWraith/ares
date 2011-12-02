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
#include "csgeom/math3d.h"
#include "iutil/objreg.h"
#include "ivideo/graph2d.h"
#include "ivideo/graph3d.h"
#include "ivideo/fontserv.h"
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

//--------------------------------------------------------------------------------

class GraphLinkStyle : public scfImplementation1<GraphLinkStyle, iGraphLinkStyle>
{
private:
  iMarkerColor* color;
  bool arrow;
  float strength;

public:
  GraphLinkStyle () : scfImplementationType (this),
    color (0), arrow (false), strength (1.0f) { }
  virtual ~GraphLinkStyle () { }

  virtual void SetColor (iMarkerColor* color) { GraphLinkStyle::color = color; }
  virtual iMarkerColor* GetColor () const { return color; }
  virtual void SetArrow (bool a) { arrow = a; }
  virtual bool IsArrow () const { return arrow; }
  virtual void SetLinkStrength (float w) { strength = w; }
  virtual float GetLinkStrength () const { return strength; }
};

//--------------------------------------------------------------------------------

class GraphNodeStyle : public scfImplementation1<GraphNodeStyle, iGraphNodeStyle>
{
private:
  iMarkerColor* fgColor;
  iMarkerColor* bgColor;
  iMarkerColor* textColor;
  int roundness;
  float weightFactor;

public:
  GraphNodeStyle () : scfImplementationType (this),
    fgColor (0), bgColor (0), textColor (0), roundness (10), weightFactor (1.0f) { }
  virtual ~GraphNodeStyle () { }

  virtual void SetBorderColor (iMarkerColor* color) { fgColor = color; }
  virtual iMarkerColor* GetBorderColor () const { return fgColor; }
  virtual void SetBackgroundColor (iMarkerColor* color) { bgColor = color; }
  virtual iMarkerColor* GetBackgroundColor () const { return bgColor; }
  virtual void SetTextColor (iMarkerColor* color) { textColor = color; }
  virtual iMarkerColor* GetTextColor () const { return textColor; }
  virtual void SetRoundness (int roundness) { GraphNodeStyle::roundness = roundness; }
  virtual int GetRoundness () const { return roundness; }
  virtual void SetWeightFactor (float w) { weightFactor = w; }
  virtual float GetWeightFactor () const { return weightFactor; }
};

//--------------------------------------------------------------------------------

GraphView::GraphView (MarkerManager* mgr) : scfImplementationType (this), mgr (mgr), visible (false)
{
  draggingMarker = 0;
  coolDownPeriod = true;

  nodeForceFactor = 750.0f;
  linkForceFactor = 0.3f;

  secondsTodo = 0.0f;

  draggingMarker = 0;
  activeMarker = 0;
}

void GraphView::ActivateNode (const char* name)
{
  GraphNode n;
  const GraphNode& node = nodes.Get (name, n);
  ActivateMarker (node.marker, name);
}

const char* GraphView::GetActiveNode () const
{
  if (!activeMarker) return 0;
  csHash<GraphNode,csString>::ConstGlobalIterator it = nodes.GetIterator ();
  while (it.HasNext ())
  {
    csString key;
    const GraphNode& node = it.Next (key);
    if (node.marker == activeMarker) return node.name;
  }
  return 0;
}

void GraphView::ActivateMarker (iMarker* marker, const char* node)
{
  if (activeMarker) activeMarker->SetSelectionLevel (SELECTION_NONE);
  activeMarker = marker;
  if (activeMarker) activeMarker->SetSelectionLevel (SELECTION_ACTIVE);
  if (activeMarker && !node)
  {
    csHash<GraphNode,csString>::GlobalIterator it = nodes.GetIterator ();
    while (it.HasNext ())
    {
      csString key;
      const GraphNode& n = it.Next (key);
      if (marker == n.marker) { node = n.name; break; }
    }
  }
  for (size_t i = 0 ; i < callbacks.GetSize () ; i++)
    callbacks[i]->ActivateNode (node);
}

void GraphView::AddNodeActivationCallback (iGraphNodeCallback* cb)
{
  callbacks.Push (cb);
}

void GraphView::ForcePosition (const char* name, const csVector2& pos)
{
  GraphNode n;
  GraphNode& node = nodes.Get (name, n);
  node.frozen = true;
  if (node.marker)
    node.marker->SetPosition (pos);
}

const char* GraphView::FindHitNode (int mouseX, int mouseY)
{
  int data;
  iMarker* marker = mgr->FindHitMarker (mouseX, mouseY, data);
  csHash<GraphNode,csString>::GlobalIterator it = nodes.GetIterator ();
  while (it.HasNext ())
  {
    GraphNode& node = it.Next (currentNode);
    if (node.marker == marker) return currentNode;
  }
  return 0;
}

bool GraphView::IsLinked (const char* n1, const char* n2)
{
  for (size_t i = 0 ; i < links.GetSize () ; i++)
  {
    GraphLink& l = links[i];
    if (l.node1 == n1 && l.node2 == n2) return true;
    if (l.node2 == n1 && l.node1 == n2) return true;
  }
  return false;
}

void GraphView::HandlePushingForces ()
{
  int fw = mgr->GetG2D ()->GetWidth ();
  int fh = mgr->GetG2D ()->GetHeight ();
  // Handle all pushing forces.
  csHash<GraphNode,csString>::GlobalIterator it = nodes.GetIterator ();
  while (it.HasNext ())
  {
    csString key;
    GraphNode& node = it.Next (key);
    if (node.marker == draggingMarker || node.frozen) continue;

    const csVector2& pos = node.marker->GetPosition ();
    node.netForce.Set (0, 0);

    // The border pushes too.
    node.netForce += (pos - csVector2 (0, pos.y)) * nodeForceFactor / (pos.x * pos.x);
    node.netForce += (pos - csVector2 (fw, pos.y)) * nodeForceFactor / ((fw-pos.x) * (fw-pos.x));
    node.netForce += (pos - csVector2 (pos.x, 0)) * nodeForceFactor / (pos.y * pos.y);
    node.netForce += (pos - csVector2 (pos.x, fh)) * nodeForceFactor / ((fh-pos.y) * (fh-pos.y));

    csHash<GraphNode,csString>::GlobalIterator it2 = nodes.GetIterator ();
    while (it2.HasNext ())
    {
      csString key2;
      GraphNode& node2 = it2.Next (key2);
      if (node.marker != node2.marker)
      {
	const csVector2& pos2 = node2.marker->GetPosition ();
	float sqdist = SqDistance2d (pos, pos2);
	if (sqdist < .0001) sqdist = .0001;
	node.netForce += (pos-pos2) * node2.weightFactor * nodeForceFactor / sqdist;
      }
    }
  }
}

void GraphView::HandlePullingLinks ()
{
  // Handle all links.
  GraphNode n;
  for (size_t i = 0 ; i < links.GetSize () ; i++)
  {
    GraphLink& l = links[i];
    GraphNode& node1 = nodes.Get (l.node1, n);
    GraphNode& node2 = nodes.Get (l.node2, n);
    if (node1.marker && node2.marker)
    {
      const csVector2& pos1 = node1.marker->GetPosition ();
      const csVector2& pos2 = node2.marker->GetPosition ();
      csVector2 force = (pos2-pos1) * l.strength * linkForceFactor;
      node1.netForce += force;
      node2.netForce -= force;
    }
  }
}

bool GraphView::MoveNodes (float seconds)
{
  int fw = mgr->GetG2D ()->GetWidth ();
  int fh = mgr->GetG2D ()->GetHeight ();
  bool allCool = true;
  csHash<GraphNode,csString>::GlobalIterator it = nodes.GetIterator ();
  while (it.HasNext ())
  {
    csString key;
    GraphNode& node = it.Next (key);
    if (node.marker == draggingMarker || node.frozen) continue;

    node.velocity = (node.velocity + node.netForce) * 0.85f;
    csVector2 pos = node.marker->GetPosition ();
    csVector2 oldpos = pos;
    pos += node.velocity * (seconds * 50.0f);
#   define NODE_MARGIN 10
    if (pos.x > fw-node.size.x/2-NODE_MARGIN) pos.x = fw-node.size.x/2-NODE_MARGIN;
    else if (pos.x < node.size.x/2+NODE_MARGIN) pos.x = node.size.x/2+NODE_MARGIN;
    if (pos.y > fh-node.size.y/2-NODE_MARGIN) pos.y = fh-node.size.y/2-NODE_MARGIN;
    else if (pos.y < node.size.y/2+NODE_MARGIN) pos.y = node.size.y/2+NODE_MARGIN;
    node.marker->SetPosition (pos);
    if (coolDownPeriod)
    {
      float d = SqDistance2d (pos, oldpos);
      if (d > .00001) allCool = false;
    }
  }
  return allCool;
}

void GraphView::UpdateFrame ()
{
  float seconds = mgr->GetVC ()->GetElapsedSeconds ();
  secondsTodo += seconds;
  // Protection to make sure we don't get an excessive elapsed time.
  // This is needed because frame updating stops while we're in a context menu.
  if (secondsTodo > .1) secondsTodo = .1;

  bool loop = true;
  int maxLoop = 20;
  while (loop)
  {
    bool allCool = true;
    while (secondsTodo > .01f)
    {
      HandlePushingForces ();
      HandlePullingLinks ();
      if (!MoveNodes (.01f)) allCool = false;
      secondsTodo -= .01f;
    }

    maxLoop--;
    if (allCool || maxLoop <= 0) coolDownPeriod = false;
    loop = coolDownPeriod;
    secondsTodo = 0.1f;
  }
}

void GraphView::Render3D ()
{
  GraphNode n;
  for (size_t i = 0 ; i < links.GetSize () ; i++)
  {
    GraphLink& l = links[i];
    GraphNode& node1 = nodes.Get (l.node1, n);
    GraphNode& node2 = nodes.Get (l.node2, n);
    if ((!node1.marker) || (!node2.marker))
    {
      if (!node1.marker) printf ("Link: node '%s' does not exist!\n", l.node1.GetData ());
      if (!node2.marker) printf ("Link: node '%s' does not exist!\n", l.node2.GetData ());
      continue;	// Ignore this link.
    }
    csVector2 pos1 = node1.marker->GetPosition ();
    csVector2 pos2 = node2.marker->GetPosition ();
    iMarkerColor* color = l.color;
    csPen* pen = static_cast<MarkerColor*> (color)->GetPen (1);
    pen->DrawLine (pos1.x, pos1.y, pos2.x, pos2.y);

    if (l.arrow)
    {
      csVector2 c = (pos1+pos2)/2.0f;
      int dx = pos2.x-pos1.x;
      int dy = pos2.y-pos1.y;
      int dxr = -(pos2.y-pos1.y);
      int dyr = pos2.x-pos1.x;
      csVector2 r1 (-dx+dxr, -dy+dyr); r1.Normalize (); r1 *= 10.0f;
      csVector2 r2 (-dx-dxr, -dy-dyr); r2.Normalize (); r2 *= 10.0f;
      pen->DrawLine (c.x, c.y, c.x+r1.x, c.y+r1.y);
      pen->DrawLine (c.x, c.y, c.x+r2.x, c.y+r2.y);
    }
  }
}

void GraphView::SetVisible (bool v)
{
  visible = v;
  if (visible)
  {
    coolDownPeriod = true;
  }
  csHash<GraphNode,csString>::GlobalIterator it = nodes.GetIterator ();
  while (it.HasNext ())
  {
    iMarker* marker = it.Next ().marker;
    marker->SetVisible (v);
  }
}

void GraphView::Clear ()
{
  csHash<GraphNode,csString>::GlobalIterator it = nodes.GetIterator ();
  while (it.HasNext ())
  {
    iMarker* marker = it.Next ().marker;
    mgr->DestroyMarker (marker);
  }
  nodes.DeleteAll ();
  links.DeleteAll ();
  activeMarker = 0;
}

class MarkerCallback : public scfImplementation1<MarkerCallback,iMarkerCallback>
{
private:
  GraphView* view;

public:
  MarkerCallback (GraphView* view) : scfImplementationType (this),
    view (view) { printf ("view=%p\n", view); fflush (stdout); }
  virtual ~MarkerCallback () { }
  virtual void StartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers)
  {
    view->ActivateMarker (marker);
    view->SetDraggingMarker (marker);
  }
  virtual void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
  {
    marker->SetPosition (csVector2 (pos.x, pos.y));
  }
  virtual void MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform) { }
  virtual void StopDragging (iMarker* marker, iMarkerHitArea* area)
  {
    view->SetDraggingMarker (0);
  }
};

void GraphView::LinkNode (const char* node1, const char* node2,
      iGraphLinkStyle* style)
{
  if (!style) style = defaultLinkStyle;
  GraphLink l;
  l.node1 = node1;
  l.node2 = node2;
  l.color = style->GetColor ();
  l.arrow = style->IsArrow ();
  l.strength = style->GetLinkStrength ();
  links.Push (l);
}

void GraphView::UpdateNodeMarker (iMarker* marker, const char* label,
    iGraphNodeStyle* style, int& w, int& h)
{
  int fw = mgr->GetG2D ()->GetWidth ();
  int fh = mgr->GetG2D ()->GetHeight ();

  csStringArray labelArray (label, "\n");

  int textHeight = mgr->GetFont ()->GetTextHeight ();
  h = textHeight * labelArray.GetSize () + 6;
  w = 0;
  for (size_t i = 0 ; i < labelArray.GetSize () ; i++)
  {
    int ww, hh;
    mgr->GetFont ()->GetDimensions (labelArray.Get (i), ww, hh);
    if (ww > w) w = ww;
  }
  w += 10;
  if (*label == 0) w = h = 1;

  int w2 = w / 2;
  int h2 = h / 2;

  if (!style) style = defaultStyle;
  marker->RoundedBox2D (MARKER_2D, csVector3 (-w2, -h2, 0),
    csVector3 (w2, h2, 0), style->GetRoundness (), style->GetBackgroundColor ());
  marker->RoundedBox2D (MARKER_2D, csVector3 (-w2, -h2, 0),
    csVector3 (w2, h2, 0), style->GetRoundness (), style->GetBorderColor ());
  marker->Text (MARKER_2D, csVector3 (0, 0, 0), labelArray, style->GetTextColor (), true);
  marker->SetSelectionLevel (SELECTION_NONE);
  marker->SetVisible (visible);
  marker->SetPosition (csVector2 (w2+rng.Get ()*(fw-w2-w2), h2+rng.Get ()*(fh-h2-h2)));

  // Make an invisible hit area.
  csVector3 size3 (w2, h2, 0);
  iMarkerHitArea* hitArea = marker->HitArea (MARKER_2D, csBox3 (-size3, size3), 0);
  csRef<MarkerCallback> cb;
  cb.AttachNew (new MarkerCallback (this));
  hitArea->DefineDrag (0, 0, MARKER_2D, CONSTRAIN_NONE, cb);
}

void GraphView::CreateNode (const char* name, const char* label,
    iGraphNodeStyle* style)
{
  if (!label) label = name;
  int w, h;
  iMarker* marker = mgr->CreateMarker ();
  UpdateNodeMarker (marker, label, style, w, h);

  GraphNode node;
  node.name = name;
  node.marker = marker;
  node.velocity.Set (0, 0);
  node.size = csVector2 (w, h);
  node.weightFactor = style->GetWeightFactor ();
  nodes.Put (name, node);
}

void GraphView::RemoveNode (const char* name)
{
  GraphNode node = nodes.Get (name, GraphNode ());
  if (node.marker)
  {
    nodes.DeleteAll (name);
    if (node.marker == activeMarker) activeMarker = 0;
    if (node.marker == draggingMarker) draggingMarker = 0;
    if (mgr->GetDraggingMarker () == node.marker)
      mgr->StopDrag ();

    mgr->DestroyMarker (node.marker);
  }
}

void GraphView::ChangeNode (const char* name, const char* label,
    iGraphNodeStyle* style)
{
  int w, h;
  GraphNode n;
  GraphNode& node = nodes.Get (name, n);
#if 1
  if (mgr->GetDraggingMarker () == node.marker)
    mgr->StopDrag ();
  node.marker->Clear ();
  node.marker->ClearHitAreas ();
  UpdateNodeMarker (node.marker, label, style, w, h);
  node.size = csVector2 (w, h);
#else
  bool setActive = false;
  csVector2 oldPos = node.marker->GetPosition ();

  if (node.marker == activeMarker) { activeMarker = 0; setActive = true; }
  if (mgr->GetDraggingMarker () == node.marker)
    mgr->StopDrag ();
  if (node.marker == draggingMarker) draggingMarker = 0;

  mgr->DestroyMarker (node.marker);
  if (!label) label = name;
  iMarker* marker = mgr->CreateMarker ();
  UpdateNodeMarker (marker, label, style, w, h);
  marker->SetPosition (oldPos);
  node.marker = marker;
  node.size = csVector2 (w, h);
  if (setActive)
  {
    activeMarker = node.marker;
    activeMarker->SetSelectionLevel (SELECTION_ACTIVE);
  }
#endif
}

void GraphView::ReplaceNode (const char* oldNode, const char* newNode,
      const char* label, iGraphNodeStyle* style)
{
  printf ("Replacing old '%s' with new '%s'!\n", oldNode, newNode); fflush (stdout);
  if (!strcmp (oldNode, newNode))
  {
    ChangeNode (oldNode, label, style);
    return;
  }
  GraphNode n;
  const GraphNode& nodeOld = nodes.Get (oldNode, n);
  CreateNode (newNode, label, style);
  GraphNode& nodeNew = nodes.Get (newNode, n);
  nodeNew.velocity = n.velocity;
  nodeNew.netForce = n.netForce;
  if (n.marker)
    nodeNew.marker->SetPosition (n.marker->GetPosition ());
  if (nodeOld.marker == activeMarker)
  {
    activeMarker = nodeNew.marker;
    activeMarker->SetSelectionLevel (SELECTION_ACTIVE);
  }
  RemoveNode (oldNode);

  for (size_t i = 0 ; i < links.GetSize () ; i++)
  {
    GraphLink& l = links[i];
    if (l.node1 == oldNode) l.node1 = newNode;
    if (l.node2 == oldNode) l.node2 = newNode;
  }
}

//--------------------------------------------------------------------------------

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

void MarkerColor::EnableFill (int selectionLevel, bool fill)
{
  csPen* pen = GetOrCreatePen (selectionLevel);
  if (fill)
    pen->SetFlag (CS_PEN_FILL);
  else
    pen->ClearFlag (CS_PEN_FILL);
}

//---------------------------------------------------------------------------------------

static csVector3 TransPointWorld (
    const csOrthoTransform& camtrans,
    const csReversibleTransform& meshtrans,
    MarkerSpace space,
    const csVector3& point)
{
  if (space == MARKER_CAMERA)
    return camtrans.This2Other (point);
  else if (space == MARKER_OBJECT)
    return meshtrans.This2Other (point);
  else if (space == MARKER_WORLD)
    return meshtrans.This2Other (point);
  else
    return point;
}

static csVector3 TransPointCam (
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

//--------------------------------------------------------------------------

iMarker* InternalMarkerHitArea::GetMarker () const
{
  return static_cast<iMarker*> (marker);
}

MarkerDraggingMode* InternalMarkerHitArea::FindDraggingMode (
    uint button, uint32 modifiers) const
{
  for (size_t i = 0 ; i < draggingModes.GetSize () ; i++)
  {
    MarkerDraggingMode* dm = draggingModes[i];
    if (dm->button == button &&
	(dm->modifiers & CSMASK_MODIFIERS) == (modifiers & CSMASK_MODIFIERS))
    {
      return dm;
    }
  }
  return 0;
}

void InternalMarkerHitArea::DefineDrag (uint button, uint32 modifiers,
      MarkerSpace constrainSpace, uint32 constrainPlane,
      iMarkerCallback* cb)
{
  MarkerDraggingMode* dm = new MarkerDraggingMode ();
  dm->cb = cb;
  dm->button = button;
  dm->modifiers = modifiers;
  dm->constrainSpace = constrainSpace;
  dm->constrainPlane = constrainPlane;
  draggingModes.Push (dm);
}

//--------------------------------------------------------------------------

void InvBoxMarkerHitArea::Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos)
{
}

float InvBoxMarkerHitArea::CheckHit (int x, int y,
    const csOrthoTransform& camtrans, const csReversibleTransform& meshtrans,
    MarkerManager* mgr, const csVector2& pos)
{
  csVector3 c1 = TransPointCam (camtrans, meshtrans, space, box.Min ());
  csVector3 c2 = TransPointCam (camtrans, meshtrans, space, box.Max ());
  if (space == MARKER_2D || (c1.z > .5 && c2.z > .5))
  {
    csVector2 f (float (x), float (mgr->g2d->GetHeight () - y));
    csVector2 s1, s2;
    if (space != MARKER_2D)
    {
      s1 = mgr->camera->Perspective (c1) + pos;
      s2 = mgr->camera->Perspective (c2) + pos;
    }
    else
    {
      s1.Set (pos.x + c1.x, mgr->g2d->GetHeight () - (pos.y + c1.y));
      s2.Set (pos.x + c2.x, mgr->g2d->GetHeight () - (pos.y + c2.y));
    }
    if (f.x >= s1.x && f.x <= s2.x && f.y >= s2.y && f.y <= s1.y)	// s1.y and s2.y are swapped!
    {
      float d = sqrt (SqDistance2d ((s1+s2)/2.0f, f));
      return d;
    }
  }
  return -1.0f;
}

//--------------------------------------------------------------------------

csVector2 CircleMarkerHitArea::GetPerspectiveRadius (iView* view, float z) const
{
  iCamera* camera = view->GetCamera ();
  csVector4 r4 (radius, radius, z, 1.0f);
  csVector4 t = camera->GetProjectionMatrix () * r4;
  csVector2 r (t.x / t.w - 1.0f, t.y / t.w - 1.0f);
  return view->NormalizedToScreen (r);
}

void CircleMarkerHitArea::Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos)
{
  if (GetColor ())
  {
    csVector3 c = TransPointCam (camtrans, meshtrans, space, center);
    if (space == MARKER_2D || c.z > .5)
    {
      int h = mgr->g2d->GetHeight ();
      csVector2 s;
      if (space != MARKER_2D)
	s = mgr->camera->Perspective (c) + pos;
      else
	s.Set (pos.x + c.x, h - (pos.y + c.y));
      int x = int (s.x);
      int y = h - int (s.y);
      int mouseX = mgr->GetMouseX ();
      int mouseY = mgr->GetMouseY ();
      csVector2 r;
      if (space != MARKER_2D)
	r = GetPerspectiveRadius (mgr->view, c.z);
      else
	r.Set (radius, radius);
      bool selected = mouseX >= (x-r.x) && mouseX <= (x+r.x) &&
	mouseY >= (y-r.y) && mouseY <= (y+r.y);
      csPen* pen = color->GetPen (selected ? SELECTION_SELECTED : SELECTION_NONE);
      pen->DrawArc (x-r.x, y-r.y, x+r.x, y+r.y);
    }
  }
}

float CircleMarkerHitArea::CheckHit (int x, int y,
    const csOrthoTransform& camtrans, const csReversibleTransform& meshtrans,
    MarkerManager* mgr, const csVector2& pos)
{
  csVector3 c = TransPointCam (camtrans, meshtrans, space, center);
  if (space == MARKER_2D || c.z > .5)
  {
    csVector2 f (float (x), float (mgr->g2d->GetHeight () - y));
    csVector2 s;
    if (space != MARKER_2D)
      s = mgr->camera->Perspective (c) + pos;
    else
      s.Set (pos.x + c.x, mgr->g2d->GetHeight () - (pos.y + c.y));
    float d = sqrt (SqDistance2d (s, f));
    csVector2 r;
    if (space != MARKER_2D)
      r = GetPerspectiveRadius (mgr->view, c.z);
    else
      r.Set (radius, radius);
    float radius = (r.x+r.y) / 2.0f;
    if (d <= radius)
      return d;
  }
  return -1.0f;
}

//------------------------------------------------------------------------------

void MarkerLine::Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel)
{
  csVector3 v1 = TransPointCam (camtrans, meshtrans, space, vec1);
  csVector3 v2 = TransPointCam (camtrans, meshtrans, space, vec2);
  // @@@ Do proper clipping?
  if (space == MARKER_2D || (v1.z > .5 && v2.z > .5))
  {
    int h = mgr->g2d->GetHeight ();
    csPen* pen = color->GetPen (selectionLevel);
    csVector2 s1, s2;
    if (space != MARKER_2D)
    {
      s1 = mgr->camera->Perspective (v1);
      s2 = mgr->camera->Perspective (v2);
    }
    else
    {
      s1.Set (v1.x, h-v1.y);
      s2.Set (v2.x, h-v2.y);
    }

    int x1 = int (s1.x);
    int y1 = h - int (s1.y);
    int x2 = int (s2.x);
    int y2 = h - int (s2.y);

    pen->DrawLine (pos.x + x1, pos.y + y1, pos.x + x2, pos.y + y2);

    if (arrow)
    {
      //float d = sqrt (SqDistance2d (s1, s2));
      int dx = (x2-x1) / 4;
      int dy = (y2-y1) / 4;
      int dxr = -(y2-y1) / 4;
      int dyr = (x2-x1) / 4;
      pen->DrawLine (pos.x + x2, pos.y + y2, pos.x + x2-dx+dxr, pos.y + y2-dy+dyr);
      pen->DrawLine (pos.x + x2, pos.y + y2, pos.x + x2-dx-dxr, pos.y + y2-dy-dyr);
    }
  }
}

void MarkerRoundedBox::Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel)
{
  csVector3 v1 = TransPointCam (camtrans, meshtrans, space, vec1);
  csVector3 v2 = TransPointCam (camtrans, meshtrans, space, vec2);
  // @@@ Do proper clipping?
  if (space == MARKER_2D || (v1.z > .5 && v2.z > .5))
  {
    int h = mgr->g2d->GetHeight ();
    csPen* pen = color->GetPen (selectionLevel);
    csVector2 s1, s2;
    if (space != MARKER_2D)
    {
      s1 = mgr->camera->Perspective (v1);
      s2 = mgr->camera->Perspective (v2);
    }
    else
    {
      s1.Set (v1.x, h-v1.y);
      s2.Set (v2.x, h-v2.y);
    }

    int x1 = int (s1.x);
    int y1 = h - int (s1.y);
    int x2 = int (s2.x);
    int y2 = h - int (s2.y);

    pen->DrawRoundedRect (pos.x + x1, pos.y + y1, pos.x + x2, pos.y + y2,
	roundness);
  }
}

void MarkerText::Render2D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel)
{
  csVector3 v = TransPointCam (camtrans, meshtrans, space, position);
  // @@@ Do proper clipping?
  if (space == MARKER_2D || v.z > .5)
  {
    int h = mgr->g2d->GetHeight ();
    csPen* pen = color->GetPen (selectionLevel);
    csVector2 s;
    if (space != MARKER_2D)
      s = mgr->camera->Perspective (v);
    else
      s.Set (v.x, h-v.y);

    int x1 = int (s.x);
    int y1 = h - int (s.y);

    if (centered)
      pen->WriteLinesBoxed (mgr->GetFont (), pos.x + x1, pos.y + y1,
	pos.x + x1, pos.y + y1, CS_PEN_TA_CENTER, CS_PEN_TA_CENTER, text);
    else
      pen->WriteLines (mgr->GetFont (), pos.x + x1, pos.y + y1, text);
  }
}

//------------------------------------------------------------------------------

const csReversibleTransform& Marker::GetTransform () const
{
  if (attachedMesh) return attachedMesh->GetMovable ()->GetTransform ();
  else return trans;
}

void Marker::Render3D ()
{
  if (!visible) return;

  const csOrthoTransform& camtrans = mgr->camera->GetTransform ();
  const csReversibleTransform& meshtrans = GetTransform ();
  for (size_t i = 0 ; i < primitives.GetSize () ; i++)
  {
    MarkerPrimitive* prim = primitives[i];
    prim->Render3D (camtrans, meshtrans, mgr, pos, selectionLevel);
  }

  bool mouseOverDrag = false;
  for (size_t i = 0 ; i < hitAreas.GetSize () ; i++)
  {
    InternalMarkerHitArea* ha = static_cast<InternalMarkerHitArea*> (hitAreas[i]);
    ha->Render3D (camtrans, meshtrans, mgr, pos);
    if (ha->HitAreaHiLightsMarker ())
    {
      float r = ha->CheckHit (mgr->GetMouseX (), mgr->GetMouseY (),
	  camtrans, meshtrans, mgr, pos);
      if (r >= 0.0f)
        mouseOverDrag = true;
    }
  }
  if (GetSelectionLevel () != SELECTION_ACTIVE)
  {
    if (mouseOverDrag) SetSelectionLevel (SELECTION_SELECTED);
    else SetSelectionLevel (SELECTION_NONE);
  }
}

void Marker::Render2D ()
{
  if (!visible) return;

  const csOrthoTransform& camtrans = mgr->camera->GetTransform ();
  const csReversibleTransform& meshtrans = GetTransform ();

  for (size_t i = 0 ; i < primitives.GetSize () ; i++)
  {
    MarkerPrimitive* prim = primitives[i];
    prim->Render2D (camtrans, meshtrans, mgr, pos, selectionLevel);
  }
}

void Marker::Line (MarkerSpace space,
      const csVector3& v1, const csVector3& v2, iMarkerColor* color,
      bool arrow)
{
  MarkerLine* line = new MarkerLine ();
  line->space = space;
  line->vec1 = v1;
  line->vec2 = v2;
  line->color = static_cast<MarkerColor*> (color);
  line->arrow = arrow;
  primitives.Push (line);
}

void Marker::RoundedBox2D (MarkerSpace space,
      const csVector3& corner1, const csVector3& corner2, int roundness,
      iMarkerColor* color)
{
  MarkerRoundedBox* box = new MarkerRoundedBox ();
  box->space = space;
  box->vec1 = corner1;
  box->vec2 = corner2;
  box->color = static_cast<MarkerColor*> (color);
  box->roundness = roundness;
  primitives.Push (box);
}

void Marker::Text (MarkerSpace space, const csVector3& pos,
      const csStringArray& text, iMarkerColor* color, bool centered)
{
  MarkerText* txt = new MarkerText ();
  txt->space = space;
  txt->position = pos;
  txt->color = static_cast<MarkerColor*> (color);
  txt->text = text;
  txt->centered = centered;
  primitives.Push (txt);
}

void Marker::Clear ()
{
  primitives.Empty ();
}

iMarkerHitArea* Marker::HitArea (MarkerSpace space, const csVector3& center,
      float radius, int data, iMarkerColor* color)
{
  csRef<CircleMarkerHitArea> hitArea;
  hitArea.AttachNew (new CircleMarkerHitArea (this));
  hitArea->SetSpace (space);
  hitArea->SetCenter (center);
  hitArea->SetRadius (radius);
  hitArea->SetData (data);
  hitArea->SetColor (static_cast<MarkerColor*> (color));
  hitAreas.Push (hitArea);
  return hitArea;
}

iMarkerHitArea* Marker::HitArea (MarkerSpace space, const csBox3& box,
      int data)
{
  csRef<InvBoxMarkerHitArea> hitArea;
  hitArea.AttachNew (new InvBoxMarkerHitArea (this));
  hitArea->SetSpace (space);
  hitArea->SetBox (box);
  hitArea->SetData (data);
  hitAreas.Push (hitArea);
  return hitArea;
}

void Marker::ClearHitAreas ()
{
  hitAreas.Empty ();
}

float Marker::CheckHitAreas (int x, int y, iMarkerHitArea*& bestHitArea)
{
  const csOrthoTransform& camtrans = mgr->camera->GetTransform ();
  const csReversibleTransform& meshtrans = GetTransform ();
  bestHitArea = 0;
  float bestRadius = 10000000.0f;
  for (size_t i = 0 ; i < hitAreas.GetSize () ; i++)
  {
    InternalMarkerHitArea* hitArea = static_cast<InternalMarkerHitArea*> (
	hitAreas[i]);
    float r = hitArea->CheckHit (x, y, camtrans, meshtrans, mgr, pos);
    if (r < bestRadius && r >= 0.0f)
    {
      bestRadius = r;
      bestHitArea = hitArea;
    }
  }
  if (bestHitArea) return bestRadius;
  else return -1.0f;
}

//---------------------------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (MarkerManager)

MarkerManager::MarkerManager (iBase *iParent)
  : scfImplementationType (this, iParent)
{  
  object_reg = 0;
  camera = 0;
  currentDraggingHitArea = 0;
  currentDraggingMode = 0;
}

MarkerManager::~MarkerManager ()
{
  // First remove the views before the markers because destroying the
  // views also causes markers to be deleted.
  graphViews.DeleteAll ();
  markers.DeleteAll ();
}

bool MarkerManager::Initialize (iObjectRegistry *object_reg)
{
  MarkerManager::object_reg = object_reg;
  engine = csQueryRegistry<iEngine> (object_reg);
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  g2d = g3d->GetDriver2D ();

  font = g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_LARGE);

  return true;
}

void MarkerManager::SetView (iView* view)
{
  MarkerManager::view = view;
  MarkerManager::camera = view->GetCamera ();
}

static bool CheckConstrain (bool constrain, float start, float end, float restr)
{
  if (!constrain) return true;
  if (fabs (start - end) < 0.1f) return false;
  if (end < start && restr > start) return false;
  if (end > start && restr < start) return false;
  return true;
}

bool MarkerManager::FindPlanarIntersection (const csVector3& start, const csVector3& end,
    csVector3& newpos)
{
  uint32 cp = currentDraggingMode->constrainPlane;
  bool cpx = cp & CONSTRAIN_XPLANE;
  bool cpy = cp & CONSTRAIN_YPLANE;
  bool cpz = cp & CONSTRAIN_ZPLANE;

  if (!CheckConstrain (cpx, start.x, end.x, dragRestrict.x)) return false;
  if (!CheckConstrain (cpy, start.y, end.y, dragRestrict.y)) return false;
  if (!CheckConstrain (cpz, start.z, end.z, dragRestrict.z)) return false;

  iMarker* marker = currentDraggingHitArea->GetMarker ();

  // @@@ TODO! Only WORLD and OBJECT supported here!
  csVector3 st, en, dr;
  if (currentDraggingMode->constrainSpace == MARKER_OBJECT)
  {
    dr = marker->GetTransform ().Other2This (dragRestrict);
    st = marker->GetTransform ().Other2This (start);
    en = marker->GetTransform ().Other2This (end);
  }
  else if (currentDraggingMode->constrainSpace == MARKER_WORLD)
  {
    dr = dragRestrict;
    st = start;
    en = end;
  }
  else
  {
    printf ("Unsupported dragging mode %d!\n", currentDraggingMode->constrainSpace);
    fflush (stdout);
    return false;
  }

  if (cpx)
  {
    float dist = csIntersect3::SegmentXPlane (st, en, dr.x, newpos);
    if (dist > 0.8f)
    {
      newpos = st + (en-st).Unit () * 80.0f;
      newpos.x = dr.x;
    }
  }
  if (cpy)
  {
    float dist = csIntersect3::SegmentYPlane (st, en, dr.y, newpos);
    if (dist > 0.8f)
    {
      newpos = st + (en-st).Unit () * 80.0f;
      newpos.y = dr.y;
    }
  }
  if (cpz)
  {
    float dist = csIntersect3::SegmentZPlane (st, en, dr.z, newpos);
    if (dist > 0.8f)
    {
      newpos = st + (en-st).Unit () * 80.0f;
      newpos.z = dr.z;
    }
  }

  if (currentDraggingMode->constrainSpace == MARKER_OBJECT)
  {
    newpos = marker->GetTransform ().This2Other (newpos);
  }
  return true;
}

bool MarkerManager::HandlePlanarRotation (const csVector3& newpos, csMatrix3& m)
{
  uint32 cp = currentDraggingMode->constrainPlane;
  bool cprotx = cp & CONSTRAIN_ROTATEX;
  bool cproty = cp & CONSTRAIN_ROTATEY;
  bool cprotz = cp & CONSTRAIN_ROTATEZ;
  bool cpx = cp & CONSTRAIN_XPLANE;
  bool cpy = cp & CONSTRAIN_YPLANE;
  bool cpz = cp & CONSTRAIN_ZPLANE;

  iMarker* marker = currentDraggingHitArea->GetMarker ();
  csReversibleTransform newrot = marker->GetTransform ();
  csVector3 rel = newpos - newrot.GetOrigin ();
  if (rel < 0.01f) return false;
  csVector3 front = newrot.GetFront (), up = newrot.GetUp (), right = newrot.GetRight ();
  if (cproty)
  {
    up = rel.Unit ();
    if (cpx) front = right % up;
    else if (cpz) right = up % front;
    else return false;
  }
  else if (cprotz)
  {
    front = rel.Unit ();
    if (cpy) right = up % front;
    else if (cpx) up = front % right;
    else return false;
  }
  else if (cprotx)
  {
    right = rel.Unit ();
    if (cpz) up = front % right;
    else if (cpy) front = right % up;
    else return false;
  }
  else return false;
  m.Set (right.x, right.y, right.z, up.x, up.y, up.z, front.x, front.y, front.z);
  return true;
}

void MarkerManager::HandleDrag ()
{
  if (currentDraggingMode)
  {
    iMarker* marker = currentDraggingHitArea->GetMarker ();
    csVector3 newpos;
    bool do_drag = true;
    uint32 cp = currentDraggingMode->constrainPlane;
    if (currentDraggingMode->constrainSpace == MARKER_2D)
    {
      newpos.x = mouseX;
      newpos.y = mouseY;
      newpos.z = 0;
    }
    else
    {
      csVector2 v2d (mouseX, g2d->GetHeight () - mouseY);
      csVector3 v3d = camera->InvPerspective (v2d, 100.0f);
      csVector3 start = camera->GetTransform ().GetOrigin ();
      csVector3 end = camera->GetTransform ().This2Other (v3d);
      if (cp & CONSTRAIN_MESH)
      {
        iMeshWrapper* mesh = currentDraggingHitArea->GetMarker ()->GetAttachedMesh ();
        csFlags oldFlags = mesh->GetFlags ();
        mesh->GetFlags ().Set (CS_ENTITY_NOHITBEAM);
        csSectorHitBeamResult result = camera->GetSector ()->HitBeamPortals (
	    start, end);
        mesh->GetFlags ().SetAll (oldFlags.Get ());
        if (!result.mesh) do_drag = false;	// Safety
        newpos = result.isect;
      }
      else if (cp & CONSTRAIN_PLANE)
      {
        if (!FindPlanarIntersection (start, end, newpos))
	  return;
      }
      else
      {
        newpos = end - start;
        newpos.Normalize ();
        newpos = camera->GetTransform ().GetOrigin () + newpos * dragDistance;
      }
    }

    if (do_drag && currentDraggingMode->cb)
    {
      if (cp & CONSTRAIN_ROTATE)
      {
	csMatrix3 m;
	if (HandlePlanarRotation (newpos, m))
	{
	  csReversibleTransform newrot = marker->GetTransform ();
	  newrot.SetO2T (m);
	  currentDraggingMode->cb->MarkerWantsRotate (marker, currentDraggingHitArea, newrot);
	}
      }
      else
      {
	currentDraggingMode->cb->MarkerWantsMove (marker, currentDraggingHitArea, newpos);
      }
    }
  }
}

void MarkerManager::Frame2D ()
{
  for (size_t i = 0 ; i < graphViews.GetSize () ; i++)
    if (graphViews[i]->IsVisible ())
      graphViews[i]->UpdateFrame ();

  HandleDrag ();
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    markers[i]->Render2D ();
}

void MarkerManager::Frame3D ()
{
  for (size_t i = 0 ; i < graphViews.GetSize () ; i++)
    if (graphViews[i]->IsVisible ())
      graphViews[i]->Render3D ();

  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    markers[i]->Render3D ();
}

iMarker* MarkerManager::GetDraggingMarker ()
{
  if (currentDraggingHitArea)
    return currentDraggingHitArea->GetMarker ();
  else
    return 0;
}

void MarkerManager::StopDrag ()
{
  if (currentDraggingMode)
  {
    if (currentDraggingMode->cb)
      currentDraggingMode->cb->StopDragging (currentDraggingHitArea->GetMarker (),
	  currentDraggingHitArea);
    currentDraggingMode = 0;
    currentDraggingHitArea = 0;
  }
}

bool MarkerManager::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  InternalMarkerHitArea* hitArea = static_cast<InternalMarkerHitArea*> (
      FindHitArea (mouseX, mouseY));
  if (hitArea)
  {
    uint32 mod = csMouseEventHelper::GetModifiers (&ev);
    MarkerDraggingMode* dm = hitArea->FindDraggingMode (but, mod);
    if (dm)
    {
      currentDraggingHitArea = hitArea;
      currentDraggingMode = dm;
      const csOrthoTransform& camtrans = camera->GetTransform ();
      const csReversibleTransform& meshtrans = hitArea->GetMarker ()
	->GetTransform ();
      dragRestrict = TransPointWorld (camtrans, meshtrans,
	  hitArea->GetSpace (), hitArea->GetCenter ());
      dragDistance = (dragRestrict - camtrans.GetOrigin ()).Norm ();
      if (dm->cb)
        dm->cb->StartDragging (hitArea->GetMarker (), hitArea, dragRestrict,
	    but, mod);
      return true;
    }
  }

  return false;
}

bool MarkerManager::OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY)
{
  StopDrag ();
  return false;
}

bool MarkerManager::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  MarkerManager::mouseX = mouseX;
  MarkerManager::mouseY = mouseY;
  return false;
}

iMarkerHitArea* MarkerManager::FindHitArea (int x, int y)
{
  iMarkerHitArea* bestHitArea = 0;
  float bestRadius = 10000000.0f;
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    if (markers[i]->IsVisible ())
    {
      iMarkerHitArea* hitArea;
      float d = markers[i]->CheckHitAreas (x, y, hitArea);
      if (d >= 0.0f && d < bestRadius)
      {
        bestRadius = d;
        bestHitArea = hitArea;
      }
    }
  return bestHitArea;
}

iMarker* MarkerManager::FindHitMarker (int x, int y, int& data)
{
  iMarker* bestMarker = 0;
  float bestRadius = 10000000.0f;
  for (size_t i = 0 ; i < markers.GetSize () ; i++)
    if (markers[i]->IsVisible ())
    {
      iMarkerHitArea* hitArea;
      float d = markers[i]->CheckHitAreas (x, y, hitArea);
      if (d >= 0.0f && d < bestRadius)
      {
        bestRadius = d;
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

iGraphView* MarkerManager::CreateGraphView ()
{
  csRef<GraphView> gv;
  gv.AttachNew (new GraphView (this));
  graphViews.Push (gv);
  return gv;
}

void MarkerManager::DestroyGraphView (iGraphView* view)
{
  graphViews.Delete (static_cast<GraphView*> (view));
}

csPtr<iGraphNodeStyle> MarkerManager::CreateGraphNodeStyle ()
{
  GraphNodeStyle* style = new GraphNodeStyle ();
  return style;
}

csPtr<iGraphLinkStyle> MarkerManager::CreateGraphLinkStyle ()
{
  GraphLinkStyle* style = new GraphLinkStyle ();
  return style;
}

}
CS_PLUGIN_NAMESPACE_END(MarkerManager)

