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

void Camera::Frame (csTicks elapsed_time)
{
  // speed is a "magic value" which can help with FPS independence
  //float speed = (elapsed_time / 1000.0) * (0.03 * 20);

  csVector3 obj_move (0);
  csVector3 obj_rotate (0);
  iKeyboardDriver* kbd = aresed->GetKeyboardDriver ();
  bool slow = kbd->GetKeyState (CSKEY_CTRL);

  if (kbd->GetKeyState (CSKEY_SHIFT))
  {
    // If the user is holding down shift, the arrow keys will cause
    // the camera to strafe up, down, left or right from it's
    // current position.
    if (kbd->GetKeyState ('d'))
      obj_move = CS_VEC_RIGHT * 3.0f;
    if (kbd->GetKeyState ('a'))
      obj_move = CS_VEC_LEFT * 3.0f;
    if (kbd->GetKeyState ('w'))
      obj_move = CS_VEC_UP * 3.0f;
    if (kbd->GetKeyState ('s'))
      obj_move = CS_VEC_DOWN * 3.0f;
  }
  else
  { 
    // left and right cause the camera to rotate on the global Y
    // axis; page up and page down cause the camera to rotate on the
    // _camera's_ X axis (more on this in a second) and up and down
    // arrows cause the camera to go forwards and backwards.
    if (kbd->GetKeyState ('d'))
      obj_rotate.Set (0, 1, 0);
    if (kbd->GetKeyState ('a'))
      obj_rotate.Set (0, -1, 0);
    if (kbd->GetKeyState (CSKEY_PGUP))
      obj_rotate.Set (1, 0, 0);
    if (kbd->GetKeyState (CSKEY_PGDN))
      obj_rotate.Set (-1, 0, 0);
    if (kbd->GetKeyState ('w'))
      obj_move = CS_VEC_FORWARD * 3.0f;
    if (kbd->GetKeyState ('s'))
      obj_move = CS_VEC_BACKWARD * 3.0f;
  } 

  const float speed = elapsed_time / 1000.0;
  collider_actor.Move (speed, slow ? 0.5f : 2.0f, obj_move, obj_rotate);
}

bool Camera::OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY)
{
  if (but == 3)	// MouseWheelUp
  {
    collider_actor.Move (1.0f, 6.0f, CS_VEC_FORWARD * 3.0f, csVector3 (0));
    return true;
  }
  else if (but == 4)	// MouseWheelDown
  {
    collider_actor.Move (1.0f, 6.0f, CS_VEC_BACKWARD * 3.0f, csVector3 (0));
    return true;
  }
  return false;
}

bool Camera::OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY)
{
  return false;
}

CamLocation Camera::GetCameraLocation ()
{
  CamLocation loc;
  loc.pos = camera->GetTransform ().GetOrigin ();
  loc.rot = collider_actor.GetRotation ();
  return loc;
}

void Camera::SetCameraLocation (const CamLocation& loc)
{
  camera->GetTransform ().SetOrigin (loc.pos);
  collider_actor.SetRotation (loc.rot);
}

void Camera::CamMove (const csVector3& pos)
{
  camera->GetTransform ().SetOrigin (pos);
}

void Camera::CamMoveAndLookAt (const csVector3& pos, const csVector3& rot)
{
  camera->GetTransform ().SetOrigin (pos);
  collider_actor.SetRotation (rot);
}

void Camera::CamLookAt (const csVector3& rot)
{
  collider_actor.SetRotation (rot);
}

void Camera::EnableGravity ()
{
  collider_actor.SetGravity (9.806);
}

void Camera::DisableGravity ()
{
  collider_actor.SetGravity (0);
}


