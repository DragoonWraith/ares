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
#include "csutil/randomgen.h"
#include "csutil/hash.h"
#include "csutil/stringarray.h"
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
  virtual void EnableFill (int selectionLevel, bool fill);

  csPen* GetPen (int level) const
  {
    if (size_t (level) >= pens.GetSize ())
      level = pens.GetSize () - 1;
    return pens[level];
  }
};

struct MarkerPrimitive
{
  virtual void Render2D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel) { }
  virtual void Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel) { }
};

struct MarkerLine : public MarkerPrimitive
{
  MarkerSpace space;
  csVector3 vec1, vec2;
  MarkerColor* color;
  bool arrow;

  virtual ~MarkerLine () { }
  virtual void Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel);
};

struct MarkerRoundedBox : public MarkerPrimitive
{
  MarkerSpace space;
  csVector3 vec1, vec2;
  int roundness;
  MarkerColor* color;

  virtual ~MarkerRoundedBox () { }
  virtual void Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel);
};

struct MarkerText : public MarkerPrimitive
{
  MarkerSpace space;
  csVector3 position;
  csStringArray text;
  MarkerColor* color;
  bool centered;
  iFont* font;

  MarkerText () : font (0) { }
  virtual ~MarkerText () { }
  virtual void Render2D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos, int selectionLevel);
};

struct MarkerDraggingMode
{
  csRef<iMarkerCallback> cb;
  uint button;
  uint32 modifiers;
  MarkerSpace constrainSpace;
  uint32 constrainPlane;
};

class InternalMarkerHitArea : public scfImplementation1<InternalMarkerHitArea,
  iMarkerHitArea>
{
protected:
  Marker* marker;
  MarkerSpace space;
  int data;
  csPDelArray<MarkerDraggingMode> draggingModes;

public:
  InternalMarkerHitArea (Marker* marker) : scfImplementationType (this),
      marker (marker) { }
  virtual ~InternalMarkerHitArea () { }
  virtual void Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos) = 0;
  virtual float CheckHit (int x, int y, const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos) = 0;

  virtual void DefineDrag (uint button, uint32 modifiers,
      MarkerSpace constrainSpace, uint32 constrainPlane,
      iMarkerCallback* cb);
  MarkerDraggingMode* FindDraggingMode (uint button, uint32 modifiers) const;

  /// Return true if mouse-over on this hit area should light the entire marker.
  virtual bool HitAreaHiLightsMarker () const = 0;

  virtual iMarker* GetMarker () const;

  void SetSpace (MarkerSpace space) { InternalMarkerHitArea::space = space; }
  virtual MarkerSpace GetSpace () const { return space; }

  void SetData (int data) { InternalMarkerHitArea::data = data; }
  virtual int GetData () const { return data; }
};

class InvBoxMarkerHitArea : public InternalMarkerHitArea
{
private:
  csBox3 box;
  csVector3 center;

public:
  InvBoxMarkerHitArea (Marker* marker) : InternalMarkerHitArea (marker) { }
  virtual ~InvBoxMarkerHitArea () { }

  void SetBox (const csBox3& box)
  {
    InvBoxMarkerHitArea::box = box;
    center = box.GetCenter ();
  }
  virtual const csVector3& GetCenter () const { return center; }

  // For InternalMarkerHitArea
  virtual void Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos);
  virtual float CheckHit (int x, int y, const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos);
  virtual bool HitAreaHiLightsMarker () const { return true; }
};

class CircleMarkerHitArea : public InternalMarkerHitArea
{
private:
  csVector3 center;
  float radius;
  MarkerColor* color;

public:
  CircleMarkerHitArea (Marker* marker) : InternalMarkerHitArea (marker) { }
  virtual ~CircleMarkerHitArea () { }

  void SetColor (MarkerColor* color) { CircleMarkerHitArea::color = color; }
  MarkerColor* GetColor () const { return color; }

  void SetCenter (const csVector3& center) { CircleMarkerHitArea::center = center; }
  virtual const csVector3& GetCenter () const { return center; }

  void SetRadius (float radius) { CircleMarkerHitArea::radius = radius; }
  float GetRadius () const { return radius; }
  csVector2 GetPerspectiveRadius (iView* view, float z) const;

  // For InternalMarkerHitArea
  virtual void Render3D (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos);
  virtual float CheckHit (int x, int y, const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans, MarkerManager* mgr,
      const csVector2& pos);
  virtual bool HitAreaHiLightsMarker () const { return false; }
};

class Marker : public scfImplementation1<Marker, iMarker>
{
private:
  MarkerManager* mgr;
  iMeshWrapper* attachedMesh;
  csReversibleTransform trans;
  csVector2 pos;
  int selectionLevel;

  csPDelArray<MarkerPrimitive> primitives;
  csRefArray<iMarkerHitArea> hitAreas;

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
  virtual void RoundedBox2D (MarkerSpace space,
      const csVector3& corner1, const csVector3& corner2,
      int roundness, iMarkerColor* color);
  virtual void Text (MarkerSpace space, const csVector3& pos,
      const csStringArray& text, iMarkerColor* color, bool centered = false,
      iFont* font = 0);
  virtual void Clear ();
  virtual iMarkerHitArea* HitArea (MarkerSpace space, const csVector3& center,
      float radius, int data, iMarkerColor* color);
  virtual iMarkerHitArea* HitArea (MarkerSpace space, const csBox3& box,
      int data);
  virtual void ClearHitAreas ();

  void Render2D ();
  void Render3D ();

  /**
   * Check if a hit area was hit and return the distance.
   * Returns negative number if there was no hit.
   */
  float CheckHitAreas (int x, int y, iMarkerHitArea*& bestHitArea);
};

