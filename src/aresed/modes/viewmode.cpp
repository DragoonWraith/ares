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

#include "../apparesed.h"
#include "../camerawin.h"
#include "../transformtools.h"
#include "viewmode.h"

//---------------------------------------------------------------------------

ViewMode::ViewMode (AresEdit3DView* aresed3d, const char* name)
  : EditingMode (aresed3d, name)
{
}

void ViewMode::Start ()
{
  aresed3d->GetApp ()->GetCameraWindow ()->Show ();
}

void ViewMode::Stop ()
{
}

void ViewMode::FramePre()
{
  aresed3d->Do3DPreFrameStuff ();
}

void ViewMode::Frame3D()
{
  iView* view = aresed3d->GetView ();
  view->Draw ();
  if (aresed3d->IsDebugMode ())
    aresed3d->GetBulletSystem ()->DebugDraw (view);
}

void ViewMode::Frame2D()
{
  csString buf;
  iView* view = aresed3d->GetView ();
  const csOrthoTransform& trans = view->GetCamera ()->GetTransform ();
  const csVector3& origin = trans.GetOrigin ();
  buf.Format ("%g,%g,%g", origin.x, origin.y, origin.z);
  aresed3d->WriteText (buf);
}

bool ViewMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  if (code == '1')
  {
    aresed3d->SetDebugMode (!aresed3d->IsDebugMode ());
  }
  else if (code == CSKEY_F2)
  {
    aresed3d->ModifyCurrentTime (500);
  }
  else if (code == CSKEY_F3)
  {
    aresed3d->SetAutoTime (!aresed3d->IsAutoTime ());
  }
  else if (code == CSKEY_F4)
  {
    iNature* nature = aresed3d->GetNature ();
    nature->SetFoliageDensityFactor (nature->GetFoliageDensityFactor ()-.05);
  }
  else if (code == CSKEY_F5)
  {
    iNature* nature = aresed3d->GetNature ();
    nature->SetFoliageDensityFactor (nature->GetFoliageDensityFactor ()+.05);
  }
  else if (code == '.')
  {
    if (aresed3d->GetCamera ().IsPanningEnabled ())
    {
      aresed3d->GetCamera ().DisablePanning ();
    }
    else if (aresed3d->GetSelection ()->HasSelection ())
    {
      csVector3 center = TransformTools::GetCenterSelected (aresed3d->GetSelection ());
      aresed3d->GetCamera ().EnablePanning (center);
    }
  }
  else return false;
  return true;
}

bool ViewMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (aresed3d->GetCamera ().OnMouseDown (ev, but, mouseX, mouseY))
    return true;
  return false;
}

bool ViewMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (aresed3d->GetCamera ().OnMouseUp (ev, but, mouseX, mouseY))
    return true;
  return false;
}

bool ViewMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  if (aresed3d->GetCamera ().OnMouseMove (ev, mouseX, mouseY))
    return true;
  return false;
}

void ViewMode::OnFocusLost ()
{
  aresed3d->GetCamera ().OnFocusLost ();
}

