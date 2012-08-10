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

#ifndef __appares_camerawindow_h
#define __appares_camerawindow_h

#include <crystalspace.h>

#include "apparesed.h"
#include "editor/icamerawin.h"

class AresEdit3DView;
struct iDynamicObject;

class CameraWindow : public scfImplementation1<CameraWindow, iCameraWindow>
{
private:
  AresEdit3DView* aresed3d;
  wxSizer* parentSizer;

  int idLookAt, idTopDownSel, idMoveTo, idTopDown;

  CamLocation trans[3];
  void StoreTrans (int idx);
  void RecallTrans (int idx);

  void OnNorthButton ();
  void OnSouthButton ();
  void OnWestButton ();
  void OnEastButton ();

  void OnS1Button ();
  void OnR1Button ();
  void OnS2Button ();
  void OnR2Button ();
  void OnS3Button ();
  void OnR3Button ();

  void OnTopDownButton ();
  void OnLookAtButton ();
  void OnMoveToButton ();
  void OnTopDownSelButton ();

  void OnGravitySelected ();
  void OnPanSelected ();

  csBox3 GetBoxSelected ();

public:
  CameraWindow (wxWindow* parent, AresEdit3DView* aresed3d);
  virtual ~CameraWindow();

  void CurrentObjectsChanged (const csArray<iDynamicObject*>& current);

  virtual void AllocContextHandlers (wxFrame* frame);
  virtual void AddContextMenu (wxMenu* contextMenu, int mouseX, int mouseY);

  void Show () { panel->Show (); parentSizer->Layout (); }
  void Hide () { panel->Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return panel->IsShown (); }

  class Panel : public wxPanel
  {
  public:
    Panel(wxWindow* parent, CameraWindow* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}

    void OnNorthButton (wxCommandEvent& event) { s->OnNorthButton (); }
    void OnSouthButton (wxCommandEvent& event) { s->OnSouthButton (); }
    void OnWestButton (wxCommandEvent& event) { s->OnWestButton (); }
    void OnEastButton (wxCommandEvent& event) { s->OnEastButton (); }

    void OnS1Button (wxCommandEvent& event) { s->OnS1Button (); }
    void OnR1Button (wxCommandEvent& event) { s->OnR1Button (); }
    void OnS2Button (wxCommandEvent& event) { s->OnS2Button (); }
    void OnR2Button (wxCommandEvent& event) { s->OnR2Button (); }
    void OnS3Button (wxCommandEvent& event) { s->OnS3Button (); }
    void OnR3Button (wxCommandEvent& event) { s->OnR3Button (); }

    void OnTopDownButton (wxCommandEvent& event) { s->OnTopDownButton (); }
    void OnLookAtButton (wxCommandEvent& event) { s->OnLookAtButton (); }
    void OnMoveToButton (wxCommandEvent& event) { s->OnMoveToButton (); }
    void OnTopDownSelButton (wxCommandEvent& event) { s->OnTopDownSelButton (); }

    void OnGravitySelected (wxCommandEvent& event) { s->OnGravitySelected (); }
    void OnPanSelected (wxCommandEvent& event) { s->OnPanSelected (); }

  private:
    CameraWindow* s;

    DECLARE_EVENT_TABLE()
  };
  Panel* panel;
};

#endif // __appares_camerawindow_h

