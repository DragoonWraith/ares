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

#ifndef __apparesed_h
#define __apparesed_h

#include <CEGUI.h>
#include <crystalspace.h>
#include <ivaria/icegui.h>
#include "include/idynworld.h"
#include "include/icurvemesh.h"
#include "include/irooms.h"
#include "include/inature.h"
#include "include/imarker.h"

#include "aresed.h"
#include "filereq.h"
#include "mainmode.h"
#include "curvemode.h"
//#include "roommode.h"
#include "camera.h"
#include "selection.h"

#define USE_DECAL 0

class CameraWindow;
class WorldLoader;
class AppAresEdit;

class CurvedFactoryCreator
{
public:
  csString name;
  float maxradius, imposterradius, mass;
};

class RoomFactoryCreator
{
public:
  csString name;
};

class AresEditSelectionListener : public SelectionListener
{
private:
  AppAresEdit* aresed;

public:
  AresEditSelectionListener (AppAresEdit* aresed) : aresed (aresed) { }
  virtual void SelectionChanged (const csArray<iDynamicObject*>& current_objects);
};

class AppAresEdit :
  public csApplicationFramework, public csBaseEventHandler
{
private:
  csRef<iDynamicWorld> dynworld;
  csRef<iCurvedMeshCreator> curvedMeshCreator;
  csRef<iRoomMeshCreator> roomMeshCreator;
  csRef<iNature> nature;
  csRef<iMarkerManager> markerMgr;

  csRef<iGraphics3D> g3d;
  csRef<iKeyboardDriver> kbd;
  csRef<iEngine> engine;
  csRef<iLoader> loader;
  csRef<iVirtualClock> vc;
  csRef<iVFS> vfs;
  csRef<iDecalManager> decalMgr;

#if USE_DECAL
  csRef<iDecalTemplate> cursorDecalTemplate;
  iDecal* cursorDecal;
#endif

  MainMode* mainMode;
  CurveMode* curveMode;
  //RoomMode* roomMode;
  EditingMode* editMode;

  WorldLoader* worldLoader;

  csTicks currentTime;
  bool do_auto_time;

  int colorWhite;
  csRef<iFont> font;

  /// Physics.
  csRef<iDynamics> dyn;
  csRef<iDynamicSystem> dynSys;
  csRef<CS::Physics::Bullet::iDynamicSystem> bullet_dynSys;

  /// A pointer to the collision detection system.
  csRef<iCollideSystem> cdsys;

  /// Our window system.
  csRef<iCEGUI> cegui;

  /// A pointer to the view which contains the camera.
  csRef<iView> view;
  int view_width;
  int view_height;

  /// A pointer to the frame printer.
  csRef<FramePrinter> printer;

  /// A pointer to the configuration manager.
  csRef<iConfigManager> cfgmgr;

  /// A pointer to the sector the camera will be in.
  iSector* sector;

  /// The player has a flashlight.
  csRef<iLight> camlight;

  /// The terrain mesh.
  iMeshWrapper* terrainMesh;

  /// Camera.
  Camera camera;

  /**
   * A list with all curved factories which are generated indirectly through
   * the curved mesh generator.
   */
  csArray<iDynamicFactory*> curvedFactories;

  /**
   * A list with all curved factories which are generated indirectly through
   * the room mesh generator.
   */
  csArray<iDynamicFactory*> roomFactories;

  /// A map to offset and size for every factory.
  csHash<float,csString> factory_to_origin_offset;
  /// A set indicating all curved factory creators.
  csArray<CurvedFactoryCreator> curvedFactoryCreators;
  int curvedFactoryCounter;
  /// A set indicating all room factory creators.
  csArray<RoomFactoryCreator> roomFactoryCreators;
  int roomFactoryCounter;

  CurvedFactoryCreator* FindFactoryCreator (const char* name);
  RoomFactoryCreator* FindRoomFactoryCreator (const char* name);

  /// If the factory is in this set then objects of this factory are created
  /// static by default.
  csSet<csString> static_factories;

  /// Categories with items.
  csHash<csStringArray,csString> categories;

  /// Debug drawing enabled.
  bool do_debug;

  /// Do simulation.
  bool do_simulation;

  /// The selection.
  Selection* selection;

  /// Create the room.
  bool SetupWorld ();

  /// Setup stuff after map loading.
  bool PostLoadMap ();

  bool SetupDynWorld ();

#if USE_DECAL
  /// Setup the decals.
  bool SetupDecal ();
#endif

  /// Load a library file with the given VFS path.
  bool LoadLibrary (const char* path, const char* file);

  virtual void Frame();
  virtual bool OnKeyboard(iEvent&);
  virtual bool OnMouseDown(iEvent&);
  virtual bool OnMouseUp(iEvent&);
  virtual bool OnMouseMove (iEvent&);
  virtual bool OnUnhandledEvent (iEvent&);

  csEventID FocusLost;

  int mouseX, mouseY;

  /**
   * This method is called by Frame ().
   * It was separated so it's easy to remove or customize it.
   */
  void DoStuffOncePerFrame ();

  /**
   * Initialize physics.
   */
  bool InitPhysics ();

  /**
   * Initialize window system.
   */
  bool InitWindowSystem ();

  /// Set the state of the tabs buttons based on selected objects.
  void SetButtonState ();

  /// Add an item to a category (create the category if not already present).
  void AddItem (const char* category, const char* itemname);

  bool OnUndoButtonClicked (const CEGUI::EventArgs&);
  bool OnSaveButtonClicked (const CEGUI::EventArgs&);
  bool OnLoadButtonClicked (const CEGUI::EventArgs&);
  bool OnSimulationSelected (const CEGUI::EventArgs&);
  bool OnMainTabButtonClicked (const CEGUI::EventArgs&);
  bool OnCurveTabButtonClicked (const CEGUI::EventArgs&);
  //bool OnRoomTabButtonClicked (const CEGUI::EventArgs&);

  bool SwitchToCurveMode ();
  //bool SwitchToRoomMode ();

  CEGUI::Checkbox* simulationCheck;
  CEGUI::PushButton* undoButton;
  CEGUI::Window* filenameLabel;
  CEGUI::TabButton* mainTabButton;
  CEGUI::TabButton* curveTabButton;
  //CEGUI::TabButton* roomTabButton;

  FileReq* filereq;
  CameraWindow* camwin;

public:
  /**
   * Constructor.
   */
  AppAresEdit();

  /**
   * Destructor.
   */
  virtual ~AppAresEdit();

  iGraphics3D* GetG3D () const { return g3d; }
  iGraphics2D* GetG2D () const { return g3d->GetDriver2D (); }
  iEngine* GetEngine () const { return engine; }
  CS::Physics::Bullet::iDynamicSystem* GetBulletSystem () const { return bullet_dynSys; }
  iCamera* GetCsCamera () const { return view->GetCamera (); }
  iCollideSystem* GetCollisionSystem () const { return cdsys; }
  iCurvedMeshCreator* GetCurvedMeshCreator () const { return curvedMeshCreator; }
  iRoomMeshCreator* GetRoomMeshCreator () const { return roomMeshCreator; }
  iCEGUI* GetCEGUI () const { return cegui; }
  iDynamicWorld* GetDynamicWorld () const { return dynworld; }
  iKeyboardDriver* GetKeyboardDriver () const { return kbd; }
  iMarkerManager* GetMarkerManager () const { return markerMgr; }

  int GetMouseX () const { return mouseX; }
  int GetMouseY () const { return mouseY; }
  int GetViewWidth () const { return view_width; }
  int GetViewHeight () const { return view_height; }

  Selection* GetSelection () const { return selection; }
  void SelectionChanged (const csArray<iDynamicObject*>& current_objects);

  /// Get all categories.
  const csHash<csStringArray,csString>& GetCategories () const { return categories; }

  /// Spawn an item.
  void SpawnItem (const csString& name);

  /// Get the camera.
  Camera& GetCamera () { return camera; }

  /**
   * Delete all currently selected objects.
   */
  void DeleteSelectedObjects ();

  /**
   * Set the static state of the current selected objects.
   */
  void SetStaticSelectedObjects (bool st);

  /**
   * Calculate a segment representing a beam that starts from camera
   * position towards a given point on screen.
   */
  csSegment3 GetBeam (int x, int y, float maxdist = 1000.0f);
  
  /**
   * Calculate a segment representing a beam that starts from camera
   * position towards a given point on screen as pointed to by the mouse.
   */
  csSegment3 GetMouseBeam (float maxdist = 1000.0f);
  
  /**
   * Given a beam, calculate the rigid body at that position.
   */
  iRigidBody* TraceBeam (const csSegment3& beam, csVector3& isect);

  /**
   * Hit a beam with the terrain and return the intersection point.
   */
  bool TraceBeamTerrain (const csVector3& start, const csVector3& end,
      csVector3& isect);

  /**
   * Final cleanup.
   */
  virtual void OnExit();

  /**
   * Clean up the current world.
   */
  void CleanupWorld ();

  /**
   * Save the current world.
   */
  void SaveFile (const char* filename);

  /**
   * Load the world from a file.
   */
  void LoadFile (const char* filename);

  /**
   * Main initialization routine.  This routine should set up basic facilities
   * (such as loading startup-time plugins, etc.).  In case of failure this
   * routine will return false.  You can assume that the error message has been
   * reported to the user.
   */
  virtual bool OnInitialize(int argc, char* argv[]);

  /**
   * Run the application.  Performs additional initialization (if needed), and
   * then fires up the main run/event loop.  The loop will fire events which
   * actually causes Crystal Space to "run".  Only when the program exits does
   * this function return.
   */
  virtual bool Application();
  
  CS_EVENTHANDLER_NAMES("application.ares")

  /* Declare that we want to receive events *after* the CEGUI plugin. */
  virtual const csHandlerID * GenericPrec (csRef<iEventHandlerRegistry> &r1, 
    csRef<iEventNameRegistry> &r2, csEventID event) const 
  {
    static csHandlerID precConstraint[2];
    
    precConstraint[0] = r1->GetGenericID("crystalspace.cegui");
    precConstraint[1] = CS_HANDLERLIST_END;
    return precConstraint;
  }

  CS_EVENTHANDLER_DEFAULT_INSTANCE_CONSTRAINTS
};

#endif // __apparesed_h
