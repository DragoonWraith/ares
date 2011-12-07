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

/**
 * A mesh viewer.
 */
class MeshView : public wxEvtHandler
{
private:
  iObjectRegistry* object_reg;
  csRef<iEngine> engine;
  csRef<iMeshWrapper> mesh;
  wxBitmapButton* button;
  csMeshOnTexture* meshOnTexture;
  csRef<iMaterialWrapper> material;
  iTextureWrapper* txt;
  iTextureHandle* handle;

  iSector* FindSuitableSector (int& num);
  void RemoveMesh ();

public:
  MeshView (iObjectRegistry* object_reg, wxWindow* parent);
  ~MeshView ();

  /**
   * Set mesh. Returns false on failure (not reported).
   * This will render the mesh on the texture but not update the
   * button yet. This will happen later when Render() is called but
   * this should not be done in the same frame.
   */
  bool SetMesh (const char* name);

  /**
   * Render the texture on the button. This should be called in the next
   * frame after calling SetMesh().
   */
  void Render ();
};

#endif // __appares_meshview_h

