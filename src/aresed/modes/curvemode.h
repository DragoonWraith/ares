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

#ifndef __aresed_curvemodes_h
#define __aresed_curvemodes_h

#include "csutil/csstring.h"
#include "edcommon/viewmode.h"

struct iGeometryGenerator;

struct DragPoint
{
  size_t idx;
  csVector3 kineOffset;
};

class CurveMode : public ViewMode
{
private:
  iCurvedFactory* editingCurveFactory;
  csRef<iGeometryGenerator> ggen;

  /**
   * Find the curve point closest to the mouse position.
   * Return csArrayItemNotFound if none found.
   */
  size_t FindCurvePoint (int mouseX, int mouseY);

  csArray<iMarker*> markers;

  csArray<size_t> selectedPoints;
  void SetCurrentPoint (size_t idx);
  void AddCurrentPoint (size_t idx);

  csArray<DragPoint> dragPoints;
  void StopDrag ();

  /// Get the world position of a given point on the curve.
  csVector3 GetWorldPosPoint (size_t idx);

  /**
   * Smooth the front/up/right vectors for a given point so that the
   * curve follows a smooth path.
   */
  void SmoothPoint (size_t idx, bool regen = true);

  /**
   * Flatten this point.
   */
  void FlatPoint (size_t idx);

  void RotateCurrent (float baseAngle);

  void OnRotLeft ();
  void OnRotRight ();
  void OnRotReset ();
  void OnFlatten ();
  void OnAutoSmoothSelected ();

  bool autoSmooth;
  void DoAutoSmooth ();

  void UpdateMarkers ();
  void UpdateMarkerSelection ();

public:
  CurveMode (wxWindow* parent, i3DView* view, iObjectRegistry* object_reg);
  virtual ~CurveMode () { }

  virtual void Start ();
  virtual void Stop ();

  virtual void FramePre ();
  virtual void Frame3D ();
  virtual void Frame2D ();
  virtual bool OnKeyboard (iEvent& ev, utf32_char code);
  virtual bool OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove (iEvent& ev, int mouseX, int mouseY);

  virtual void MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers);
  virtual void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos);
  virtual void MarkerStopDragging (iMarker* marker, iMarkerHitArea* area);

  class Panel : public wxPanel
  {
  public:
    Panel(wxWindow* parent, CurveMode* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}

    void OnRotLeft (wxCommandEvent& event) { s->OnRotLeft (); }
    void OnRotRight (wxCommandEvent& event) { s->OnRotRight (); }
    void OnRotReset (wxCommandEvent& event) { s->OnRotReset (); }
    void OnFlatten (wxCommandEvent& event) { s->OnFlatten (); }
    void OnAutoSmoothSelected (wxCommandEvent& event) { s->OnAutoSmoothSelected (); }

  private:
    CurveMode* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __aresed_curvemodes_h

