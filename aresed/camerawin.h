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

#include <CEGUI.h>
#include <crystalspace.h>
#include <ivaria/icegui.h>

#include "apparesed.h"

class AppAresEdit;
struct iDynamicObject;

class CameraWindow
{
private:
  AppAresEdit* aresed;
  csRef<iCEGUI> cegui;
  CEGUI::Window* camwin;

  CamLocation trans[4];
  bool transStored[4];
  CEGUI::Window* transButton[4];
  void StoreTrans (int idx);
  void RecallTrans (int idx);

  bool OnNorthButtonClicked (const CEGUI::EventArgs& e);
  bool OnSouthButtonClicked (const CEGUI::EventArgs& e);
  bool OnWestButtonClicked (const CEGUI::EventArgs& e);
  bool OnEastButtonClicked (const CEGUI::EventArgs& e);

  bool OnS1ButtonClicked (const CEGUI::EventArgs& e);
  bool OnR1ButtonClicked (const CEGUI::EventArgs& e);
  bool OnS2ButtonClicked (const CEGUI::EventArgs& e);
  bool OnR2ButtonClicked (const CEGUI::EventArgs& e);
  bool OnS3ButtonClicked (const CEGUI::EventArgs& e);
  bool OnR3ButtonClicked (const CEGUI::EventArgs& e);
  bool OnS4ButtonClicked (const CEGUI::EventArgs& e);
  bool OnR4ButtonClicked (const CEGUI::EventArgs& e);

  bool OnTopDownButtonClicked (const CEGUI::EventArgs& e);
  bool OnLookAtButtonClicked (const CEGUI::EventArgs& e);
  bool OnMoveToButtonClicked (const CEGUI::EventArgs& e);
  bool OnTopDownSelButtonClicked (const CEGUI::EventArgs& e);

  CEGUI::Window* lookAtButton;
  CEGUI::Window* moveToButton;
  CEGUI::Window* topDownSelButton;

  bool OnGravitySelected (const CEGUI::EventArgs&);
  CEGUI::Checkbox* gravityCheck;

  bool OnPanSelected (const CEGUI::EventArgs&);
  CEGUI::Checkbox* panCheck;

  csVector3 GetCenterSelected ();
  csBox3 GetBoxSelected ();

public:
  CameraWindow (AppAresEdit* aresed, iCEGUI* cegui);
  ~CameraWindow();

  void Show ();
  void Hide ();
  bool IsVisible () const;

  void CurrentObjectsChanged (const csArray<iDynamicObject*>& current);
};

#endif // __appares_camerawindow_h

