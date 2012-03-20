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

#include <crystalspace.h>
#include "edcommon/transformtools.h"
#include "edcommon/viewmode.h"
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/icamera.h"
#include "editor/iselection.h"
#include "inature.h"

//---------------------------------------------------------------------------

ViewMode::ViewMode (i3DView* view, iObjectRegistry* object_reg, const char* name)
  : EditingMode (view, object_reg, name)
{
  Initialize (object_reg);
}

ViewMode::ViewMode (iBase* parent) : EditingMode (this)
{
}

bool ViewMode::Initialize (iObjectRegistry* object_reg)
{
  EditingMode::Initialize (object_reg);
  font = g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_COURIER);
  nature = csQueryRegistry<iNature> (object_reg);
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  dyn = csQueryRegistry<iDynamics> (object_reg);
  colorWhite = g3d->GetDriver2D ()->FindRGB (255, 255, 255);
  return true;
}

void ViewMode::Start ()
{
  view3d->GetApplication ()->ShowCameraWindow ();
}

void ViewMode::Stop ()
{
}

void ViewMode::FramePre ()
{
  // First get elapsed time from the virtual clock.
  iCamera* camera = view3d->GetCsCamera ();

  float elapsed_time = vc->GetElapsedSeconds ();
  csTicks currentTime = view3d->GetCurrentTime ();
  nature->UpdateTime (currentTime, camera);
  if (view3d->IsAutoTime ())
    view3d->ModifyCurrentTime (csTicks (elapsed_time * 1000));

  iEditorCamera* editorCamera = view3d->GetEditorCamera ();
  editorCamera->Frame (elapsed_time, mouseX, mouseY);

  iLight* camlight = view3d->GetCameraLight ();
  csReversibleTransform tc = camera->GetTransform ();
  //csVector3 pos = tc.GetOrigin () + tc.GetT2O () * csVector3 (0, 0, .5);
  csVector3 pos = tc.GetOrigin () + tc.GetT2O () * csVector3 (2, 0, 2);
  camlight->GetMovable ()->GetTransform ().SetOrigin (pos);
  camlight->GetMovable ()->UpdateMove ();

  if (view3d->IsSimulation ())
  {
    float dynamicSpeed = 1.0f;
    dyn->Step (elapsed_time / dynamicSpeed);
  }

  view3d->GetDynamicWorld ()->PrepareView (camera, elapsed_time);
}

void ViewMode::Frame3D()
{
  iView* view = view3d->GetView ();
  view->Draw ();
  if (view3d->IsDebugMode ())
    view3d->GetBulletSystem ()->DebugDraw (view);
}

void ViewMode::Frame2D()
{
  csString buf;
  iView* view = view3d->GetView ();
  const csOrthoTransform& trans = view->GetCamera ()->GetTransform ();
  const csVector3& origin = trans.GetOrigin ();
  buf.Format ("%g,%g,%g", origin.x, origin.y, origin.z);
  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->Write (font, 200, g2d->GetHeight ()-20, colorWhite, -1, buf);
}

bool ViewMode::OnKeyboard(iEvent& ev, utf32_char code)
{
  if (code == '1')
  {
    view3d->SetDebugMode (!view3d->IsDebugMode ());
  }
  else if (code == CSKEY_F2)
  {
    view3d->ModifyCurrentTime (500);
  }
  else if (code == CSKEY_F3)
  {
    view3d->SetAutoTime (!view3d->IsAutoTime ());
  }
  else if (code == CSKEY_F4)
  {
    nature->SetFoliageDensityFactor (nature->GetFoliageDensityFactor ()-.05);
  }
  else if (code == CSKEY_F5)
  {
    nature->SetFoliageDensityFactor (nature->GetFoliageDensityFactor ()+.05);
  }
  else if (code == '.')
  {
    if (view3d->GetEditorCamera ()->IsPanningEnabled ())
    {
      view3d->GetEditorCamera ()->DisablePanning ();
    }
    else if (view3d->GetSelection ()->HasSelection ())
    {
      csVector3 center = TransformTools::GetCenterSelected (view3d->GetSelection ());
      view3d->GetEditorCamera ()->EnablePanning (center);
    }
  }
  else return false;
  return true;
}

bool ViewMode::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (view3d->GetEditorCamera ()->OnMouseDown (ev, but, mouseX, mouseY))
    return true;
  return false;
}

bool ViewMode::OnMouseUp(iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (view3d->GetEditorCamera ()->OnMouseUp (ev, but, mouseX, mouseY))
    return true;
  return false;
}

bool ViewMode::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  if (view3d->GetEditorCamera ()->OnMouseMove (ev, mouseX, mouseY))
    return true;
  return false;
}

void ViewMode::OnFocusLost ()
{
  view3d->GetEditorCamera ()->OnFocusLost ();
}

