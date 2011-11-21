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

#ifndef __ARES_MARKER_H__
#define __ARES_MARKER_H__

#include "csutil/scf.h"

struct iView;
struct iMeshWrapper;
struct iMaterialWrapper;
class csReversibleTransform;
class csVector3;
class csBox3;
class csStringArray;

struct iEvent;
struct iMarker;
struct iMarkerHitArea;

/**
 * Where should a drawing primitive be rendered?
 */
enum MarkerSpace
{
  /**
   * The drawing primitive is rendered in camera space and this will
   * stay on the same position on screen even if the camera moves.
   */
  MARKER_CAMERA = 0,

  /**
   * The drawing primitive is rendered in object space and so will move
   * exactly with the attached object. This only works in combination
   * with iMarker->AttachMesh().
   */
  MARKER_OBJECT,

  /**
   * The drawing primitive is rendered in world space and will not
   * follow the attached object (if any).
   */
  MARKER_WORLD,

  /**
   * The drawing primitive is rendered in camera space but the
   * position if relative to the attached mesh.
   */
  MARKER_CAMERA_AT_MESH,

  /**
   * The marker is in 2D space.
   */
  MARKER_2D,
};

/**
 * A callback that is fired whenever a hit area in a marker wants to be moved.
 */
struct iMarkerCallback : public virtual iBase
{
  SCF_INTERFACE(iMarkerCallback,0,0,1);

  /**
   * Dragging has started.
   * The given position is the position in world space where the marker was hit.
   */
  virtual void StartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers) = 0;

  /**
   * The marker wants to move. The given position is the location in world
   * space coordinates where the marker wants to be. It is up to the implementation
   * of this callback to make sure the marker actually does end up there.
   */
  virtual void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos) = 0;

  /**
   * The marker wants to rotate. It is up to the implementation of
   * this callback to make sure the marker actually does end up at the new orientation.
   */
  virtual void MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform) = 0;

  /**
   * Dragging has stopped.
   */
  virtual void StopDragging (iMarker* marker, iMarkerHitArea* area) = 0;
};

#define CONSTRAIN_NONE 0
#define CONSTRAIN_XPLANE 1
#define CONSTRAIN_YPLANE 2
#define CONSTRAIN_ZPLANE 4
#define CONSTRAIN_PLANE (CONSTRAIN_XPLANE|CONSTRAIN_YPLANE|CONSTRAIN_ZPLANE)
#define CONSTRAIN_MESH 8
#define CONSTRAIN_ROTATEX 16
#define CONSTRAIN_ROTATEY 32
#define CONSTRAIN_ROTATEZ 64
#define CONSTRAIN_ROTATE (CONSTRAIN_ROTATEX|CONSTRAIN_ROTATEY|CONSTRAIN_ROTATEZ)

/**
 * A marker hit area (place on a marker that a user can select).
 */
struct iMarkerHitArea : public virtual iBase
{
  SCF_INTERFACE(iMarkerHitArea,0,0,1);

  /**
   * Define a way to drag this hit area. Using the constrain?plane parameters you
   * can control how the dragging should be restricted. If you don't constrain then
   * the position of the hit area will be equivalent to the position where the beam
   * hits the world. You can constrain at most two planes. In that case it will
   * be a line.
   * @param button is the mouse button that can initiate this dragging behaviour.
   * @param modifiers
   * @param constrainSpace this indicates in which space the dragging constraints should
   * occur. MARKER_CAMERA_AT_MESH is illegal in this context.
   * @param constainPlane is a mask with CONSTRAIN_xxx values. In case of CONSTRAIN_MESH
   *        a hitbeam is used to find out where to drag. With one of
   *        the CONSTRAIN_ROTATE modes the marker is rotated instead of moved.
   * @param cb is the callback to call when this kind of dragging is initiated.
   */
  virtual void DefineDrag (uint button, uint32 modifiers,
      MarkerSpace constrainSpace, uint32 constrainPlane,
      iMarkerCallback* cb) = 0;

  virtual int GetData () const = 0;

  virtual iMarker* GetMarker () const = 0;
  virtual MarkerSpace GetSpace () const  = 0;
  virtual const csVector3& GetCenter () const = 0;
};

