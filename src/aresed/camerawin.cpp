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
#include "transformtools.h"

#include <wx/xrc/xmlres.h>

//---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CameraWindow::Panel, wxPanel)
  EVT_BUTTON (XRCID("northButton"), CameraWindow::Panel::OnNorthButton)
  EVT_BUTTON (XRCID("southButton"), CameraWindow::Panel::OnSouthButton)
  EVT_BUTTON (XRCID("westButton"), CameraWindow::Panel::OnWestButton)
  EVT_BUTTON (XRCID("eastButton"), CameraWindow::Panel::OnEastButton)

  EVT_BUTTON (XRCID("store1Button"), CameraWindow::Panel::OnS1Button)
  EVT_BUTTON (XRCID("recall1Button"), CameraWindow::Panel::OnR1Button)
  EVT_BUTTON (XRCID("store2Button"), CameraWindow::Panel::OnS2Button)
  EVT_BUTTON (XRCID("recall2Button"), CameraWindow::Panel::OnR2Button)
  EVT_BUTTON (XRCID("store3Button"), CameraWindow::Panel::OnS3Button)
  EVT_BUTTON (XRCID("recall3Button"), CameraWindow::Panel::OnR3Button)
  EVT_BUTTON (XRCID("store4Button"), CameraWindow::Panel::OnS4Button)
  EVT_BUTTON (XRCID("recall4Button"), CameraWindow::Panel::OnR4Button)

  EVT_BUTTON (XRCID("topDownButton"), CameraWindow::Panel::OnTopDownButton)
  EVT_BUTTON (XRCID("lookButton"), CameraWindow::Panel::OnLookAtButton)
  EVT_BUTTON (XRCID("moveButton"), CameraWindow::Panel::OnMoveToButton)
  EVT_BUTTON (XRCID("topButton"), CameraWindow::Panel::OnTopDownSelButton)

  EVT_CHECKBOX (XRCID("gravityCheckBox"), CameraWindow::Panel::OnGravitySelected)
  EVT_CHECKBOX (XRCID("panCheckBox"), CameraWindow::Panel::OnPanSelected)
END_EVENT_TABLE()

//---------------------------------------------------------------------------

void CameraWindow::OnNorthButton ()
{
  aresed3d->GetCamera ().CamLookAt (csVector3 (0, 0, 0));
}

void CameraWindow::OnSouthButton ()
{
  aresed3d->GetCamera ().CamLookAt (csVector3 (0, PI, 0));
}

void CameraWindow::OnWestButton ()
{
  aresed3d->GetCamera ().CamLookAt (csVector3 (0, -PI/2, 0));
}

void CameraWindow::OnEastButton ()
{
  aresed3d->GetCamera ().CamLookAt (csVector3 (0, PI/2, 0));
}

void CameraWindow::OnTopDownButton ()
{
  aresed3d->GetCamera ().CamMoveAndLookAt (csVector3 (0, 200, 0), csVector3 (-PI/2, 0, 0));
}

void CameraWindow::StoreTrans (int idx)
{
  wxButton* recallButton;
  switch (idx)
  {
    case 0: recallButton = XRCCTRL (*panel, "recall1Button", wxButton); break;
    case 1: recallButton = XRCCTRL (*panel, "recall2Button", wxButton); break;
    case 2: recallButton = XRCCTRL (*panel, "recall3Button", wxButton); break;
    case 3: recallButton = XRCCTRL (*panel, "recall4Button", wxButton); break;
  }
  recallButton->Enable ();
  CamLocation loc = aresed3d->GetCamera ().GetCameraLocation ();
  trans[idx] = loc;
}

void CameraWindow::RecallTrans (int idx)
{
  aresed3d->GetCamera ().SetCameraLocation (trans[idx]);
}

void CameraWindow::OnS1Button ()
{
  StoreTrans (0);
}

void CameraWindow::OnS2Button ()
{
  StoreTrans (1);
}

void CameraWindow::OnS3Button ()
{
  StoreTrans (2);
}

void CameraWindow::OnS4Button ()
{
  StoreTrans (3);
}

void CameraWindow::OnR1Button ()
{
  RecallTrans (0);
}

void CameraWindow::OnR2Button ()
{
  RecallTrans (1);
}

void CameraWindow::OnR3Button ()
{
  RecallTrans (2);
}

void CameraWindow::OnR4Button ()
{
  RecallTrans (3);
}

