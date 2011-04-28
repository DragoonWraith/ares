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

#ifndef __APPARES_H__
#define __APPARES_H__

#include <crystalspace.h>

// CEL Includes
#include "physicallayer/messaging.h"
#include "actorsettings.h"

#include "include/idynworld.h"
#include "include/icurvemesh.h"
#include "include/inature.h"

struct iEngine;
struct iLoader;
struct iGraphics3D;
struct iKeyboardDriver;
struct iVirtualClock;
struct iObjectRegistry;
struct iEvent;
struct iSector;
struct iView;
class csVector3;
class FramePrinter;

struct iPcCamera;
struct iCelEntity;
struct iCelPlLayer;
struct iCelBlLayer;
struct iCelPropertyClass;
struct iCelPropertyClassFactory;

class WorldLoader;

/**
 * Main application class of AppAres.
 */
class AppAres : public csApplicationFramework,
		public csBaseEventHandler
{
private:
  csRef<iEngine> engine;
  csRef<iLoader> loader;
  csRef<iGraphics3D> g3d;
  csRef<iKeyboardDriver> kbd;
  csRef<iVirtualClock> vc;
  csRef<FramePrinter> printer;
  csRef<iVFS> vfs;
  csRef<iCollideSystem> cdsys;

  csRef<iDynamicWorld> dynworld;
  csRef<iCurvedMeshCreator> curvedMeshCreator;

  csRef<iNature> nature;
  csTicks currentTime;

  csRef<iCelPlLayer> pl;
  csRef<iCelEntity> game;
  csRef<iCelEntity> entity_cam;

  WorldLoader* worldLoader;

  ActorSettings actorsettings;

  iSector* sector;
  iCamera* camera;

  /// Physics.
  csRef<iDynamics> dyn;
  csRef<iDynamicSystem> dynSys;
  csRef<CS::Physics::Bullet::iDynamicSystem> bullet_dynSys;

  /**
   * Setup everything that needs to be rendered on screen. This routine
   * is called from the event handler in response to a csevFrame
   * broadcast message.
   */
  virtual void Frame ();

  /**
   * Handle keyboard events - ie key presses and releases.
   * This routine is called from the event handler in response to a 
   * csevKeyboard event.
   */
  virtual bool OnKeyboard (iEvent &event);

  bool PostLoadMap ();
  bool InitPhysics ();
  void CreateActor ();
  void CreateActionIcon ();
  void CreateSettingBar ();
  void ConnectWires ();

  /// Load a library file with the given VFS path.
  bool LoadLibrary (const char* path, const char* file);

public:
  AppAres ();
  virtual ~AppAres ();

  /**
   * Final cleanup.
   */
  virtual void OnExit ();

  /**
   * Main initialization routine. This routine will set up some basic stuff
   * (like load all needed plugins, setup the event handler, ...).
   * In case of failure this routine will return false. You can assume
   * that the error message has been reported to the user.
   */
  virtual bool OnInitialize (int argc, char* argv[]);

  /**
   * Run the application.
   * First, there are some more initialization (everything that is needed 
   * by CrystalCore to use Crystal Space and CEL), Then this routine fires up 
   * the main event loop. This is where everything starts. This loop will 
   * basically start firing events which actually causes Crystal Space to 
   * function. Only when the program exits this function will return.
   */
  virtual bool Application ();

  CS_EVENTHANDLER_NAMES("ares")
  CS_EVENTHANDLER_NIL_CONSTRAINTS
};

#endif // __APPARES_H__