#define SELECTION_NONE 0
#define SELECTION_ACTIVE 1
#define SELECTION_SELECTED 2

/**
 * Representation of a color for a marker.
 */
struct iMarkerColor : public virtual iBase
{
  SCF_INTERFACE(iMarkerColor,0,0,1);

  /**
   * Get the name.
   */
  virtual const char* GetName () const = 0;

  /**
   * Set an RGB color.
   */
  virtual void SetRGBColor (int selectionLevel, float r, float g, float b, float a) = 0;

  /**
   * Set a material.
   */
  virtual void SetMaterial (int selectionLevel, iMaterialWrapper* material) = 0;

  /**
   * Set the pen width.
   */
  virtual void SetPenWidth (int selectionLevel, float width) = 0;

  /**
   * Enable fill.
   */
  virtual void EnableFill (int selectionLevel, bool fill) = 0;
};

/**
 * A marker.
 */
struct iMarker : public virtual iBase
{
  SCF_INTERFACE(iMarker,0,0,1);

  /**
   * Set visibility of this marker.
   */
  virtual void SetVisible (bool v) = 0;

  /**
   * Get visibility status of this marker.
   */
  virtual bool IsVisible () const = 0;

  /**
   * Set the selection level. The usage of this level is up to the
   * user of this marker but typically 0 means not selected.
   */
  virtual void SetSelectionLevel (int level) = 0;

  /**
   * Get the selection level.
   */
  virtual int GetSelectionLevel () const = 0;

  /**
   * This marker will follow the given mesh.
   */
  virtual void AttachMesh (iMeshWrapper* mesh) = 0;

  /**
   * Get the attached mesh.
   */
  virtual iMeshWrapper* GetAttachedMesh () const = 0;

  /**
   * Set this marker at a specific transformation.
   * This function is not useful for primitives that are in MARKER_2D mode.
   */
  virtual void SetTransform (const csReversibleTransform& trans) = 0;

  /**
   * Get the current transform for this marker. This will be the
   * transform of the mesh it is following or else its own transform.
   * This function is not useful for primitives that are in MARKER_2D mode.
   */
  virtual const csReversibleTransform& GetTransform () const = 0;

  /**
   * Set the position of this marker. This also affects the primites
   * in MARKER_2D mode.
   */
  virtual void SetPosition (const csVector2& pos) = 0;

  /**
   * Get the position of this marker.
   */
  virtual const csVector2& GetPosition () const = 0;

  /**
   * Draw a line.
   * In MARKER_2D space the 'z' component is not used.
   */
  virtual void Line (MarkerSpace space,
      const csVector3& v1, const csVector3& v2, iMarkerColor* color,
      bool arrow = false) = 0;

  /**
   * Draw a rounded 2D box.
   * In MARKER_2D space the 'z' component is not used.
   */
  virtual void RoundedBox2D (MarkerSpace space,
      const csVector3& corner1, const csVector3& corner2,
      int roundness, iMarkerColor* color) = 0;

  /**
   * Draw text.
   * In MARKER_2D space the 'z' component is not used.
   * If centered is true the text is centered around the given position.
   */
  virtual void Text (MarkerSpace space, const csVector3& pos,
      const csStringArray& text, iMarkerColor* color, bool centered = false) = 0;

  /**
   * Clear all geometry.
   */
  virtual void Clear () = 0;

  /**
   * Define a hit area on this marker (this is a circular marker).
   */
  virtual iMarkerHitArea* HitArea (MarkerSpace space, const csVector3& center,
      float radius, int data, iMarkerColor* color) = 0;

  /**
   * Define a hit area on this marker (this version is invisible and
   * depends on other geometry in the marker to be visible).
   */
  virtual iMarkerHitArea* HitArea (MarkerSpace space, const csBox3& box,
      int data) = 0;

  /**
   * Clear all hit areas.
   */
  virtual void ClearHitAreas () = 0;
};

/**
 * A style for a given graph node.
 */
struct iGraphNodeStyle : public virtual iBase
{
  SCF_INTERFACE(iGraphNodeStyle,0,0,1);

