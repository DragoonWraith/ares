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

struct iEngine;
struct iSector;
struct iMeshWrapper;
class ImagePanel;

struct MVBox
{
  csBox3 box;
  int color;
};

/**
 * A mesh viewer.
 */
class MeshView : public wxEvtHandler
{
private:
  iObjectRegistry* object_reg;
  csRef<iEngine> engine;
  csRef<iGraphics3D> g3d;
  csRef<iMeshWrapper> mesh;
  ImagePanel* imagePanel;
  csMeshOnTexture* meshOnTexture;
  csRef<iTextureHandle> handle;

  csArray<MVBox> boxes;

  iSector* FindSuitableSector (int& num);
  void RemoveMesh ();
  void UpdateImageButton ();
  void Render2D ();

public:
  MeshView (iObjectRegistry* object_reg, wxWindow* parent);
  ~MeshView ();

  /**
   * Set mesh. Returns false on failure (not reported).
   */
  bool SetMesh (const char* name);

  /**
   * Add a box to show in 2D on top of the mesh.
   */
  void AddBox (const csBox3& box, int r, int g, int b);

  /**
   * Clear boxes.
   */
  void ClearBoxes () { boxes.DeleteAll (); }

  /**
   * Rotate the mesh.
   */
  void RotateMesh (float seconds);
};

#endif // __appares_meshview_h

