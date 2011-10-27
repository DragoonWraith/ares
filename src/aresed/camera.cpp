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
#include "camera.h"

//---------------------------------------------------------------------------

Camera::Camera (AppAresEdit* aresed, AresEdit3DView* aresed3d) :
  aresed (aresed), aresed3d (aresed3d)
{
  do_panning = false;
  do_mouse_panning = false;
  do_mouse_dragging = false;
  do_gravity = false;
  camera = 0;
}

void Camera::EnablePanning (const csVector3& center)
{
  DisableGravity ();
  do_panning = true;
  panningCenter = center;
  CamLookAtPosition (center);
  panningDistance = sqrt (csSquaredDist::PointPoint (center, desired.pos));
}

void Camera::DisablePanning ()
{
  do_panning = false;
  // @@@ Bug! In case camera window is open it should be notified!
}

// Convenience function to get a csVector3 from a config file.
static csVector3 GetConfigVector (iConfigFile* config,
        const char* key, const char* def)
{
  csVector3 v;
  csScanStr (config->GetStr (key, def), "%f,%f,%f", &v.x, &v.y, &v.z);
  return v;
}

void Camera::Init (iCamera* camera, iSector* sector, const csVector3& pos)
{
  Camera::camera = camera;

  // Now we need to position the camera in our world.
  camera->SetSector (sector);
  camera->GetTransform ().SetOrigin (pos);
  current.pos = pos;
  desired = current;

  // Initialize our collider actor.
  collider_actor.SetCollideSystem (aresed3d->GetCollisionSystem ());
  collider_actor.SetEngine (aresed3d->GetEngine ());
  collider_actor.SetGravity (0);

  // Creates an accessor for configuration settings.
  csConfigAccess cfgAcc (aresed3d->GetObjectRegistry ());

  // Read the values from config files, and use default values if not set.
  csVector3 legs (GetConfigVector (cfgAcc, "Actor.Legs", "0.2,0.5,0.2"));
  csVector3 body (GetConfigVector (cfgAcc, "Actor.Body", "0.2,1.8,0.2"));
  csVector3 shift (GetConfigVector (cfgAcc, "Actor.Shift", "0.0,-1.7,0.0"));
  collider_actor.InitializeColliders (camera, legs, body, shift);
}

void Camera::InterpolateCamera (float elapsed_time)
{
  float sqdist = csSquaredDist::PointPoint (current.pos, desired.pos);
  bool rotEqual = (current.rot.v - desired.rot.v).IsZero ()
    && fabs (current.rot.w - desired.rot.w) < 0.00001f;
  if (sqdist < 0.02f && rotEqual) return;

  float damping = 10.0f * elapsed_time;
  if (damping >= 1.0f) damping = 1.0f;
  current.pos += (desired.pos - current.pos) * damping;
  current.rot = current.rot.SLerp (desired.rot, damping);

  SetCameraTransform (current);
}

float Camera::CalculatePanningVerticalAngle ()
{
  csVector3 rpos = desired.pos - panningCenter;
  float normalizedY = rpos.y / panningDistance;
  if (normalizedY >= 1.0f) normalizedY = 1.0f;
  float angle = acos (normalizedY);
  if (rpos.z < 0.0f) angle = PI*2.0f - angle;
  return angle;
}

float Camera::CalculatePanningHorizontalAngle ()
{
  csVector3 rpos = desired.pos - panningCenter;
  float normalizedX = rpos.x / panningDistance;
  if (normalizedX >= 1.0f) normalizedX = 1.0f;
  float angle = acos (normalizedX);
  if (rpos.z < 0.0f) angle = PI*2.0f - angle;
  return angle;
}

csVector3 Camera::CalculatePanningPosition (float angleX, float angleY)
{
  csVector3 rpos (0);
  rpos.x = cos (angleY) * panningDistance;
  rpos.y = cos (angleX) * panningDistance;
  rpos.z = sin (angleY) * panningDistance;
  return panningCenter + rpos;
}

void Camera::ClampDesiredLocation ()
{
  csVector3 isect;
  if (aresed3d->TraceBeamTerrain (desired.pos + csVector3 (0, 100, 0),
      desired.pos - csVector3 (0, 100, 0), isect))
  {
    if (desired.pos.y < isect.y+0.5f) desired.pos.y = isect.y+0.5f;
  }
}

