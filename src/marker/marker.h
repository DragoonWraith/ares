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

#ifndef __ARES_MARKER_IMP_H__
#define __ARES_MARKER_IMP_H__

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "csutil/parray.h"
#include "csutil/hash.h"
#include "iengine/engine.h"
#include "iutil/virtclk.h"
#include "iutil/comp.h"
#include "ivaria/view.h"

#include "include/imarker.h"

class csPen;
struct iFont;

CS_PLUGIN_NAMESPACE_BEGIN(MarkerManager)
{

class Marker;
class MarkerManager;

#define CSMASK_MODIFIERS (CSMASK_SHIFT + CSMASK_CTRL + CSMASK_ALT)

class MarkerColor : public scfImplementation1<MarkerColor, iMarkerColor>
{
private:
  MarkerManager* mgr;
  csString name;
  csPDelArray<csPen> pens;

  csPen* GetOrCreatePen (int level);

public:
  MarkerColor (MarkerManager* mgr, const char* name):
    scfImplementationType (this), mgr (mgr), name (name) { }
  virtual ~MarkerColor () { }

  virtual const char* GetName () const { return name; }
  virtual const csString& GetNameString () const { return name; }
  virtual void SetRGBColor (int selectionLevel, float r, float g, float b, float a);
  virtual void SetMaterial (int selectionLevel, iMaterialWrapper* material) { }
  virtual void SetPenWidth (int selectionLevel, float width);

  csPen* GetPen (int level) const
  {
    if (size_t (level) >= pens.GetSize ())
      level = pens.GetSize () - 1;
    return pens[level];
  }
};

struct MarkerLine
{
  MarkerSpace space;
  csVector3 v1, v2;
  MarkerColor* color;
  bool arrow;
};

struct MarkerRoundedBox
{
  MarkerSpace space;
  csVector3 v1, v2;
  int roundness;
  MarkerColor* color;
};

struct MarkerText
{
  MarkerSpace space;
  csVector3 pos;
  csString text;
  MarkerColor* color;
  bool centered;
};

struct MarkerDraggingMode
{
  csRef<iMarkerCallback> cb;
  uint button;
  uint32 modifiers;
  MarkerSpace constrainSpace;
  uint32 constrainPlane;
};

class MarkerHitArea : public scfImplementation1<MarkerHitArea, iMarkerHitArea>
{
private:
  Marker* marker;
  MarkerSpace space;
  csVector3 center;
  float radius;
  int data;
  csPDelArray<MarkerDraggingMode> draggingModes;
  MarkerColor* color;

public:
  MarkerHitArea (Marker* marker) : scfImplementationType (this), marker (marker) { }
  virtual ~MarkerHitArea () { }

  Marker* GetMarker () const { return marker; }
  csVector3 GetWorldCenter () const;
  void SetColor (MarkerColor* color) { MarkerHitArea::color = color; }
  MarkerColor* GetColor () const { return color; }

  virtual void DefineDrag (uint button, uint32 modifiers,
      MarkerSpace constrainSpace, uint32 constrainPlane,
      iMarkerCallback* cb);

  MarkerDraggingMode* FindDraggingMode (uint button, uint32 modifiers) const;

  void SetSpace (MarkerSpace space) { MarkerHitArea::space = space; }
  MarkerSpace GetSpace () const { return space; }

  void SetCenter (const csVector3& center) { MarkerHitArea::center = center; }
  const csVector3& GetCenter () const { return center; }

  void SetRadius (float radius) { MarkerHitArea::radius = radius; }
  float GetRadius () const { return radius; }
  csVector2 GetPerspectiveRadius (iView* view, float z) const;

  void SetData (int data) { MarkerHitArea::data = data; }
  virtual int GetData () const { return data; }
};

class Marker : public scfImplementation1<Marker, iMarker>
{
private:
  MarkerManager* mgr;
  iMeshWrapper* attachedMesh;
  csReversibleTransform trans;
  csVector2 pos;
  int selectionLevel;

  csArray<MarkerLine> lines;
  csArray<MarkerRoundedBox> roundedBoxes;
  csArray<MarkerText> texts;
  csRefArray<MarkerHitArea> hitAreas;

  bool visible;

public:
  Marker (MarkerManager* mgr) :
    scfImplementationType (this), mgr (mgr), attachedMesh (0), pos (0, 0),
    selectionLevel (0), visible (true)
  { }
  virtual ~Marker () { }

  MarkerManager* GetMarkerManager () const { return mgr; }

  virtual void SetVisible (bool v) { visible = v; }
  virtual bool IsVisible () const { return visible; }

  virtual void SetSelectionLevel (int level) { selectionLevel = level; }
  virtual int GetSelectionLevel () const { return selectionLevel; }
  virtual void AttachMesh (iMeshWrapper* mesh) { attachedMesh = mesh; }
  virtual iMeshWrapper* GetAttachedMesh () const { return attachedMesh; }
  virtual void SetTransform (const csReversibleTransform& trans)
  {
    Marker::trans = trans;
  }
  virtual const csReversibleTransform& GetTransform () const;
  virtual void SetPosition (const csVector2& pos)
  {
    Marker::pos = pos;
  }
  virtual const csVector2& GetPosition () const { return pos; }
  virtual void Line (MarkerSpace space,
      const csVector3& v1, const csVector3& v2, iMarkerColor* color,
      bool arrow = false);
  virtual void Poly2D (MarkerSpace space,
      const csVector3& v1, const csVector3& v2,
      const csVector3& v3, iMarkerColor* color) { }
  virtual void Box3D (MarkerSpace space,
      const csBox3& box, iMarkerColor* color) { }
  virtual void RoundedBox2D (MarkerSpace space,
      const csVector3& corner1, const csVector3& corner2,
      int roundness, iMarkerColor* color);
  virtual void Text (MarkerSpace space, const csVector3& pos,
      const char* text, iMarkerColor* color, bool centered = false);
  virtual void Clear ();
  virtual iMarkerHitArea* HitArea (MarkerSpace space, const csVector3& center,
      float radius, int data, iMarkerColor* color);
  virtual void ClearHitAreas ();

  void Render2D ();
  void Render3D ();

  /**
   * Check if a hit area was hit and return the distance.
   * Returns negative number if there was no hit.
   */
  float CheckHitAreas (int x, int y, MarkerHitArea*& bestHitArea);
};

struct GraphLink
{
  csString node1;
  csString node2;
};

class GraphView : public scfImplementation1<GraphView, iGraphView>
{
private:
  MarkerManager* mgr;
  bool visible;

  csHash<iMarker*,csString> nodes;
  csHash<GraphLink,csString> links;

public:
  GraphView (MarkerManager* mgr) : scfImplementationType (this), mgr (mgr), visible (false) { }
  virtual ~GraphView () { Clear(); }

  void UpdateFrame ();

  virtual void SetVisible (bool v);
  virtual bool IsVisible () const { return visible; }
  virtual void Clear ();
  virtual void CreateNode (const char* name);
  virtual void RemoveNode (const char* name);
  virtual void LinkNode (const char* name, const char* node1, const char* node2)
  {
    GraphLink l;
    l.node1 = node1;
    l.node2 = node2;
    links.Put (name, l);
  }
  virtual void RemoveLink (const char* name)
  {
    links.DeleteAll (name);
  }
  virtual void RemoveLinks (const char* node1, const char* node2)
  {
    csHash<GraphLink,csString>::GlobalIterator it = links.GetIterator ();
    while (it.HasNext ())
    {
      GraphLink l = it.NextNoAdvance ();
      if ((l.node1 == node1 && l.node2 == node2)
	  || (l.node1 == node2 && l.node2 == node1))
        links.DeleteElement (it);
      else
        it.Next ();
    }
  }
};

class MarkerManager : public scfImplementation2<MarkerManager, iMarkerManager, iComponent>
{
public:
  iObjectRegistry *object_reg;
  csRef<iEngine> engine;
  csRef<iVirtualClock> vc;
  csRef<iGraphics3D> g3d;
  csRef<iGraphics2D> g2d;
  csRef<iFont> font;
  iView* view;
  iCamera* camera;

  int mouseX, mouseY;

  csRefArray<Marker> markers;
  csRefArray<MarkerColor> markerColors;
  csRefArray<GraphView> graphViews;

  MarkerHitArea* currentDraggingHitArea;
  MarkerDraggingMode* currentDraggingMode;
  float dragDistance;
  csVector3 dragRestrict;

  void StopDrag ();
  void HandleDrag ();

  /**
   * Calculate the planar intersection of a beam given the current
   * dragging mode. The intersection point is returned in 'newpos'.
   * This function returns false if there was some kind of error.
   */
  bool FindPlanarIntersection (const csVector3& start, const csVector3& end,
      csVector3& newpos);

  /**
   * Handle planar rotation to be used in dragging mode.
   * Returns false if rotation cannot occur due to some error.
   * The new rotation matrix is returned in 'm'.
   * @param newpos is the position where we want the rotation to look at.
   */
  bool HandlePlanarRotation (const csVector3& newpos, csMatrix3& m);

public:
  MarkerManager (iBase *iParent);
  virtual ~MarkerManager ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  int GetMouseX () const { return mouseX; }
  int GetMouseY () const { return mouseY; }

  iFont* GetFont () const { return font; }

  virtual void Frame2D ();
  virtual void Frame3D ();
  virtual bool OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove (iEvent& ev, int mouseX, int mouseY);

  virtual void SetView (iView* view);
  virtual void SetSelectionLevel (int level);
  virtual iMarkerColor* CreateMarkerColor (const char* name)
  {
    MarkerColor* c = new MarkerColor (this, name);
    markerColors.Push (c);
    c->DecRef ();
    return c;
  }
  virtual iMarkerColor* FindMarkerColor (const char* name);
  virtual iMarker* CreateMarker ()
  {
    Marker* m = new Marker (this);
    markers.Push (m);
    m->DecRef ();
    return m;
  }
  virtual void DestroyMarker (iMarker* marker)
  {
    markers.Delete (static_cast<Marker*> (marker));
  }
  virtual iMarker* FindHitMarker (int x, int y, int& data);

  virtual iGraphView* CreateGraphView ();
  virtual void DestroyGraphView (iGraphView* view);

  MarkerHitArea* FindHitArea (int x, int y);
};

}
CS_PLUGIN_NAMESPACE_END(MarkerManager)

#endif // __ARES_MARKER_IMP_H__

