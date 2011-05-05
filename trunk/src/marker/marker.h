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
#include "iengine/engine.h"
#include "iutil/virtclk.h"
#include "iutil/comp.h"

#include "include/imarker.h"

class csPen;

CS_PLUGIN_NAMESPACE_BEGIN(MarkerManager)
{

class MarkerManager;

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
};

class Marker : public scfImplementation1<Marker, iMarker>
{
private:
  MarkerManager* mgr;
  iMeshWrapper* attachedMesh;
  csReversibleTransform trans;
  int selectionLevel;

  csArray<MarkerLine> lines;

public:
  Marker (MarkerManager* mgr) :
    scfImplementationType (this), mgr (mgr), attachedMesh (0), selectionLevel (0) { }
  virtual ~Marker () { }

  virtual void AddMarkerCallback (iMarkerCallback* cb) { }
  virtual void RemoveMarkerCallback (iMarkerCallback* cb) { }
  virtual void SetSelectionLevel (int level) { selectionLevel = level; }
  virtual int GetSelectionLevel () const { return selectionLevel; }
  virtual void AttachMesh (iMeshWrapper* mesh) { attachedMesh = mesh; }
  virtual iMeshWrapper* GetAttachedMesh () const { return attachedMesh; }
  virtual void SetTransform (const csReversibleTransform& trans)
  {
    Marker::trans = trans;
  }
  virtual const csReversibleTransform& GetTransform () const;
  virtual void Line (MarkerSpace space,
      const csVector3& v1, const csVector3& v2, iMarkerColor* color);
  virtual void Poly2D (MarkerSpace space,
      const csVector3& v1, const csVector3& v2,
      const csVector3& v3, iMarkerColor* color) { }
  virtual void Box3D (MarkerSpace space,
      const csBox3& box, iMarkerColor* color) { }
  virtual void Clear ();

  void Render2D ();
  void Render3D ();
};

class MarkerManager : public scfImplementation2<MarkerManager, iMarkerManager, iComponent>
{
public:
  iObjectRegistry *object_reg;
  csRef<iEngine> engine;
  csRef<iVirtualClock> vc;
  csRef<iGraphics3D> g3d;
  csRef<iGraphics2D> g2d;
  iCamera* camera;

  csRefArray<Marker> markers;
  csRefArray<MarkerColor> markerColors;

public:
  MarkerManager (iBase *iParent);
  virtual ~MarkerManager ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  virtual void Frame2D ();
  virtual void Frame3D ();

  virtual void SetCamera (iCamera* camera) { MarkerManager::camera = camera; }
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
};

}
CS_PLUGIN_NAMESPACE_END(MarkerManager)

#endif // __ARES_MARKER_IMP_H__