void Camera::Pan (float rot_speed_x, float rot_speed_y, float distance)
{
  float angleX = CalculatePanningVerticalAngle ();
  float angleY = CalculatePanningHorizontalAngle ();

  panningDistance += distance;
  if (panningDistance < 0.5f) panningDistance = 0.5f;
  angleX += rot_speed_x;
  angleY += rot_speed_y;
  desired.pos = CalculatePanningPosition (angleX, angleY);
  ClampDesiredLocation ();

  CamLookAtPosition (panningCenter);
}

void Camera::Frame (float elapsed_time, int mouseX, int mouseY)
{
  if (!do_gravity)
    InterpolateCamera (elapsed_time);

  if (do_mouse_panning) return;
  if (do_mouse_dragging) return;

  iKeyboardDriver* kbd = aresed3d->GetKeyboardDriver ();
  bool slow = kbd->GetKeyState (CSKEY_CTRL);

  float kspeed = 20.0f * elapsed_time;

  float rot_speed_y = 0.0f;
  float rot_speed_x = 0.0f;

  if (do_panning)
  {
    float distance = 0.0f;
    if (kbd->GetKeyState ('d')) rot_speed_y += 0.08f * kspeed;
    if (kbd->GetKeyState ('a')) rot_speed_y -= 0.08f * kspeed;
    if (kbd->GetKeyState ('w')) distance -= 0.5f * kspeed;
    if (kbd->GetKeyState ('s')) distance += 0.5f * kspeed;
    if (kbd->GetKeyState (CSKEY_PGUP)) rot_speed_x -= 0.08f * kspeed;
    if (kbd->GetKeyState (CSKEY_PGDN)) rot_speed_x += 0.08f * kspeed;
    float speed = slow ? 0.1f : 1.0f;
    Pan (rot_speed_x * speed, rot_speed_y * speed, distance * speed);
    return;
  }

  csVector3 obj_move (0);
  if (kbd->GetKeyState (CSKEY_SHIFT))
  {
    // If the user is holding down shift, the arrow keys will cause
    // the camera to strafe up, down, left or right from it's
    // current position.
    if (kbd->GetKeyState ('d')) obj_move += CS_VEC_RIGHT * 2.0f * kspeed;
    if (kbd->GetKeyState ('a')) obj_move += CS_VEC_LEFT * 2.0f * kspeed;
    if (kbd->GetKeyState ('w')) obj_move += CS_VEC_UP * 2.0f * kspeed;
    if (kbd->GetKeyState ('s')) obj_move += CS_VEC_DOWN * 2.0f * kspeed;
  }
  else
  {
    // left and right cause the camera to rotate on the global Y
    // axis; page up and page down cause the camera to rotate on the
    // _camera's_ X axis (more on this in a second) and up and down
    // arrows cause the camera to go forwards and backwards.
    if (kbd->GetKeyState ('d')) rot_speed_y -= 0.1f * kspeed;
    if (kbd->GetKeyState ('a')) rot_speed_y += 0.1f * kspeed;
    if (kbd->GetKeyState (CSKEY_PGUP)) rot_speed_x += 0.1f * kspeed;
    if (kbd->GetKeyState (CSKEY_PGDN)) rot_speed_x -= 0.1f * kspeed;
    if (kbd->GetKeyState ('w')) obj_move += CS_VEC_FORWARD * 2.0f * kspeed;
    if (kbd->GetKeyState ('s')) obj_move += CS_VEC_BACKWARD * 2.0f * kspeed;
  }

  if (do_gravity)
  {
    csVector3 obj_rotate = rot_speed_x * csVector3 (1, 0, 0) +
      rot_speed_y * csVector3 (0, -1, 0);
    collider_actor.Move (kspeed / 20.0f, slow ? 0.5f : 2.0f, obj_move, obj_rotate * 10.0f);
  }
  else
  {
    float speed = slow ? 0.1f : 1.0f;
    CamMoveRelative (obj_move * speed, rot_speed_x * speed, rot_speed_y * speed);
  }
}

