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

#include "include/icurvemesh.h"
#include "include/irooms.h"
#include "include/inature.h"

#include "propclass/dynworld.h"

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

struct iAssetManager;

struct MenuEntry
{
  csString name;
  csString filename;
  int x1, y1;
  int dx, dy;
  int w, h;
  int alpha;
  bool active;
  int fontw, fonth;

  int src_w, dest_w;
  int src_h, dest_h;
  int src_dx, dest_dx;
  int src_dy, dest_dy;
  float totaltime, currenttime;

  MenuEntry () : dx (0), dy (0), alpha (0), active (false),
  	totaltime (0.0f), currenttime (0.0f)
  { }

  void Move (int x, int y, int screenw);
  void StartInterpolate (int destdx, int destdy, int destw, int desth, float seconds);
  void Step (float seconds);
};

#define MENU_GAME 0
#define MENU_LIST 1
#define MENU_WAIT1 2
#define MENU_WAIT2 3

class AppAres;

/**
 * Menu handling.
 */
class AresMenu
{
private:
  AppAres* app;
  int menuMode;
  csArray<MenuEntry> menuGames;
  csRef<iFont> menuFont;
  int mouseX, mouseY;
  csString gameFile;
  float menuActive;		// Active menu entry

  csSimplePixmap* menuBox;
  csSimplePixmap* menuLoading;

  void ActivateMenuEntry (float entry);

public:
  AresMenu (AppAres* app);
  ~AresMenu ();

  bool SetupMenu ();
  void CleanupMenu ();

  void MenuFrame ();
  void OnMouseMove (iEvent& ev);
  bool OnMouseDown (iEvent& ev);

  int GetMode () const { return menuMode; }
  void SetMode (int m) { menuMode = m; }
};

/**
 * Main application class of AppAres.
 */
class AppAres : public csApplicationFramework,
		public csBaseEventHandler
{
public:
  csRef<iEngine> engine;
  csRef<iLoader> loader;
  csRef<iGraphics3D> g3d;
  csRef<iKeyboardDriver> kbd;
  csRef<iVirtualClock> vc;
  csRef<FramePrinter> printer;
  csRef<iVFS> vfs;
  csRef<iCollideSystem> cdsys;
  csRef<iPcDynamicWorld> dynworld;

  bool ParseCommandLine ();

private:
  iDynamicCell* dyncell;
  csRef<iCurvedMeshCreator> curvedMeshCreator;

  csRef<iNature> nature;
  csTicks currentTime;

  csRef<iCelPlLayer> pl;
  csRef<iCelEntity> player;
  csRef<iCelEntity> world;

  csRef<iAssetManager> assetManager;

  iSector* sector;
  iCamera* camera;

  /// Physics.
  csRef<CS::Physics::iPhysicalSystem> dyn;

  AresMenu menu;

  /**
   * Setup everything that needs to be rendered on screen. This routine
   * is called from the event handler in response to a csevFrame
   * broadcast message.
   */
  virtual void Frame ();

  virtual bool OnKeyboard (iEvent &event);
  virtual bool OnMouseMove (iEvent &event);
  virtual bool OnMouseDown (iEvent &event);

  bool PostLoadMap ();

  /// Load a library file with the given VFS path.
  bool LoadLibrary (const char* path, const char* file);

public:
  AppAres ();
  virtual ~AppAres ();

  iDynamicCell* CreateCell (const char* name);

  bool StartGame (const char* filename);

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

