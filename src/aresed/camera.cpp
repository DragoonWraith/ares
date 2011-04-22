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

Camera::Camera (AppAresEdit* aresed) : aresed (aresed)
{
  do_panning = false;
  do_gravity = false;
  camera = 0;
}

void Camera::EnablePanning (const csVector3& center)
{
  do_panning = true;
  panningCenter = center;
}

void Camera::DisablePanning ()
{
  do_panning = false;
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
  collider_actor.SetCollideSystem (aresed->GetCollisionSystem ());
  collider_actor.SetEngine (aresed->GetEngine ());
  collider_actor.SetGravity (0);

  // Creates an accessor for configuration settings.
  csConfigAccess cfgAcc (aresed->GetObjectRegistry ());

  // Read the values from config files, and use default values if not set.
  csVector3 legs (GetConfigVector (cfgAcc, "Actor.Legs", "0.2,0.5,0.2"));
  csVector3 body (GetConfigVector (cfgAcc, "Actor.Body", "0.2,1.8,0.2"));
  csVector3 shift (GetConfigVector (cfgAcc, "Actor.Shift", "0.0,-1.7,0.0"));
  collider_actor.InitializeColliders (camera, legs, body, shift);
}

void Camera::InterpolateCamera (csTicks elapsed_time)
{
  float sqdist = csSquaredDist::PointPoint (current.pos, desired.pos);
  bool rotEqual = (current.rot.v - desired.rot.v).IsZero ()
    && fabs (current.rot.w - desired.rot.w) < 0.00001f;
  if (sqdist < 0.02f && rotEqual) return;

  float damping = 10.0f * float (elapsed_time) / 1000.0f;
  if (damping >= 1.0f) damping = 1.0f;
  current.pos += (desired.pos - current.pos) * damping;
  current.rot = current.rot.SLerp (desired.rot, damping);

  SetCameraTransform (current);
}

void Camera::Frame (csTicks elapsed_time)
{
  if (!do_gravity)
    InterpolateCamera (elapsed_time);

  iKeyboardDriver* kbd = aresed->GetKeyboardDriver ();
  bool slow = kbd->GetKeyState (CSKEY_CTRL);

  csVector3 obj_move (0);
  float rot_speed_y = 0.0f;
  float rot_speed_x = 0.0f;
  if (kbd->GetKeyState (CSKEY_SHIFT))
  {
    // If the user is holding down shift, the arrow keys will cause
    // the camera to strafe up, down, left or right from it's
    // current position.
    if (kbd->GetKeyState ('d')) obj_move += CS_VEC_RIGHT * 3.0f;
    if (kbd->GetKeyState ('a')) obj_move += CS_VEC_LEFT * 3.0f;
    if (kbd->GetKeyState ('w')) obj_move += CS_VEC_UP * 3.0f;
    if (kbd->GetKeyState ('s')) obj_move += CS_VEC_DOWN * 3.0f;
  }
  else
  {
    // left and right cause the camera to rotate on the global Y
    // axis; page up and page down cause the camera to rotate on the
    // _camera's_ X axis (more on this in a second) and up and down
    // arrows cause the camera to go forwards and backwards.
    if (kbd->GetKeyState ('d')) rot_speed_y -= 0.1f;
    if (kbd->GetKeyState ('a')) rot_speed_y += 0.1f;
    if (kbd->GetKeyState (CSKEY_PGUP)) rot_speed_x += 0.1f;
    if (kbd->GetKeyState (CSKEY_PGDN)) rot_speed_x -= 0.1f;
    if (kbd->GetKeyState ('w')) obj_move += CS_VEC_FORWARD * 3.0f;
    if (kbd->GetKeyState ('s')) obj_move += CS_VEC_BACKWARD * 3.0f;
  }

  if (do_gravity)
  {
    const float speed = elapsed_time / 1000.0;
    csVector3 obj_rotate = rot_speed_x * csVector3 (1, 0, 0) +
      rot_speed_y * csVector3 (0, -1, 0);
    collider_actor.Move (speed, slow ? 0.5f : 2.0f, obj_move, obj_rotate * 10.0f);
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
    CamZoom (mouseX, mouseY);
    return true;
  }
  else if (but == 4)	// MouseWheelDown
  {
    CamMoveRelative (CS_VEC_BACKWARD * 10.0f, 0.0f, 0.0f);
    return true;
  }
  return false;
}

bool Camera::OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY)
{
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
  if (do_gravity)
    camera->GetTransform ().SetOrigin (pos);
}

void Camera::CamMoveRelative (const csVector3& offset, float angleX, float angleY)
{
  if (do_gravity)
  {
    desired = GetCameraLocation ();
  }
  desired.pos += camera->GetTransform ().This2OtherRelative (offset);
  if (fabs (angleX) > 0.0001 || fabs (angleY) > 0.0001)
  {
    csQuaternion rotQuatX, rotQuatY;
    rotQuatX.SetAxisAngle (csVector3 (1, 0, 0), angleX);
    rotQuatY.SetAxisAngle (csVector3 (0, 1, 0), angleY);
    desired.rot = rotQuatX * desired.rot;
    desired.rot = desired.rot * rotQuatY;
  }
  if (do_gravity)
  {
    camera->GetTransform ().SetOrigin (desired.pos);
    collider_actor.SetRotation (QuatToRotation (desired.rot));
  }
}

void Camera::CamMoveAndLookAt (const csVector3& pos, const csQuaternion& rot)
{
  desired.pos = pos;
  desired.rot = rot;
  if (do_gravity)
    SetCameraLocation (desired);
}

void Camera::CamMoveAndLookAt (const csVector3& pos, const csVector3& rot)
{
  CamMoveAndLookAt (pos, RotationToQuat (rot));
}

void Camera::CamLookAt (const csQuaternion& rot)
{
  desired.rot = rot;
  if (do_gravity)
    collider_actor.SetRotation (QuatToRotation (desired.rot));
}

void Camera::CamLookAt (const csVector3& rot)
{
  desired.rot = RotationToQuat (rot);
  if (do_gravity)
    collider_actor.SetRotation (rot);
}

void Camera::CamLookAtPosition (const csVector3& center)
{
  csVector3 diff = center - camera->GetTransform ().GetOrigin ();

  csOrthoTransform trans = camera->GetTransform ();
  trans.LookAt (diff, csVector3 (0, 1, 0));
  csQuaternion quat;
  quat.SetMatrix (trans.GetO2T ());
  CamLookAt (quat);
}

void Camera::CamZoom (int x, int y)
{
  csVector2 v2d (x, aresed->GetG2D ()->GetHeight () - y);
  csVector3 v3d = camera->InvPerspective (v2d, 10);
  csVector3 endBeam = camera->GetTransform ().This2Other (v3d);

  CamLookAtPosition (endBeam);
  CamMove (endBeam);
}

void Camera::EnableGravity ()
{
  do_gravity = true;
  collider_actor.SetGravity (9.806);
  CamMoveAndLookAt (current.pos, current.rot);
}

void Camera::DisableGravity ()
{
  do_gravity = false;
  collider_actor.SetGravity (0);
  if (camera)
  {
    current.pos = camera->GetTransform ().GetOrigin ();
    current.rot = RotationToQuat (collider_actor.GetRotation ());
    desired = current;
  }
}


