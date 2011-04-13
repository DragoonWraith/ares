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

#include "cssysdef.h"
#include "appares.h"

#include "propclass/cameras/tracking.h"
#include "propclass/linmove.h"
#include "propclass/analogmotion.h"
#include "tools/billboard.h"
#include "propclass/billboard.h"

//-----------------------------------------------------------------------------

enum {
  CELSET_TRACK_TGT_INTERPOLATION_TIME,
  CELSET_TRACK_FOLLOW_SPRING_LENGTH,
  CELSET_TRACK_FOLLOW_MIN_SPRING_FACTOR,
  CELSET_TRACK_PAN_SPEED,
  CELSET_TRACK_PAN_ACCEL,
  CELSET_TRACK_TILT_SPEED,
  CELSET_TRACK_TILT_ACCEL,
  CELSET_TRACK_ZOOMOUT_CORR_SPEED,
  CELSET_LINMOVE_DELTA_LIMIT,
  CELSET_LINMOVE_SPEED,
  CELSET_LINMOVE_GRAVITY,
  CELSET_ANALOG_MOVE_SPEED,
  CELSET_ANALOG_MOVE_ACCEL,
  CELSET_ANALOG_MOVE_DECEL,
  CELSET_ANALOG_TURNING_SPEED,
  CELSET_ANALOG_MIN_TURNING_SPEED,
  CELSET_ANALOG_MAX_TURNING_SPEED,
  CELSET_DELEGCAM_TRANSITION_TIME,
  CELSET_DELEGCAM_CONT_TRANS_SPEED
};

#define CELSET_FIRST CELSET_TRACK_TGT_INTERPOLATION_TIME
#define CELSET_LAST CELSET_DELEGCAM_CONT_TRANS_SPEED

//-----------------------------------------------------------------------------

bool CelTestMessageReceiver::ReceiveMessage (csStringID msg_id, iMessageSender*,
      celData&, iCelParameterBlock* params)
{
  return as->ReceiveMessage (msg_id, params);
}

//-----------------------------------------------------------------------------

void ActorSettings::Initialize (iCelPlLayer* pl)
{
  ActorSettings::pl = pl;

  setting_bar = pl->FindEntity ("setting_bar");
  entity_cam = pl->FindEntity ("camera");

  // Setup our receiver.
  receiver.AttachNew (new CelTestMessageReceiver (this));
  entity_cam->QueryMessageChannel ()->Subscribe (receiver, "cel.input.settings");
  id_toggle_setting_bar = pl->FetchStringID ("cel.input.settings.down");
  id_next_setting = pl->FetchStringID ("cel.input.settings_down.down");
  id_prev_setting = pl->FetchStringID ("cel.input.settings_up.down");
  id_decrease_setting = pl->FetchStringID ("cel.input.settings_left.down");
  id_increase_setting = pl->FetchStringID ("cel.input.settings_right.down");
  id_decrease_setting_slow = pl->FetchStringID ("cel.input.settings_slowleft.down");
  id_increase_setting_slow = pl->FetchStringID ("cel.input.settings_slowright.down");
  id_decrease_setting_fast = pl->FetchStringID ("cel.input.settings_fastleft.down");
  id_increase_setting_fast = pl->FetchStringID ("cel.input.settings_fastright.down");
  id_next_setting_repeat = pl->FetchStringID ("cel.input.settings_down.repeat");
  id_prev_setting_repeat = pl->FetchStringID ("cel.input.settings_up.repeat");
  id_decrease_setting_repeat = pl->FetchStringID ("cel.input.settings_left.repeat");
  id_increase_setting_repeat = pl->FetchStringID ("cel.input.settings_right.repeat");
  id_decrease_setting_slow_repeat = pl->FetchStringID ("cel.input.settings_slowleft.repeat");
  id_increase_setting_slow_repeat = pl->FetchStringID ("cel.input.settings_slowright.repeat");
  id_decrease_setting_fast_repeat = pl->FetchStringID ("cel.input.settings_fastleft.repeat");
  id_increase_setting_fast_repeat = pl->FetchStringID ("cel.input.settings_fastright.repeat");
  current_setting = CELSET_TRACK_TGT_INTERPOLATION_TIME;
  UpdateSetting ();
}