  /**
   * Set the border color.
   */
  virtual void SetBorderColor (iMarkerColor* color) = 0;
  virtual iMarkerColor* GetBorderColor () const = 0;

  /**
   * Set the background color.
   */
  virtual void SetBackgroundColor (iMarkerColor* color) = 0;
  virtual iMarkerColor* GetBackgroundColor () const = 0;

  /**
   * Set the text color.
   */
  virtual void SetTextColor (iMarkerColor* color) = 0;
  virtual iMarkerColor* GetTextColor () const = 0;

  /**
   * Set the roundness (default 10).
   */
  virtual void SetRoundness (int roundness) = 0;
  virtual int GetRoundness () const = 0;
};

/**
 * A graph view based on markers.
 */
struct iGraphView : public virtual iBase
{
  SCF_INTERFACE(iGraphView,0,0,1);

  /**
   * Set the color scheme.
   */
  virtual void SetLinkColor (iMarkerColor* linkColor) = 0;

  /**
   * Set the default node style.
   */
  virtual void SetDefaultNodeStyle (iGraphNodeStyle* style) = 0;

  /**
   * Clear the entire graph.
   */
  virtual void Clear () = 0;

  /**
   * Make the graph visible/invisible.
   * By default graphs are created invisible.
   */
  virtual void SetVisible (bool v) = 0;

  /**
   * Returns true if this view is visible.
   */
  virtual bool IsVisible () const = 0;

  /**
   * Create a node.
   */
  virtual void CreateNode (const char* name, const char* label = 0, iGraphNodeStyle* style = 0) = 0;

  /**
   * Remove a node.
   */
  virtual void RemoveNode (const char* name) = 0;

  /**
   * Link two nodes.
   */
  virtual void LinkNode (const char* node1, const char* node2,
      iMarkerColor* color = 0, bool arrow = false) = 0;

  /**
   * Remove all links between two nodes.
   */
  virtual void RemoveLink (const char* node1, const char* node2) = 0;

  /**
   * Force the position of a node.
   */
  virtual void ForcePosition (const char* name, const csVector2& pos) = 0;

  /**
   * Find the node that contains the mouse.
   */
  virtual const char* FindHitNode (int mouseX, int mouseY) = 0;

  /**
   * Control the force factor with which nodes push each other away.
   */
  virtual void SetNodeForceFactor (float f) = 0;
  virtual float GetNodeForceFactor () const = 0;

  /**
   * Control the force factor with which links pull nodes to each other.
   */
  virtual void SetLinkForceFactor (float f) = 0;
  virtual float GetLinkForceFactor () const = 0;
};

/**
 * Interface to the marker manager.
 */
struct iMarkerManager : public virtual iBase
{
  SCF_INTERFACE(iMarkerManager,0,0,1);

  virtual void Frame2D () = 0;
  virtual void Frame3D () = 0;
  virtual bool OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY) = 0;
  virtual bool OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY) = 0;
  virtual bool OnMouseMove (iEvent& ev, int mouseX, int mouseY) = 0;

  /**
   * Set the viewport.
   */
  virtual void SetView (iView* view) = 0;

  /**
   * Set the selection level for all markers.
   */
  virtual void SetSelectionLevel (int level) = 0;

  /**
   * Check if a given marker is at this screen location.
   */
  virtual iMarker* FindHitMarker (int x, int y, int& data) = 0;

  /**
   * Create a marker color.
   */
  virtual iMarkerColor* CreateMarkerColor (const char* name) = 0;

  /**
   * Find a marker color by name.
   */
  virtual iMarkerColor* FindMarkerColor (const char* name) = 0;

  /**
   * Create a new marker.
   */
  virtual iMarker* CreateMarker () = 0;

  /**
   * Destroy a marker.
   */
  virtual void DestroyMarker (iMarker* marker) = 0;

  /**
   * Create a new graph view.
   */
  virtual iGraphView* CreateGraphView () = 0;

  /**
   * Destroy a graph view.
   */
  virtual void DestroyGraphView (iGraphView* view) = 0;

  /**
   * Create a graph node style.
   */
  virtual csPtr<iGraphNodeStyle> CreateGraphNodeStyle () = 0;
};

#endif // __ARES_MARKER_H__