struct SubNode
{
  csString name;
  iMarker* marker;
  csVector2 relpos;	// Relative position (relative to parent node).
  csVector2 size;
  bool maybeDelete;	// Used in smart refresh mode.
  SubNode () : marker (0), maybeDelete (false) { }
};

struct GraphNode
{
  csString name;
  iMarker* marker;
  csVector2 velocity, netForce;
  bool frozen;
  csVector2 size;
  float weightFactor;
  bool maybeDelete;	// Used in smart refresh mode.
  csPDelArray<SubNode> subnodes;
  GraphNode () : marker (0), frozen (false), weightFactor (1.0f), maybeDelete (false) { }
};

struct GraphLink
{
  csString node1;
  csString node2;
  iMarkerColor* color;
  bool arrow;
  float strength;
  bool maybeDelete;	// Used in smart refresh mode.
  GraphLink () : color (0), arrow (false), strength (1.0f), maybeDelete (false) { }
};

class GraphView : public scfImplementation1<GraphView, iGraphView>
{
private:
  MarkerManager* mgr;
  bool visible;
  bool smartRefresh;

  csHash<GraphNode*,csString> nodes;
  csArray<GraphLink> links;
  iMarker* draggingMarker;
  iMarker* activeMarker;
  csRefArray<iGraphNodeCallback> callbacks;

  csRandomGen rng;
  bool coolDownPeriod;

  // Force with which nodes push each other away.
  float nodeForceFactor;
  // Force with which links pull the two nodes together.
  float linkForceFactor;

  csRef<iGraphNodeStyle> defaultStyle;
  csRef<iGraphLinkStyle> defaultLinkStyle;

  bool IsLinked (const char* n1, const char* n2);

  csString currentNode;	// String as returned to the caller in FindHitNode().

  void HandlePushingForces ();
  void HandlePullingLinks ();
  // Update the velocities of all nodes and move them according to those
  // velocities. Returns true if the simulation appears cool enough (not
  // a lot of movement).
  bool MoveNodes (float seconds);
  void UpdateSubNodePositions (GraphNode* node);

  float secondsTodo;

  void UpdateNodeMarker (iMarker* marker, const char* label,
      iGraphNodeStyle* style, int& w, int& h);

public:
  GraphView (MarkerManager* mgr);
  virtual ~GraphView () { Clear(); }

  void UpdateFrame ();
  void Render3D ();

  void ActivateMarker (iMarker* marker, const char* node = 0);

  void SetDraggingMarker (iMarker* marker) { draggingMarker = marker; }

  virtual void SetDefaultLinkStyle (iGraphLinkStyle* style)
  {
    defaultLinkStyle = style;
  }
  virtual void SetDefaultNodeStyle (iGraphNodeStyle* style)
  {
    defaultStyle = style;
  }

  virtual void ActivateNode (const char* node);
  virtual const char* GetActiveNode () const;
  virtual void SetVisible (bool v);
  virtual bool IsVisible () const { return visible; }
  virtual void Clear ();
  virtual void StartRefresh ();
  virtual void FinishRefresh ();
  virtual bool NodeExists (const char* nodeName) const;
  virtual void CreateSubNode (const char* parentNode, const char* name, const char* label = 0,
      iGraphNodeStyle* style = 0);
  virtual void CreateNode (const char* name, const char* label = 0,
      iGraphNodeStyle* style = 0);
  virtual void RemoveNode (const char* name);
  virtual void ChangeNode (const char* name, const char* label, iGraphNodeStyle* style);
  virtual void ChangeSubNode (const char* parentNode, const char* name,
      const char* label, iGraphNodeStyle* style);
  virtual void ReplaceNode (const char* oldNode, const char* newNode,
      const char* label = 0, iGraphNodeStyle* style = 0);
  virtual void LinkNode (const char* node1, const char* node2,
      iGraphLinkStyle* style = 0);
  virtual void RemoveLink (const char* node1, const char* node2)
  {
    size_t i = 0;
    while (i < links.GetSize ())
    {
      GraphLink& l = links[i];
      if ((l.node1 == node1 && l.node2 == node2)
	  || (l.node1 == node2 && l.node2 == node1))
        links.DeleteIndex (i);
      else
        i++;
    }
  }
  virtual void ForcePosition (const char* name, const csVector2& pos);
  virtual const char* FindHitNode (int mouseX, int mouseY);
  virtual void SetNodeForceFactor (float f) { nodeForceFactor = f; }
  virtual float GetNodeForceFactor () const { return nodeForceFactor; }
  virtual void SetLinkForceFactor (float f) { linkForceFactor = f; }
  virtual float GetLinkForceFactor () const { return linkForceFactor; }
  virtual void AddNodeActivationCallback (iGraphNodeCallback* cb);
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

  iMarkerHitArea* currentDraggingHitArea;
  MarkerDraggingMode* currentDraggingMode;
  float dragDistance;
  csVector3 dragRestrict;
  csVector2 dragOffset;

  void StopDrag ();
  void HandleDrag ();
  iMarker* GetDraggingMarker ();

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

  virtual void SetDefaultFont (iFont* font) { MarkerManager::font = font; }
  iFont* GetFont () const { return font; }
  iGraphics2D* GetG2D () const { return g2d; }
  iVirtualClock* GetVC () const { return vc; }

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
  virtual csPtr<iGraphNodeStyle> CreateGraphNodeStyle ();
  virtual csPtr<iGraphLinkStyle> CreateGraphLinkStyle ();

  iMarkerHitArea* FindHitArea (int x, int y);
};

}
CS_PLUGIN_NAMESPACE_END(MarkerManager)

#endif // __ARES_MARKER_IMP_H__