csBox3 CameraWindow::GetBoxSelected ()
{
  csBox3 totalbox;
  SelectionIterator it = aresed3d->GetSelection ()->GetIterator ();
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
  wxCheckBox* panCheck = XRCCTRL (*panel, "panCheckBox", wxCheckBox);
  wxButton* lookAtButton = XRCCTRL (*panel, "lookButton", wxButton);
  wxButton* moveToButton = XRCCTRL (*panel, "moveButton", wxButton);
  wxButton* topDownSelButton = XRCCTRL (*panel, "topButton", wxButton);
  if (current.GetSize () == 0)
  {
    panCheck->Disable();
    lookAtButton->Disable();
    moveToButton->Disable();
    topDownSelButton->Disable();
  }
  else
  {
    panCheck->Enable();
    lookAtButton->Enable();
    moveToButton->Enable();
    topDownSelButton->Enable();
  }
}

void CameraWindow::OnTopDownSelButton ()
{
  csBox3 box = GetBoxSelected ();
  float xdim = box.MaxX ()-box.MinX ();
  float zdim = box.MaxZ ()-box.MinZ ();
  csVector3 origin = box.GetCenter () + csVector3 (0, MAX(xdim,zdim), 0);
  aresed3d->GetCamera ().CamMoveAndLookAt (origin, csVector3 (-PI/2, 0, 0));
}

void CameraWindow::OnLookAtButton ()
{
  if (aresed3d->GetSelection ()->HasSelection ())
  {
    csVector3 center = TransformTools::GetCenterSelected (aresed3d->GetSelection ());
    aresed3d->GetCamera ().CamLookAtPosition (center);
  }
}

void CameraWindow::OnMoveToButton ()
{
  csBox3 box = GetBoxSelected ();
  csVector3 center = box.GetCenter ();
  center.y = box.MaxY () + 2.0;
  aresed3d->GetCamera ().CamMove (center);
}

void CameraWindow::OnPanSelected ()
{
  wxCheckBox* panCheck = XRCCTRL (*panel, "panCheckBox", wxCheckBox);
  if (panCheck->IsChecked ())
  {
    if (aresed3d->GetSelection ()->HasSelection ())
    {
      csVector3 center = TransformTools::GetCenterSelected (aresed3d->GetSelection ());
      aresed3d->GetCamera ().EnablePanning (center);
    }
  }
  else
  {
    aresed3d->GetCamera ().DisablePanning ();
  }
}

void CameraWindow::OnGravitySelected ()
{
  wxCheckBox* gravityCheck = XRCCTRL (*panel, "gravityCheckBox", wxCheckBox);
  if (gravityCheck->IsChecked ())
    aresed3d->GetCamera ().EnableGravity ();
  else
    aresed3d->GetCamera ().DisableGravity ();
}

CameraWindow::CameraWindow (wxWindow* parent, AresEdit3DView* aresed3d)
  : aresed3d (aresed3d)
{
  panel = new Panel (parent, this);
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (panel, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (panel, parent, wxT ("CameraPanel"));

  wxButton* recallButton;
  recallButton  = XRCCTRL (*panel, "recall1Button", wxButton);
  recallButton->Disable ();
  recallButton  = XRCCTRL (*panel, "recall2Button", wxButton);
  recallButton->Disable ();
  recallButton  = XRCCTRL (*panel, "recall3Button", wxButton);
  recallButton->Disable ();
  recallButton  = XRCCTRL (*panel, "recall4Button", wxButton);
  recallButton->Disable ();
}

CameraWindow::~CameraWindow ()
{
}

void CameraWindow::AddContextMenu (wxFrame* frame, wxMenu* contextMenu, int& id)
{
  contextMenu->AppendSeparator ();

  if (aresed3d->GetSelection ()->HasSelection ())
  {
    contextMenu->Append (id, wxT ("Look at selection"));
    frame->Connect (id, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnLookAtButton), 0, panel);
    id++;
    contextMenu->Append (id, wxT ("Top of selection"));
    frame->Connect (id, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnTopDownSelButton), 0, panel);
    id++;
    contextMenu->Append (id, wxT ("Move to selection"));
    frame->Connect (id, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnMoveToButton), 0, panel);
    id++;
  }
  contextMenu->Append (id, wxT ("Top of the world"));
  frame->Connect (id, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnTopDownButton), 0, panel);
  id++;
}

void CameraWindow::ReleaseContextMenu (wxFrame* frame)
{
  frame->Disconnect (wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnLookAtButton), 0, panel);
  frame->Disconnect (wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnTopDownSelButton), 0, panel);
  frame->Disconnect (wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnMoveToButton), 0, panel);
  frame->Disconnect (wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnTopDownButton), 0, panel);
}

