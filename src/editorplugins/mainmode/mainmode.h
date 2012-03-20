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

#ifndef __aresed_mainmodes_h
#define __aresed_mainmodes_h

#include "csutil/scfstr.h"
#include "edcommon/viewmode.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

class TreeCtrlView;

struct AresDragObject
{
  iDynamicObject* dynobj;
  csVector3 kineOffset;
};

class MainMode : public scfImplementationExt1<MainMode, ViewMode, iComponent>
{
private:
  // Dragging related
  bool do_dragging;
  csRef<CS::Physics::Bullet::iPivotJoint> dragJoint;
  csRef<CS::Physics::Bullet::iPivotJoint> holdJoint;
  float linearDampening, angularDampening;
  float dragDistance;

  bool do_static_dragging;
  bool do_kinematic_dragging;
  bool kinematicFirstOnly;
  csArray<AresDragObject> dragObjects;
  bool doDragRestrictY;	// Only drag on the y-plane.
  float dragRestrictY;

  int idSetStatic, idClearStatic;

  void CreateMarkers ();
  iMarker* transformationMarker;

  void StartKinematicDragging (bool restrictY,
      const csSegment3& beam, const csVector3& isect, bool firstOnly);
  void StartPhysicalDragging (iRigidBody* hitBody,
      const csSegment3& beam, const csVector3& isect);
  void StopDrag ();
  void HandleKinematicDragging ();
  void HandlePhysicalDragging ();

  void AddForce (iRigidBody* hitBody, bool pull,
      const csSegment3& beam, const csVector3& isect);

  csString GetSelectedItem ();

public:
  MainMode (iBase* parent);
  virtual ~MainMode ();

  virtual bool Initialize (iObjectRegistry* object_reg);
  virtual void SetParent (wxWindow* parent);

  virtual void Start ();
  virtual void Stop ();

  virtual csRef<iString> GetStatusLine ()
  {
    csRef<iString> str = ViewMode::GetStatusLine ();
    str->Append (", LMB: select objects (shift to add to selection)");
    return str;
  }

  virtual void Refresh ();

  virtual void AllocContextHandlers (wxFrame* frame);
  virtual void AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY);

  virtual void CurrentObjectsChanged (const csArray<iDynamicObject*>& current);

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);

  void OnRotLeft ();
  void OnRotRight ();
  void OnRotReset ();
  void OnAlignR ();
  void OnSetPos ();
  void OnStack ();
  void OnSameY ();
  void OnSnapObjects ();
  void OnStaticSelected ();
  void OnObjectNameEntered ();
  void OnTreeSelChanged (wxTreeEvent& event);

  void OnSetStatic ();
  void OnClearStatic ();

  void MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers);
  void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos);
  void MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform);
  void MarkerStopDragging (iMarker* marker, iMarkerHitArea* area);

  class Panel : public wxPanel
  {
  public:
    Panel(wxWindow* parent, MainMode* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}

    void OnRotLeft (wxCommandEvent& event) { s->OnRotLeft (); }
    void OnRotRight (wxCommandEvent& event) { s->OnRotRight (); }
    void OnRotReset (wxCommandEvent& event) { s->OnRotReset (); }
    void OnAlignR (wxCommandEvent& event) { s->OnAlignR (); }
    void OnSetPos (wxCommandEvent& event) { s->OnSetPos (); }
    void OnStack (wxCommandEvent& event) { s->OnStack (); }
    void OnSameY (wxCommandEvent& event) { s->OnSameY (); }
    void OnSnapObjects (wxCommandEvent& event) { s->OnSnapObjects (); }
    void OnStaticSelected (wxCommandEvent& event) { s->OnStaticSelected (); }
    void OnObjectNameEntered (wxCommandEvent& event) { s->OnObjectNameEntered (); }
    void OnTreeSelChanged (wxTreeEvent& event) { s->OnTreeSelChanged (event); }

    void OnSetStatic (wxCommandEvent& event) { s->OnSetStatic (); }
    void OnClearStatic (wxCommandEvent& event) { s->OnClearStatic (); }

  private:
    MainMode* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __aresed_mainmodes_h

