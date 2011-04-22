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

#ifndef __aresed_camera_h
#define __aresed_camera_h

#include "iengine/camera.h"

class AppAresEdit;

struct CamLocation
{
  csVector3 pos;
  //csVector3 rot;
  csQuaternion rot;
};

class Camera
{
protected:
  AppAresEdit* aresed;

  iCamera* camera;
 
  /// Our collider used for gravity and CD (collision detection).
  csColliderActor collider_actor;

  bool do_panning;
  bool do_mouse_panning;
  csVector3 panningCenter;
  float panningDistance;

  bool do_gravity;

  CamLocation current;
  CamLocation desired;

  void InterpolateCamera (csTicks elapsed_time);
  void SetCameraTransform (const CamLocation& loc);
  float CalculatePanningHorizontalAngle ();
  float CalculatePanningVerticalAngle ();
  csVector3 CalculatePanningPosition (float angleX, float angleY);
  void Pan (float rot_speed_x, float rot_speed_y, float distance);

public:
  Camera (AppAresEdit* aresed);

  void Init (iCamera* camera, iSector* sector, const csVector3& pos);

  bool IsPanningEnabled () const { return do_panning; }
  void EnablePanning (const csVector3& center);
  void DisablePanning ();

  void Frame (csTicks elapsed, int mouseX, int mouseY);
  bool OnMouseDown (iEvent& ev, uint but, int mouseX, int mouseY);
  bool OnMouseUp (iEvent& ev, uint but, int mouseX, int mouseY);
  bool OnMouseMove (iEvent& ev, int mouseX, int mouseY);

  /**
   * Move the camera.
   */
  void CamMove (const csVector3& pos);

  /**
   * Move the camera relative in camera space.
   */
  void CamMoveRelative (const csVector3& offset, float angleX, float angleY);

  /**
   * Move the camera and let it look in some direction.
   */
  void CamMoveAndLookAt (const csVector3& pos, const csQuaternion& rot);

  /**
   * Move the camera and let it look in some direction.
   */
  void CamMoveAndLookAt (const csVector3& pos, const csVector3& rot);

  /**
   * Let the camera look in some direction.
   */
  void CamLookAt (const csQuaternion& rot);

  /**
   * Let the camera look in some direction.
   */
  void CamLookAt (const csVector3& rot);

  /**
   * Let the camera look at some position.
   */
  void CamLookAtPosition (const csVector3& pos);

  /**
   * Blend the current camera orientation with a look at some position.
   */
  void CamBlendLookAtPosition (const csVector3& pos, float weight = 1.0f);

  /**
   * Zoom the camera to a given location as pointed too by the mouse.
   */
  void CamZoom (int x, int y, bool forward);

  /**
   * Get the current camera position and rotation.
   */
  CamLocation GetCameraLocation ();

  /**
   * Set the current camera position and rotation.
   */
  void SetCameraLocation (const CamLocation& loc);

  bool IsGravityEnabled () const { return do_gravity; }

  /// Enable gravity.
  void EnableGravity ();

  /// Disable gravity.
  void DisableGravity ();
};

#endif // __aresed_camera_h

