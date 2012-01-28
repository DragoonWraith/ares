/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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

#ifndef __CEL_PF_GAMECONTROL__
#define __CEL_PF_GAMECONTROL__

#include "cstypes.h"
#include "iutil/comp.h"
#include "csutil/scf.h"
#include "physicallayer/propclas.h"
#include "physicallayer/propfact.h"
#include "physicallayer/facttmpl.h"
#include "celtool/stdpcimp.h"
#include "celtool/stdparams.h"
#include "include/igamecontrol.h"
#include "ivaria/bullet.h"

struct iCelEntity;
struct iObjectRegistry;
struct iPcCamera;
struct iMouseDriver;
struct iGraphics3D;
struct iGraphics2D;
struct iPcDynamicWorld;
struct iDynamicObject;
struct iFont;
struct iEngine;
struct iVirtualClock;
class csSimplePixmap;

/**
 * Factory for game controller.
 */
CEL_DECLARE_FACTORY (GameController)

struct TimedMessage
{
  csString message;
  float timeleft;
};

/**
 * This is a game controller property class.
 */
class celPcGameController : public scfImplementationExt1<
	celPcGameController, celPcCommon, iPcGameController>
{
private:
  // For SendMessage parameters.
  static csStringID id_message;
  static csStringID id_timeout;

  csRef<iGraphics3D> g3d;
  csRef<iGraphics2D> g2d;
  csRef<iVirtualClock> vc;
  csRef<iEngine> engine;

  /// Find the object that is pointed at in the center of the screen.
  iDynamicObject* FindCenterObject (iRigidBody*& hitBody,
      csVector3& start, csVector3& isect);

  // For dragging.
  csRef<iMouseDriver> mouse;
  csRef<CS::Physics::Bullet::iDynamicSystem> bullet_dynSys;
  csRef<CS::Physics::Bullet::iPivotJoint> dragJoint;
  float oldLinearDampening;
  float oldAngularDampening;
  float dragDistance;
  iDynamicObject* dragobj;

  // For messages.
  csArray<TimedMessage> messages;
  int messageColor;
  csRef<iFont> font;
  int fontW, fontH;

  // Icons.
  csSimplePixmap* iconCursor;
  csSimplePixmap* iconEye;
  csSimplePixmap* iconBook;
  csSimplePixmap* iconDot;
  void LoadIcons ();
  csStringID classNoteID;
  csStringID classInfoID;

  csRef<iPcDynamicWorld> dynworld;
  void TryGetDynworld ();

  csWeakRef<iPcCamera> pccamera;
  void TryGetCamera ();

  // For actions.
  enum actionids
  {
    action_message = 0,
    action_startdrag,
    action_stopdrag,
    action_examine,
  };

  // For properties.
  //enum propids
  //{
    //propid_counter = 0,
    //propid_max
  //};
  static PropertyHolder propinfo;

  //csRef<iMessageDispatcher> dispatcher_print;

public:
  celPcGameController (iObjectRegistry* object_reg);
  virtual ~celPcGameController ();

  virtual void Message (const char* message, float timeout = 2.0f);
  virtual void Examine ();
  virtual bool StartDrag ();
  virtual void StopDrag ();

  virtual bool PerformActionIndexed (int idx,
      iCelParameterBlock* params, celData& ret);

  virtual void TickEveryFrame ();
};

#endif // __CEL_PF_GAMECONTROL__