bool Camera::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (do_gravity) return false;

  if (but == 3)	// MouseWheelUp
  {
    if (do_panning) 
      Pan (0.0f, 0.0f, -10.0f);
    else
      CamZoom (mouseX, mouseY, true);
    return true;
  }
  else if (but == 4)	// MouseWheelDown
  {
    if (do_panning) 
      Pan (0.0f, 0.0f, 10.0f);
    else
      CamZoom (mouseX, mouseY, false);
    return true;
  }
  else if (but == 2)	// Middle mouse
  {
    iKeyboardDriver* kbd = aresed3d->GetKeyboardDriver ();
    if (kbd->GetKeyState (CSKEY_SHIFT))
    {
      do_mouse_panning = false;
      if (!do_mouse_dragging)
      {
        aresed3d->TraceBeam (aresed3d->GetBeam (mouseX, mouseY), panningCenter);
        do_mouse_dragging = true;
      }
    }
    else if (kbd->GetKeyState (CSKEY_CTRL))
    {
      // @@@ Implement zoom.
    }
    else
    {
      do_mouse_dragging = false;
      if (!do_mouse_panning)
      {
        aresed3d->TraceBeam (aresed3d->GetBeam (mouseX, mouseY), panningCenter);
        CamLookAtPosition (panningCenter);
        panningDistance = sqrt (csSquaredDist::PointPoint (
	      panningCenter, desired.pos));
        if (panningDistance < 200.0f)
        {
	  do_mouse_panning = true;
	  int w = aresed3d->GetG2D ()->GetWidth ();
	  int h = aresed3d->GetG2D ()->GetHeight ();
	  aresed3d->GetG2D ()->SetMousePosition (w / 2, h / 2);
        }
      }
    }
  }
  return false;
}

bool Camera::OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (but == 2)
  {
    do_mouse_dragging = false;
    do_mouse_panning = false;
  }
  return false;
}

void Camera::OnFocusLost ()
{
  do_mouse_dragging = false;
  do_mouse_panning = false;
}

bool Camera::OnMouseMove (iEvent& ev, int mouseX, int mouseY)
{
  if (do_mouse_panning)
  {
    int w = aresed3d->GetG2D ()->GetWidth () / 2;
    int h = aresed3d->GetG2D ()->GetHeight () / 2;
    aresed3d->GetG2D ()->SetMousePosition (w, h);
    Pan (float (mouseY-h) / 50.0f, float (w-mouseX) / 50.0f, 0.0f);
    return true;
  }
  else if (do_mouse_dragging)
  {
    csVector2 v2d (mouseX, aresed3d->GetG2D ()->GetHeight () - mouseY);
    csVector3 v3d = camera->InvPerspective (v2d, 1000.0f);
    csVector3 startBeam = camera->GetTransform ().GetOrigin ();
    csVector3 endBeam = camera->GetTransform ().This2Other (v3d);
    csVector3 isect;
    if (fabs (startBeam.y-endBeam.y) < 0.1f) return true;
    if (endBeam.y < startBeam.y && panningCenter.y > startBeam.y) return true;
    if (endBeam.y > startBeam.y && panningCenter.y < startBeam.y) return true;
    float dist = csIntersect3::SegmentYPlane (startBeam, endBeam, panningCenter.y,
	isect);
    if (dist > 0.15f)
    {
      isect = startBeam + (endBeam-startBeam).Unit () * 150.0f;
      isect.y = panningCenter.y;
    }
    desired.pos += panningCenter - isect;
    ClampDesiredLocation ();
    return true;
  }
  return false;
}

void Camera::SetCameraTransform (const CamLocation& loc)
{
  csMatrix3 rot = loc.rot.GetMatrix ();
  csOrthoTransform ot (rot, loc.pos);
  camera->SetTransform (ot);
}

static csQuaternion RotationToQuat (const csVector3& rot)
{
  csVector3 r = rot;
  r.y = -r.y;
  csQuaternion quat;
  quat.SetEulerAngles (r);
  return quat;
}

static csVector3 QuatToRotation (const csQuaternion& quat)
{
  csVector3 euler = quat.GetEulerAngles ();
  euler.y = -euler.y;
  return euler;
}

CamLocation Camera::GetCameraLocation ()
{
  if (do_gravity)
  {
    CamLocation loc;
    loc.pos = camera->GetTransform ().GetOrigin ();
    loc.rot = RotationToQuat (collider_actor.GetRotation ());
    return loc;
  }
  else
    return current;
}

void Camera::SetCameraLocation (const CamLocation& loc)
{
  desired = loc;
  if (do_gravity)
  {
    camera->GetTransform ().SetOrigin (loc.pos);
    collider_actor.SetRotation (QuatToRotation (loc.rot));
  }
}

void Camera::CamMove (const csVector3& pos)
{
  desired.pos = pos;
  ClampDesiredLocation ();
}

