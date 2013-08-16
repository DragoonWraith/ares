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
#include "edcommon/transformtools.h"
#include "ui/uimanager.h"
#include "apparesed.h"

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

  EVT_BUTTON (XRCID("topDownButton"), CameraWindow::Panel::OnTopDownButton)
  EVT_BUTTON (XRCID("lookButton"), CameraWindow::Panel::OnLookAtButton)
  EVT_BUTTON (XRCID("moveButton"), CameraWindow::Panel::OnMoveToButton)

  EVT_CHECKBOX (XRCID("gravityCheckBox"), CameraWindow::Panel::OnGravitySelected)
  EVT_CHECKBOX (XRCID("panCheckBox"), CameraWindow::Panel::OnPanSelected)
END_EVENT_TABLE()

//---------------------------------------------------------------------------

void CameraWindow::OnNorthButton ()
{
  aresed3d->GetCamera ()->CamLookAt (csVector3 (0, 0, 0));
}

void CameraWindow::OnSouthButton ()
{
  aresed3d->GetCamera ()->CamLookAt (csVector3 (0, PI, 0));
}

void CameraWindow::OnWestButton ()
{
  aresed3d->GetCamera ()->CamLookAt (csVector3 (0, -PI/2, 0));
}

void CameraWindow::OnEastButton ()
{
  aresed3d->GetCamera ()->CamLookAt (csVector3 (0, PI/2, 0));
}

void CameraWindow::OnTopDownButton ()
{
  aresed3d->GetCamera ()->CamMoveAndLookAt (csVector3 (0, 200, 0), csVector3 (-PI/2, 0, 0));
}

void CameraWindow::StoreTrans (int idx)
{
  wxButton* recallButton;
  switch (idx)
  {
    case 0: recallButton = XRCCTRL (*panel, "recall1Button", wxButton); break;
    case 1: recallButton = XRCCTRL (*panel, "recall2Button", wxButton); break;
    case 2: recallButton = XRCCTRL (*panel, "recall3Button", wxButton); break;
  }
  recallButton->Enable ();
  locationStored[idx] = true;
  CamLocation loc = aresed3d->GetCamera ()->GetCameraLocation ();
  trans[idx] = loc;
}

void CameraWindow::RecallTrans (int idx)
{
  aresed3d->GetCamera ()->SetCameraLocation (trans[idx]);
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

void CameraWindow::CurrentObjectsChanged (const csArray<iDynamicObject*>& current)
{
  wxCheckBox* panCheck = XRCCTRL (*panel, "panCheckBox", wxCheckBox);
  wxButton* lookAtButton = XRCCTRL (*panel, "lookButton", wxButton);
  wxButton* moveToButton = XRCCTRL (*panel, "moveButton", wxButton);
  if (current.GetSize () == 0)
  {
    lookAtButton->Disable();
    moveToButton->Disable();
  }
  else
  {
    lookAtButton->Enable();
    moveToButton->Enable();
  }

  if (panCheck->IsChecked ())
  {
    if (current.GetSize () > 0)
    {
      csVector3 center = TransformTools::GetCenterSelected (aresed3d->GetSelection ());
      aresed3d->GetCamera ()->EnablePanning (center);
    }
  }
}

void CameraWindow::OnTopDownSelButton ()
{
  csBox3 box = TransformTools::GetBoxSelected (aresed3d->GetSelection ());
  float xdim = box.MaxX ()-box.MinX ();
  float zdim = box.MaxZ ()-box.MinZ ();
  csVector3 origin = box.GetCenter () + csVector3 (0, csMax(xdim,zdim), 0);
  aresed3d->GetCamera ()->CamMoveAndLookAt (origin, csVector3 (-PI/2, 0, 0));
}

void CameraWindow::OnLookAtButton ()
{
  if (aresed3d->GetSelection ()->HasSelection ())
  {
    csVector3 center = TransformTools::GetCenterSelected (aresed3d->GetSelection ());
    aresed3d->GetCamera ()->CamLookAtPosition (center);
    aresed3d->GetApp ()->SetFocus3D ();
  }
}

void CameraWindow::MoveToSelection ()
{
  if (aresed3d->GetSelection ()->HasSelection ())
  {
    Camera* cam = aresed3d->GetCamera ();
    csSphere sphere = TransformTools::GetSphereSelected (aresed3d->GetSelection ());
    CamLocation loc = cam->GetCameraLocation ();

    const csVector3& src = loc.pos;
    const csVector3& dst = sphere.GetCenter ();

    cam->CamLookAtPosition (dst);
    cam->CamMove (dst - (dst-src).Unit () * sphere.GetRadius () * 2);
    aresed3d->GetApp ()->SetFocus3D ();
  }
}

void CameraWindow::OnMoveToButton ()
{
  MoveToSelection ();
}

void CameraWindow::OnPanSelected ()
{
  wxCheckBox* panCheck = XRCCTRL (*panel, "panCheckBox", wxCheckBox);
  if (panCheck->IsChecked ())
  {
    if (aresed3d->GetSelection ()->HasSelection ())
    {
      csVector3 center = TransformTools::GetCenterSelected (aresed3d->GetSelection ());
      aresed3d->GetCamera ()->EnablePanning (center);
    }
  }
  else
  {
    aresed3d->GetCamera ()->DisablePanning ();
  }
}

void CameraWindow::OnGravitySelected ()
{
  wxCheckBox* gravityCheck = XRCCTRL (*panel, "gravityCheckBox", wxCheckBox);
  if (gravityCheck->IsChecked ())
    aresed3d->GetCamera ()->EnableGravity ();
  else
    aresed3d->GetCamera ()->DisableGravity ();
}

CameraWindow::CameraWindow (wxWindow* parent, AresEdit3DView* aresed3d)
  : scfImplementationType (this), aresed3d (aresed3d)
{
  panel = new Panel (this);
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

  locationStored[0] = false;
  locationStored[1] = false;
  locationStored[2] = false;
  trans[0].pos.Set (0, 0, 0);
  trans[1].pos.Set (0, 0, 0);
  trans[2].pos.Set (0, 0, 0);
}

CameraWindow::~CameraWindow ()
{
}

void CameraWindow::AllocContextHandlers (wxFrame* frame)
{
  UIManager* ui = aresed3d->GetApp ()->GetUIManager ();

  idLookAt = ui->AllocContextMenuID ();
  frame->Connect (idLookAt, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnLookAtButton), 0, panel);
  idTopDownSel = ui->AllocContextMenuID ();
  frame->Connect (idTopDownSel, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnTopDownSelButton), 0, panel);
  idMoveTo = ui->AllocContextMenuID ();
  frame->Connect (idMoveTo, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnMoveToButton), 0, panel);
  idTopDown = ui->AllocContextMenuID ();
  frame->Connect (idTopDown, wxEVT_COMMAND_MENU_SELECTED,
	wxCommandEventHandler (CameraWindow::Panel::OnTopDownButton), 0, panel);
}

void CameraWindow::AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY)
{
  contextMenu->AppendSeparator ();

  if (aresed3d->GetSelection ()->HasSelection ())
  {
    contextMenu->Append (idLookAt, wxT ("Look at selection"));
    contextMenu->Append (idTopDownSel, wxT ("Top of selection"));
    contextMenu->Append (idMoveTo, wxT ("Move to selection"));
  }
  contextMenu->Append (idTopDown, wxT ("Top of the world"));
}

