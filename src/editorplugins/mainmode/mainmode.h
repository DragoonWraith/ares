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
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

class TreeCtrlView;

struct AresDragObject
{
  iDynamicObject* dynobj;
  csVector3 kineOffset;
  csReversibleTransform originalTransform;
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

  bool active;	// Main mode is active.

  // If we are busy changing the 3D selection from the object list then we set this flag
  // so that we don't sync back to the object list.
  int changing3DSelection;

  bool do_kinematic_dragging;
  bool kinematicFirstOnly;
  csArray<AresDragObject> dragObjects;
  bool doDragRestrictX;	// Only drag on the x-plane.
  bool doDragRestrictY;	// Only drag on the y-plane.
  bool doDragRestrictZ;	// Only drag on the z-plane.
  csVector3 dragRestrict;

  int idSetStatic, idClearStatic;

  void CreateMarkers ();
  iMarker* transformationMarker;
  void SetTransformationMarkerStatus ();

  void StartKinematicDragging (bool restrictY,
      const csSegment3& beam, const csVector3& isect, bool firstOnly);
  void StartPhysicalDragging (iRigidBody* hitBody,
      const csSegment3& beam, const csVector3& isect);
  // If 'cancel' == True the objects will be restored to the original position.
  // Only for kinematic dragging!
  void StopDrag (bool cancel = false);
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

  virtual csRef<iString> GetStatusLine ();

  virtual void Refresh ();

  virtual void AllocContextHandlers (wxFrame* frame);
  virtual void AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY);
  virtual bool IsContextMenuAllowed ()
  {
    return !(do_dragging || do_kinematic_dragging);
  }

  virtual void CurrentObjectsChanged (const csArray<iDynamicObject*>& current);

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);

  void OnSnapObjects ();
  void OnStaticSelected ();
  void OnObjectNameEntered ();
  void OnTreeSelChanged (wxTreeEvent& event);
  void OnListSelChanged (wxListEvent& event);
  void OnListContext (wxListEvent& event);

  void OnSetStatic ();
  void OnClearStatic ();

  void SetDynObjOrigin (iDynamicObject* dynobj, const csVector3& pos);
  void SetDynObjTransform (iDynamicObject* dynobj, const csReversibleTransform& trans);

  void MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos, uint button, uint32 modifiers);
  void MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos);
  void MarkerWantsRotate (iMarker* marker, iMarkerHitArea* area,
      const csReversibleTransform& transform);
  void MarkerStopDragging (iMarker* marker, iMarkerHitArea* area);

  virtual bool Command (const char* name, const char* args);
  virtual bool IsCommandValid (const char* name, const char* args,
      iSelection* selection, bool haspaste, const char* currentmode);

  class Panel : public wxPanel
  {
  public:
    Panel(wxWindow* parent, MainMode* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}

    void OnStaticSelected (wxCommandEvent& event) { s->OnStaticSelected (); }
    void OnObjectNameEntered (wxCommandEvent& event) { s->OnObjectNameEntered (); }
    void OnTreeSelChanged (wxTreeEvent& event) { s->OnTreeSelChanged (event); }
    void OnListSelChanged (wxListEvent& event) { s->OnListSelChanged (event); }
    void OnListContext (wxListEvent& event) { s->OnListContext (event); }

    void OnSetStatic (wxCommandEvent& event) { s->OnSetStatic (); }
    void OnClearStatic (wxCommandEvent& event) { s->OnClearStatic (); }

  private:
    MainMode* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __aresed_mainmodes_h