void Camera::CamMoveRelative (const csVector3& offset, float angleX, float angleY)
{
  desired.pos += camera->GetTransform ().This2OtherRelative (offset);
  ClampDesiredLocation ();
  if (fabs (angleX) > 0.0001 || fabs (angleY) > 0.0001)
  {
    csQuaternion rotQuatX, rotQuatY;
    rotQuatX.SetAxisAngle (csVector3 (1, 0, 0), angleX);
    rotQuatY.SetAxisAngle (csVector3 (0, 1, 0), angleY);
    desired.rot = rotQuatX * desired.rot;
    desired.rot = desired.rot * rotQuatY;
  }
}

void Camera::CamMoveAndLookAt (const csVector3& pos, const csQuaternion& rot)
{
  desired.pos = pos;
  desired.rot = rot;
  ClampDesiredLocation ();
}

void Camera::CamMoveAndLookAt (const csVector3& pos, const csVector3& rot)
{
  CamMoveAndLookAt (pos, RotationToQuat (rot));
}

void Camera::CamLookAt (const csQuaternion& rot)
{
  desired.rot = rot;
  if (do_gravity)
  {
    csVector3 vrot = QuatToRotation (desired.rot);
    vrot.x = vrot.z = 0.0f;
    collider_actor.SetRotation (vrot);
  }
}

void Camera::CamLookAt (const csVector3& rot)
{
  desired.rot = RotationToQuat (rot);
  if (do_gravity)
    collider_actor.SetRotation (rot);
}

void Camera::CamLookAtPosition (const csVector3& center)
{
  //csVector3 diff = center - camera->GetTransform ().GetOrigin ();
  csVector3 diff = center - desired.pos;

  csOrthoTransform trans = camera->GetTransform ();
  trans.LookAt (diff, csVector3 (0, 1, 0));
  csQuaternion quat;
  quat.SetMatrix (trans.GetO2T ());
  CamLookAt (quat);
}

void Camera::CamBlendLookAtPosition (const csVector3& center, float weight)
{
  csOrthoTransform trans = camera->GetTransform ();
  csVector3 diff = center - trans.GetOrigin ();

  csQuaternion currentQuat;
  if (weight < 0.9999f)
    currentQuat.SetMatrix (trans.GetO2T ());

  trans.LookAt (diff, csVector3 (0, 1, 0));
  csQuaternion quat;
  quat.SetMatrix (trans.GetO2T ());

  if (weight < 0.9999f)
    quat = currentQuat.SLerp (quat, weight);

  CamLookAt (quat);
}

void Camera::CamZoom (int x, int y, bool forward)
{
  csVector2 v2d (x, aresed3d->GetG2D ()->GetHeight () - y);
  csVector3 v3d = camera->InvPerspective (v2d, 10.0f);
  csVector3 endBeamMove = camera->GetTransform ().This2Other (forward ? v3d : -v3d);
  csVector3 endBeamLookAt = camera->GetTransform ().This2Other (v3d);

  int halfw = aresed3d->GetG2D ()->GetWidth () / 2;
  int halfh = aresed3d->GetG2D ()->GetHeight () / 2;
  int dx = x - halfw;
  int dy = y - halfh;

  csQuaternion quat;
  quat.SetMatrix (camera->GetTransform ().GetT2O ());
  csVector3 euler = quat.GetEulerAngles ();

  if (euler.x < -1.55f && dy > 0)
    dy = 0;
  if (euler.x < 1.55f && dy < 0)
    dy = 0;

  dx *= dx;
  dy *= dy;
  float weight = csQsqrt (dx + dy) * csQisqrt (halfw * halfw + halfh * halfh);

  CamBlendLookAtPosition (endBeamLookAt, weight);
  CamMove (endBeamMove);
}

void Camera::EnableGravity ()
{
  if (do_gravity) return;
  DisablePanning ();
  do_gravity = true;
  collider_actor.SetGravity (9.806f);
  camera->GetTransform ().SetOrigin (current.pos);
  csVector3 vrot = QuatToRotation (desired.rot);
  vrot.x = vrot.z = 0.0f;
  collider_actor.SetRotation (vrot);
}

void Camera::DisableGravity ()
{
  if (!do_gravity) return;
  // @@@ Bug! In case camera window is open it should be notified!
  do_gravity = false;
  collider_actor.SetGravity (0);
  if (camera)
  {
    current.pos = camera->GetTransform ().GetOrigin ();
    current.rot = RotationToQuat (collider_actor.GetRotation ());
    desired = current;
    desired.pos.y += 1.0f;
  }
}


