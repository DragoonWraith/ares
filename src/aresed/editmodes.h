/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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

#ifndef __aresed_editmodes_h
#define __aresed_editmodes_h

#include "csutil/csstring.h"
#include "imarker.h"
#include "propclass/dynworld.h"

class AresEdit3DView;

class EditingMode;

class MarkerCallback : public scfImplementation1<MarkerCallback,iMarkerCallback>
{
private:
  EditingMode* editmode;

public:
  MarkerCallback (EditingMode* editmode) : scfImplementationType (this),
    editmode (editmode) { }
  virtual ~MarkerCallback () { }
  virtual void StartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers);
  virtual void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos);
  virtual void MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform);
  virtual void StopDragging (iMarker* marker, iMarkerHitArea* area);
};

class EditingMode
{
protected:
  AresEdit3DView* aresed3d;
  csString name;

public:
  EditingMode (AresEdit3DView* aresed3d, const char* name) :
    aresed3d (aresed3d), name (name) { }
  virtual ~EditingMode () { }

  virtual void Start () { }
  virtual void Stop () { }

  virtual void CurrentObjectsChanged (const csArray<iDynamicObject*>& current) { }

  const char* GetName () const { return name; }
  virtual void FramePre() { }
  virtual void Frame3D() { }
  virtual void Frame2D() { }
  virtual bool OnKeyboard(iEvent& ev, utf32_char code) { return false; }
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY)  { return false; }
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)  { return false; }
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY)  { return false; }
  virtual void OnFocusLost () { }

  virtual void MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers) { }
  virtual void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos) { }
  virtual void MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform) { }
  virtual void MarkerStopDragging (iMarker* marker, iMarkerHitArea* area) { }
};

#endif // __aresed_editmodes_h