void ActorSettings::UpdateSetting ()
{
  csRef<iPcTrackingCamera> trackcam = celQueryPropertyClassEntity<iPcTrackingCamera> (entity_cam);
  csRef<iPcLinearMovement> linmove = celQueryPropertyClassEntity<iPcLinearMovement> (entity_cam);
  csRef<iPcAnalogMotion> analog = celQueryPropertyClassEntity<iPcAnalogMotion> (entity_cam);
  csRef<iPcDelegateCamera> deleg = celQueryPropertyClassEntity<iPcDelegateCamera> (entity_cam);
  csString str;
  str = "...";
  switch (current_setting)
  {
    case CELSET_TRACK_TGT_INTERPOLATION_TIME:
      str.Format ("Track(target interpolation time): %d", trackcam->GetTargetInterpolationTime ());
      break;
    case CELSET_TRACK_FOLLOW_SPRING_LENGTH:
      str.Format ("Track(follow spring length): %g", trackcam->GetFollowSpringLength ());
      break;
    case CELSET_TRACK_FOLLOW_MIN_SPRING_FACTOR:
      str.Format ("Track(follow min spring factor): %g", trackcam->GetFollowMinimumSpringFactor ());
      break;
    case CELSET_TRACK_PAN_SPEED:
      str.Format ("Track(pan speed): %g", trackcam->GetPanSpeed ());
      break;
    case CELSET_TRACK_PAN_ACCEL:
      str.Format ("Track(pan acceleration): %g", trackcam->GetPanAcceleration ());
      break;
    case CELSET_TRACK_TILT_SPEED:
      str.Format ("Track(tilt speed): %g", trackcam->GetTiltSpeed ());
      break;
    case CELSET_TRACK_TILT_ACCEL:
      str.Format ("Track(tilt acceleration): %g", trackcam->GetTiltAcceleration ());
      break;
    case CELSET_TRACK_ZOOMOUT_CORR_SPEED:
      str.Format ("Track(zoomout correction speed): %g", trackcam->GetZoomOutCorrectionSpeed ());
      break;
    case CELSET_LINMOVE_DELTA_LIMIT:
      str.Format ("LinMove(delta limit): %g", linmove->GetDeltaLimit ());
      break;
    case CELSET_LINMOVE_SPEED:
      str.Format ("LinMove(speed): %g", linmove->GetSpeed ());
      break;
    case CELSET_LINMOVE_GRAVITY:
      str.Format ("LinMove(gravity): %g", linmove->GetGravity ());
      break;
    case CELSET_ANALOG_MOVE_SPEED:
      str.Format ("Analog(movement speed): %g", analog->GetMovementSpeed ());
      break;
    case CELSET_ANALOG_MOVE_ACCEL:
      str.Format ("Analog(movement acceleration): %g", analog->GetMovementAcceleration ());
      break;
    case CELSET_ANALOG_MOVE_DECEL:
      str.Format ("Analog(movement deceleration): %g", analog->GetMovementDeceleration ());
      break;
    case CELSET_ANALOG_TURNING_SPEED:
      str.Format ("Analog(turning speed): %g", analog->GetTurningSpeed ());
      break;
    case CELSET_ANALOG_MIN_TURNING_SPEED:
      str.Format ("Analog(minimum turning speed): %g", analog->GetMinimumTurningSpeed ());
      break;
    case CELSET_ANALOG_MAX_TURNING_SPEED:
      str.Format ("Analog(maximum turning speed): %g", analog->GetMaximumTurningSpeed ());
      break;
    case CELSET_DELEGCAM_TRANSITION_TIME:
      str.Format ("Delegate(transition time): %d", deleg->GetTransitionTime ());
      break;
    case CELSET_DELEGCAM_CONT_TRANS_SPEED:
      str.Format ("Delegate(continous transition speed): %g", deleg->GetContinousTransitionSpeed ());
      break;
  }

  csRef<iPcBillboard> pcbb = celQueryPropertyClassEntity<iPcBillboard> (setting_bar);
  iBillboard* bb = pcbb->GetBillboard ();
  bb->SetText (str);
}

