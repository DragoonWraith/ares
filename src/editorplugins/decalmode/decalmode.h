/*
The MIT License

Copyright (c) 2013 by Jorrit Tyberghein

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

#ifndef __aresed_decalmode_h
#define __aresed_decalmode_h

#include "csutil/csstring.h"
#include "edcommon/viewmode.h"

struct iMeshGenerator;

class DecalMode : public scfImplementationExt1<DecalMode, ViewMode, iComponent>
{
private:
  iMeshGenerator* meshgen;
  csRef<iNature> nature;

  /// Update the list of types.
  void UpdateTypeList ();

public:
  DecalMode (iBase* parent);
  virtual ~DecalMode () { }

  virtual bool Initialize (iObjectRegistry* object_reg);
  virtual void SetParent (wxWindow* parent);

  virtual void Start ();
  virtual void Stop ();

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);

  virtual void MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers);
  virtual void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos);
  virtual void MarkerStopDragging (iMarker* marker, iMarkerHitArea* area);

  class Panel : public wxPanel
  {
  public:
    Panel (wxWindow* parent, DecalMode* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}

  private:
    DecalMode* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __aresed_decalmode_h

