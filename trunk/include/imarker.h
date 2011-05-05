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

struct iCamera;
struct iMeshWrapper;
struct iMaterial;
class csReversibleTransform;
class csVector3;
class csBox3;

struct iMarker;

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
};

/**
 * When a marker is clicked you can get a callback.
 */
struct iMarkerCallback : public virtual iBase
{
  SCF_INTERFACE(iMarkerCallback,0,0,1);

  virtual void MarkerSelect (iMarker* marker) = 0;
};

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
  MARKER_CAMERA_AT_MESH
};

/**
 * A marker.
 */
struct iMarker : public virtual iBase
{
  SCF_INTERFACE(iMarker,0,0,1);

  /**
   * Add a marker selection callback.
   */
  virtual void AddMarkerCallback (iMarkerCallback* cb) = 0;

  /**
   * Remove a marker selection callback.
   */
  virtual void RemoveMarkerCallback (iMarkerCallback* cb) = 0;

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
   */
  virtual void SetTransform (const csReversibleTransform& trans) = 0;

  /**
   * Get the current transform for this marker. This will be the
   * transform of the mesh it is following or else its own transform.
   */
  virtual const csReversibleTransform& GetTransform () const = 0;

  /**
   * Draw a line.
   */
  virtual void Line (MarkerSpace space,
      const csVector3& v1, const csVector3& v2, iMarkerColor* color) = 0;

  /**
   * Draw a 2D 4-sided polygon.
   */
  virtual void Poly2D (MarkerSpace space,
      const csVector3& v1, const csVector3& v2,
      const csVector3& v3, iMarkerColor* color) = 0;

  /**
   * Draw a 3D box.
   */
  virtual void Box3D (MarkerSpace space,
      const csBox3& box, iMarkerColor* color) = 0;

  /**
   * Clear all geometry.
   */
  virtual void Clear () = 0;
};

/**
 * Interface to the marker manager.
 */
struct iMarkerManager : public virtual iBase
{
  SCF_INTERFACE(iMarkerManager,0,0,1);

  virtual void Frame2D () = 0;
  virtual void Frame3D () = 0;

  /**
   * Set the camera.
   */
  virtual void SetCamera (iCamera* camera) = 0;

  /**
   * Set the selection level for all markers.
   */
  virtual void SetSelectionLevel (int level) = 0;

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
};

#endif // __ARES_MARKER_H__
