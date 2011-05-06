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

#include "editmodes.h"

struct DragObject
{
  iDynamicObject* dynobj;
  csVector3 kineOffset;
};

class MainMode : public EditingMode
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
  csArray<DragObject> dragObjects;
  bool doDragRestrictY;	// Only drag on the y-plane.
  float dragRestrictY;

  /// The 'transformation' marker.
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

  bool OnItemListSelection (const CEGUI::EventArgs&);
  bool OnCategoryListSelection (const CEGUI::EventArgs&);
  bool OnDelButtonClicked (const CEGUI::EventArgs&);
  bool OnRotLeftButtonClicked (const CEGUI::EventArgs&);
  bool OnRotRightButtonClicked (const CEGUI::EventArgs&);
  bool OnRotResetButtonClicked (const CEGUI::EventArgs&);
  bool OnAlignRButtonClicked (const CEGUI::EventArgs&);
  bool OnSetPosButtonClicked (const CEGUI::EventArgs&);
  bool OnStackButtonClicked (const CEGUI::EventArgs&);
  bool OnSameYButtonClicked (const CEGUI::EventArgs&);
  bool OnStaticSelected (const CEGUI::EventArgs&);

  CEGUI::Checkbox* staticCheck;
  CEGUI::MultiColumnList* itemList;
  CEGUI::MultiColumnList* categoryList;

  /// Update the list of items based on the current category.
  void UpdateItemList ();

public:
  MainMode (AppAresEdit* aresed);
  virtual ~MainMode () { }

  virtual void Start ();
  virtual void Stop ();

  virtual void CurrentObjectsChanged (const csArray<iDynamicObject*>& current);

  virtual void FramePre();
  virtual void Frame3D();
  virtual void Frame2D();
  virtual bool OnKeyboard(iEvent& ev, utf32_char code);
  virtual bool OnMouseDown(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY);
  virtual bool OnMouseMove(iEvent& ev, int mouseX, int mouseY);

  void AddCategory (const char* category);
};

#endif // __aresed_mainmodes_h

