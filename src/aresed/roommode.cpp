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

#include "apparesed.h"
#include "roommode.h"

//---------------------------------------------------------------------------

RoomMode::RoomMode (AppAresEdit* aresed, AresEdit3DView* aresed3d)
  : EditingMode (aresed, aresed3d, "Room")
{
  editingRoomFactory = 0;

#if 0
  CEGUI::WindowManager* winMgr = aresed->GetCEGUI ()->GetWindowManagerPtr ();
  CEGUI::Window* btn;
#endif

  //btn = winMgr->getWindow("Ares/CurveWindow/RotLeft");
  //btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    //CEGUI::Event::Subscriber(&CurveMode::OnRotLeftButtonClicked, this));
  //btn = winMgr->getWindow("Ares/CurveWindow/RotRight");
  //btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    //CEGUI::Event::Subscriber(&CurveMode::OnRotRightButtonClicked, this));
  //btn = winMgr->getWindow("Ares/CurveWindow/RotReset");
  //btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    //CEGUI::Event::Subscriber(&CurveMode::OnRotResetButtonClicked, this));

  //btn = winMgr->getWindow("Ares/CurveWindow/Flatten");
  //btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    //CEGUI::Event::Subscriber(&CurveMode::OnFlattenButtonClicked, this));

  //autoSmoothCheck = static_cast<CEGUI::Checkbox*>(winMgr->getWindow("Ares/CurveWindow/AutoSmooth"));
  //autoSmoothCheck->setSelected (autoSmooth);
  //autoSmoothCheck->subscribeEvent(CEGUI::Checkbox::EventCheckStateChanged,
    //CEGUI::Event::Subscriber(&CurveMode::OnAutoSmoothSelected, this));
}

void RoomMode::Start ()
{
}

void RoomMode::Stop ()
{
}

void RoomMode::MarkerStartDragging (iMarker* marker, iMarkerHitArea* area,
    const csVector3& pos, uint button, uint32 modifiers)
{
}

void RoomMode::MarkerWantsMove (iMarker* marker, iMarkerHitArea* area,
      const csVector3& pos)
{
}

void RoomMode::MarkerStopDragging (iMarker* marker, iMarkerHitArea* area)
{
}

void RoomMode::FramePre()
{
}

void RoomMode::Frame3D()
{
}

void RoomMode::Frame2D()
{
}

bool RoomMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  return false;
}

bool RoomMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

bool RoomMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

bool RoomMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  return false;
}

