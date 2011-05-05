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
#include "editmodes.h"

struct iGeometryGenerator;

struct DragPoint
{
  size_t idx;
  csVector3 kineOffset;
};

class CurveMode : public EditingMode
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

  bool do_dragging;
  csArray<DragPoint> dragPoints;
  float dragDistance;
  bool doDragRestrictY;	// Only drag on the y-plane.
  bool doDragMesh;	// Drag on the mesh.
  float dragRestrictY;
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
  bool OnRotLeftButtonClicked (const CEGUI::EventArgs&);
  bool OnRotRightButtonClicked (const CEGUI::EventArgs&);
  bool OnRotResetButtonClicked (const CEGUI::EventArgs&);
  bool OnFlattenButtonClicked (const CEGUI::EventArgs&);
  bool OnAutoSmoothSelected (const CEGUI::EventArgs&);

  bool autoSmooth;
  CEGUI::Checkbox* autoSmoothCheck;
  void DoAutoSmooth ();

  void UpdateMarkers ();
  void UpdateMarkerSelection ();

public:
  CurveMode (AppAresEdit* aresed);
  virtual ~CurveMode () { }

  virtual void Start ();
  virtual void Stop ();

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);
};

#endif // __aresed_curvemodes_h

