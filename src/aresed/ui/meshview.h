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

#ifndef __appares_meshview_h
#define __appares_meshview_h

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/xrc/xmlres.h>

#include "../models/rowmodel.h"
#include "customcontrol.h"

struct iEngine;
struct iSector;
struct iMeshWrapper;
class ImagePanel;

struct MVSphere
{
  csVector3 center;
  float radius;
  size_t penIdx;
};

/**
 * A mesh viewer.
 */
class MeshView : public CustomControl
{
private:
  iObjectRegistry* object_reg;
  csRef<iEngine> engine;
  csRef<iGraphics3D> g3d;
  csRef<iMeshWrapper> mesh;
  csString meshName;
  ImagePanel* imagePanel;
  csMeshOnTexture* meshOnTexture;
  csRef<iTextureHandle> handle;
  float reldist;

  csPenCache penCache;
  csArray<MVSphere> spheres;
  csPDelArray<csPen> pens;
  csPDelArray<csPen3D> pens3d;

  iSector* FindSuitableSector (int& num);
  void RemoveMesh ();
  void UpdateImageButton ();
  void RenderGeometry ();
  void RenderSpheres (const csOrthoTransform& camtrans,
      const csReversibleTransform& meshtrans);

  void Cleanup ();

  void OnMouseWheel (wxMouseEvent& event);

public:
  MeshView (iObjectRegistry* object_reg, wxWindow* parent);
  virtual ~MeshView ();

  virtual void SyncValue (Ares::Value* value)
  {
    if (!meshName.IsEmpty ())
      SetMesh (meshName);
  }

  /**
   * Set mesh. Returns false on failure (not reported).
   */
  bool SetMesh (const char* name);

  /// Get the current mesh name.
  const csString& GetMeshName () const { return meshName; }

  /**
   * Create a pen and return the index.
   */
  size_t CreatePen (float r, float g, float b, float width);

  /**
   * Add a box to show in 2D on top of the mesh.
   */
  void AddBox (const csBox3& box, size_t penIdx);

  /**
   * Add a line to show in 2D on top of the mesh.
   */
  void AddLine (const csVector3& v1, const csVector3& v2, size_t penIdx);

  /**
   * Add a sphere to show in 2D on top of the mesh.
   */
  void AddSphere (const csVector3& center, float radius, size_t penIdx);

  /**
   * Add a cylinder to show in 2D on top of the mesh.
   */
  void AddCylinder (const csVector3& center, float radius, float length,
      size_t penIdx);

  /**
   * Add a mesh (linedraw) to show in 2D on top of the real mesh.
   */
  void AddMesh (const csVector3& center, size_t penIdx);

  /**
   * Clear geometry.
   */
  void ClearGeometry ();

  /**
   * Rotate the mesh.
   */
  void RotateMesh (float seconds);

  /**
   * Change the relative distance between the camera and the mesh.
   * Default is 1.0f.
   */
  void ChangeRelativeDistance (float d);
};

#endif // __appares_meshview_h

