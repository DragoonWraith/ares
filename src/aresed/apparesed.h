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
#include "include/inature.h"

#include "aresed.h"
#include "filereq.h"
#include "mainmode.h"
#include "curvemode.h"
#include "camera.h"

#define USE_DECAL 0

class CameraWindow;

class CurvedFactoryCreator
{
public:
  csString name;
  float maxradius, imposterradius, mass;
};

class AppAresEdit :
  public csApplicationFramework, public csBaseEventHandler
{
private:
  csRef<iDynamicWorld> dynworld;
  csRef<iCurvedMeshCreator> curvedMeshCreator;
  csRef<iNature> nature;

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
  EditingMode* editMode;

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

  /// A map to offset and size for every factory.
  csHash<float,csString> factory_to_origin_offset;
  /// A set indicating all curved factory creators.
  csArray<CurvedFactoryCreator> curvedFactoryCreators;
  int curvedFactoryCounter;

  CurvedFactoryCreator* FindFactoryCreator (const char* name);

  /// If the factory is in this set then objects of this factory are created
  /// static by default.
  csSet<csString> static_factories;

  /// Categories with items.
  csHash<csStringArray,csString> categories;

  /// Undo stack.
  csRefArray<iDocument> undoStack;
  csStringArray undoOperations;

  /// Debug drawing enabled.
  bool do_debug;

  /// Do simulation.
  bool do_simulation;

  // Selected objects.
  csArray<iDynamicObject*> current_objects;

  /// Create the room.
  bool SetupWorld ();

  /// Setup stuff after map loading.
  bool PostLoadMap ();

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

  csString lastUndoType;

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

  bool SwitchToCurveMode ();

  CEGUI::Checkbox* simulationCheck;
  CEGUI::PushButton* undoButton;
  CEGUI::Window* filenameLabel;
  CEGUI::TabButton* mainTabButton;
  CEGUI::TabButton* curveTabButton;

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
  iCEGUI* GetCEGUI () const { return cegui; }
  iDynamicWorld* GetDynamicWorld () const { return dynworld; }
  iKeyboardDriver* GetKeyboardDriver () const { return kbd; }
  int GetMouseX () const { return mouseX; }
  int GetMouseY () const { return mouseY; }
  int GetViewWidth () const { return view_width; }
  int GetViewHeight () const { return view_height; }

  /// Manipulate the current objects.
  csArray<iDynamicObject*>& GetCurrentObjects () { return current_objects; }
  void SetCurrentObject (iDynamicObject* dynobj);
  void AddCurrentObject (iDynamicObject* dynobj);

  /// Get all categories.
  const csHash<csStringArray,csString>& GetCategories () const { return categories; }

  /// We are about to do an operation, push an undo.
  void PushUndo (const char* type);

  /// Spawn an item.
  void SpawnItem (const csString& name);

  /// Get the camera.
  Camera& GetCamera () { return camera; }

  /**
   * Delete all currently selected objects.
   */
  void DeleteSelectedObjects ();

  /**
   * Align all selected objects based on the first selected object.
   */
  void AlignSelectedObjects ();

  /**
   * Stack all selected objects on top of each other.
   */
  void StackSelectedObjects ();

  /**
   * Move all selected objects to the same heigh.
   */
  void SameYSelectedObjects ();

  /**
   * Set the position of the selected objects so that the
   * are adjacent to each other along the best matching axis.
   */
  void SetPosSelectedObjects ();

  /**
   * Reset the rotation of all selected objects.
   */
  void RotResetSelectedObjects ();

  /**
   * Rotate the current object with the given base angle.
   */
  void RotateCurrent (float baseAngle);

  /**
   * Move the current object with the given base vector.
   */
  void MoveCurrent (const csVector3& baseVector);

  /**
   * Set the static state of the current selected objects.
   */
  void SetStaticSelectedObjects (bool st);

  /**
   * Return the center of all selected objects.
   */
  csVector3 GetCenterSelected ();
  
  /**
   * Return true if there are selected objects.
   */
  bool AreObjectsSelected () const;

  /**
   * Given a screen position, calculate the rigid body at that position.
   */
  iRigidBody* TraceBeam (int x, int y, csVector3& startBeam, csVector3& endBeam,
      csVector3& isect);

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
  csRef<iDocument> SaveDoc ();

  /**
   * Load the world from a file.
   */
  void LoadFile (const char* filename);
  void LoadDoc (iDocument* doc);

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
