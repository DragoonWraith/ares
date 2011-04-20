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

#include "camerawin.h"

bool CameraWindow::OnNorthButtonClicked (const CEGUI::EventArgs& e)
{
  aresed->GetCamera ().CamLookAt (csVector3 (0, 0, 0));
  return true;
}

bool CameraWindow::OnSouthButtonClicked (const CEGUI::EventArgs& e)
{
  aresed->GetCamera ().CamLookAt (csVector3 (0, M_PI, 0));
  return true;
}

bool CameraWindow::OnWestButtonClicked (const CEGUI::EventArgs& e)
{
  aresed->GetCamera ().CamLookAt (csVector3 (0, -M_PI/2, 0));
  return true;
}

bool CameraWindow::OnEastButtonClicked (const CEGUI::EventArgs& e)
{
  aresed->GetCamera ().CamLookAt (csVector3 (0, M_PI/2, 0));
  return true;
}

bool CameraWindow::OnTopDownButtonClicked (const CEGUI::EventArgs& e)
{
  aresed->GetCamera ().CamMoveAndLookAt (csVector3 (0, 200, 0), csVector3 (-M_PI/2, 0, 0));
  return true;
}

void CameraWindow::StoreTrans (int idx)
{
  transButton[idx]->enable ();
  CamLocation loc = aresed->GetCamera ().GetCameraLocation ();
  trans[idx] = loc;
}

void CameraWindow::RecallTrans (int idx)
{
  aresed->GetCamera ().SetCameraLocation (trans[idx]);
}

bool CameraWindow::OnS1ButtonClicked (const CEGUI::EventArgs& e)
{
  StoreTrans (0);
  return true;
}

bool CameraWindow::OnS2ButtonClicked (const CEGUI::EventArgs& e)
{
  StoreTrans (1);
  return true;
}

bool CameraWindow::OnS3ButtonClicked (const CEGUI::EventArgs& e)
{
  StoreTrans (2);
  return true;
}

bool CameraWindow::OnS4ButtonClicked (const CEGUI::EventArgs& e)
{
  StoreTrans (3);
  return true;
}

bool CameraWindow::OnR1ButtonClicked (const CEGUI::EventArgs& e)
{
  RecallTrans (0);
  return true;
}

bool CameraWindow::OnR2ButtonClicked (const CEGUI::EventArgs& e)
{
  RecallTrans (1);
  return true;
}

bool CameraWindow::OnR3ButtonClicked (const CEGUI::EventArgs& e)
{
  RecallTrans (2);
  return true;
}

bool CameraWindow::OnR4ButtonClicked (const CEGUI::EventArgs& e)
{
  RecallTrans (3);
  return true;
}

csVector3 CameraWindow::GetCenterSelected ()
{
  csArray<iDynamicObject*>& objects = aresed->GetCurrentObjects ();
  csVector3 center (0);
  csArray<iDynamicObject*>::Iterator it = objects.GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    const csBox3& box = dynobj->GetFactory ()->GetBBox ();
    const csReversibleTransform& tr = dynobj->GetTransform ();
    center += tr.This2Other (box.GetCenter ());
  }
  center /= objects.GetSize ();
  return center;
}

csBox3 CameraWindow::GetBoxSelected ()
{
  csArray<iDynamicObject*>& objects = aresed->GetCurrentObjects ();
  csBox3 totalbox;
  csArray<iDynamicObject*>::Iterator it = objects.GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    const csBox3& box = dynobj->GetFactory ()->GetBBox ();
    const csReversibleTransform& tr = dynobj->GetTransform ();
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (0)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (1)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (2)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (3)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (4)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (5)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (6)));
    totalbox.AddBoundingVertex (tr.This2Other (box.GetCorner (7)));
  }
  return totalbox;
}

void CameraWindow::CurrentObjectsChanged (const csArray<iDynamicObject*>& current)
{
  if (current.GetSize () == 0)
  {
    panCheck->disable();
    lookAtButton->disable();
    moveToButton->disable();
    topDownSelButton->disable();
  }
  else
  {
    panCheck->enable();
    lookAtButton->enable();
    moveToButton->enable();
    topDownSelButton->enable();
  }
}