void ActorSettings::ChangeSetting (float dir)
{
  csRef<iPcTrackingCamera> trackcam = celQueryPropertyClassEntity<iPcTrackingCamera> (entity_cam);
  csRef<iPcLinearMovement> linmove = celQueryPropertyClassEntity<iPcLinearMovement> (entity_cam);
  csRef<iPcAnalogMotion> analog = celQueryPropertyClassEntity<iPcAnalogMotion> (entity_cam);
  csRef<iPcDelegateCamera> deleg = celQueryPropertyClassEntity<iPcDelegateCamera> (entity_cam);
  switch (current_setting)
  {
    case CELSET_TRACK_TGT_INTERPOLATION_TIME:
      trackcam->SetTargetInterpolationTime (trackcam->GetTargetInterpolationTime () + int (dir));
      break;
    case CELSET_TRACK_FOLLOW_SPRING_LENGTH:
      trackcam->SetFollowSpringLength (trackcam->GetFollowSpringLength () + 0.1 * dir);
      break;
    case CELSET_TRACK_FOLLOW_MIN_SPRING_FACTOR:
      trackcam->SetFollowMinimumSpringFactor (trackcam->GetFollowMinimumSpringFactor () + 0.1 * dir);
      break;
    case CELSET_TRACK_PAN_SPEED:
      trackcam->SetPanSpeed (trackcam->GetPanSpeed () + 0.1 * dir);
      break;
    case CELSET_TRACK_PAN_ACCEL:
      trackcam->SetPanAcceleration (trackcam->GetPanAcceleration () + 0.1 * dir);
      break;
    case CELSET_TRACK_TILT_SPEED:
      trackcam->SetTiltSpeed (trackcam->GetTiltSpeed () + 0.1 * dir);
      break;
    case CELSET_TRACK_TILT_ACCEL:
      trackcam->SetTiltAcceleration (trackcam->GetTiltAcceleration () + 0.1 * dir);
      break;
    case CELSET_TRACK_ZOOMOUT_CORR_SPEED:
      trackcam->SetZoomOutCorrectionSpeed (trackcam->GetZoomOutCorrectionSpeed () + 0.1 * dir);
      break;
    case CELSET_LINMOVE_DELTA_LIMIT:
      linmove->SetDeltaLimit (linmove->GetDeltaLimit () + 0.1 * dir);
      break;
    case CELSET_LINMOVE_SPEED:
      linmove->SetSpeed (linmove->GetSpeed () + 0.1 * dir);
      break;
    case CELSET_LINMOVE_GRAVITY:
      linmove->SetGravity (linmove->GetGravity () + 0.1 * dir);
      break;
    case CELSET_ANALOG_MOVE_SPEED:
      analog->SetMovementSpeed (analog->GetMovementSpeed () + 0.1 * dir);
      break;
    case CELSET_ANALOG_MOVE_ACCEL:
      analog->SetMovementAcceleration (analog->GetMovementAcceleration () + 0.1 * dir);
      break;
    case CELSET_ANALOG_MOVE_DECEL:
      analog->SetMovementDeceleration (analog->GetMovementDeceleration () + 0.1 * dir);
      break;
    case CELSET_ANALOG_TURNING_SPEED:
      analog->SetTurningSpeed (analog->GetTurningSpeed () + 0.1 * dir);
      break;
    case CELSET_ANALOG_MIN_TURNING_SPEED:
      analog->SetMinimumTurningSpeed (analog->GetMinimumTurningSpeed () + 0.1 * dir);
      break;
    case CELSET_ANALOG_MAX_TURNING_SPEED:
      analog->SetMaximumTurningSpeed (analog->GetMaximumTurningSpeed () + 0.1 * dir);
      break;
    case CELSET_DELEGCAM_TRANSITION_TIME:
      deleg->SetTransitionTime (deleg->GetTransitionTime () + int(10.0 * dir));
      break;
    case CELSET_DELEGCAM_CONT_TRANS_SPEED:
      deleg->SetContinousTransitionSpeed (deleg->GetContinousTransitionSpeed () + 0.1 * dir);
      break;
  }
}

bool ActorSettings::ReceiveMessage (csStringID msg_id, iCelParameterBlock* params)
{
  if (msg_id == id_toggle_setting_bar)
  {
    csRef<iPcBillboard> pcbb = celQueryPropertyClassEntity<iPcBillboard> (setting_bar);
    iBillboard* bb = pcbb->GetBillboard ();
    csFlags& f = bb->GetFlags ();
    if (f.Check (CEL_BILLBOARD_VISIBLE))
      f.Reset (CEL_BILLBOARD_VISIBLE);
    else
      f.Set (CEL_BILLBOARD_VISIBLE);
    return true;
  }
  if (msg_id == id_next_setting || msg_id == id_next_setting_repeat)
  {
    current_setting++;
    if (current_setting > CELSET_LAST)
      current_setting = CELSET_FIRST;
    UpdateSetting ();
    return true;
  }
  if (msg_id == id_prev_setting || msg_id == id_prev_setting_repeat)
  {
    current_setting--;
    if (current_setting < CELSET_FIRST)
      current_setting = CELSET_LAST;
    UpdateSetting ();
    return true;
  }
  if (msg_id == id_decrease_setting || msg_id == id_decrease_setting_repeat)
  {
    ChangeSetting (-1);
    UpdateSetting ();
    return true;
  }
  if (msg_id == id_increase_setting || msg_id == id_increase_setting_repeat)
  {
    ChangeSetting (1);
    UpdateSetting ();
    return true;
  }
  if (msg_id == id_decrease_setting_slow || msg_id == id_decrease_setting_slow_repeat)
  {
    ChangeSetting (-0.1);
    UpdateSetting ();
    return true;
  }
  if (msg_id == id_increase_setting_slow || msg_id == id_increase_setting_slow_repeat)
  {
    ChangeSetting (0.1);
    UpdateSetting ();
    return true;
  }
  if (msg_id == id_decrease_setting_fast || msg_id == id_decrease_setting_fast_repeat)
  {
    ChangeSetting (-10);
    UpdateSetting ();
    return true;
  }
  if (msg_id == id_increase_setting_fast || msg_id == id_increase_setting_fast_repeat)
  {
    ChangeSetting (10);
    UpdateSetting ();
    return true;
  }
  return false;
}