bool CameraWindow::OnTopDownSelButtonClicked (const CEGUI::EventArgs& e)
{
  csBox3 box = GetBoxSelected ();
  float xdim = box.MaxX ()-box.MinX ();
  float zdim = box.MaxZ ()-box.MinZ ();
  csVector3 origin = box.GetCenter () + csVector3 (0, MAX(xdim,zdim), 0);
  aresed->GetCamera ().CamMoveAndLookAt (origin, csVector3 (-M_PI/2, 0, 0));
  return true;
}

bool CameraWindow::OnLookAtButtonClicked (const CEGUI::EventArgs& e)
{
  csVector3 center = GetCenterSelected ();
  aresed->GetCamera ().CamLookAtPosition (center);
  return true;
}

bool CameraWindow::OnMoveToButtonClicked (const CEGUI::EventArgs& e)
{
  csBox3 box = GetBoxSelected ();
  csVector3 center = box.GetCenter ();
  center.y = box.MaxY () + 2.0;
  aresed->GetCamera ().CamMove (center);
  return true;
}

void CameraWindow::Show ()
{
  camwin->show ();
}

void CameraWindow::Hide ()
{
  camwin->hide();
}

bool CameraWindow::IsVisible () const
{
  return camwin->isVisible();
}

bool CameraWindow::OnPanSelected (const CEGUI::EventArgs&)
{
  if (panCheck->isSelected ())
  {
    csVector3 center = GetCenterSelected ();
    aresed->GetCamera ().EnablePanning (center);
  }
  else
  {
    aresed->GetCamera ().DisablePanning ();
  }
  return true;
}

bool CameraWindow::OnGravitySelected (const CEGUI::EventArgs&)
{
  if (gravityCheck->isSelected ())
    aresed->GetCamera ().EnableGravity ();
  else
    aresed->GetCamera ().DisableGravity ();
  return true;
}

CameraWindow::CameraWindow (AppAresEdit* aresed, iCEGUI* cegui)
  : aresed (aresed), cegui (cegui)
{
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();
  camwin = winMgr->getWindow ("CameraWindow");

  transStored[0] = false;
  transStored[1] = false;
  transStored[2] = false;
  transStored[3] = false;

  CEGUI::Window* btn;

  btn = winMgr->getWindow("CameraWindow/North");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnNorthButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/South");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnSouthButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/West");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnWestButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/East");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnEastButtonClicked, this));

  btn = winMgr->getWindow("CameraWindow/StoCP1");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnS1ButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/StoCP2");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnS2ButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/StoCP3");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnS3ButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/StoCP4");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnS4ButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/RecCP1");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnR1ButtonClicked, this));
  btn->disable();
  transButton[0] = btn;
  btn = winMgr->getWindow("CameraWindow/RecCP2");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnR2ButtonClicked, this));
  btn->disable();
  transButton[1] = btn;
  btn = winMgr->getWindow("CameraWindow/RecCP3");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnR3ButtonClicked, this));
  btn->disable();
  transButton[2] = btn;
  btn = winMgr->getWindow("CameraWindow/RecCP4");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnR4ButtonClicked, this));
  btn->disable();
  transButton[3] = btn;

  btn = winMgr->getWindow("CameraWindow/TopDown");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnTopDownButtonClicked, this));
  btn = winMgr->getWindow("CameraWindow/LookAt");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnLookAtButtonClicked, this));
  lookAtButton = btn;
  btn->disable();
  btn = winMgr->getWindow("CameraWindow/MoveTo");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnMoveToButtonClicked, this));
  moveToButton = btn;
  btn->disable();
  btn = winMgr->getWindow("CameraWindow/TopDownSel");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&CameraWindow::OnTopDownSelButtonClicked, this));
  topDownSelButton = btn;
  btn->disable();

  gravityCheck = static_cast<CEGUI::Checkbox*>(winMgr->getWindow("CameraWindow/Gravity"));
  gravityCheck->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged,
    CEGUI::Event::Subscriber(&CameraWindow::OnGravitySelected, this));
  gravityCheck->setSelected(false);

  panCheck = static_cast<CEGUI::Checkbox*>(winMgr->getWindow("CameraWindow/Pan"));
  panCheck->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged,
    CEGUI::Event::Subscriber(&CameraWindow::OnPanSelected, this));
  panCheck->disable();
}

CameraWindow::~CameraWindow ()
{
}


