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

#include <crystalspace.h>
#include "include/icurvemesh.h"
#include "include/irooms.h"
#include "include/inature.h"

#include "aresed.h"
#include "camera.h"
#include "selection.h"

#include "propclass/dynworld.h"
#include "tools/elcm.h"

#include "ivideo/wxwin.h"
#include "csutil/custom_new_disable.h"
#include <wx/wx.h>
#include <wx/notebook.h>
#include "csutil/custom_new_enable.h"

#define USE_DECAL 0

class CameraWindow;
class WorldLoader;
class AppAresEditWX;
class AresEdit3DView;
class Asset;

class DynfactRowModel;
class DynfactCollectionValue;
class NewProjectDialog;
class UIManager;

class EditingMode;
class PlayMode;
class MainMode;
class FoliageMode;
class CurveMode;
class RoomMode;
class EntityMode;

struct iCelPlLayer;
struct iCelEntity;
struct iMarkerManager;
struct iParameterManager;

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
  AresEdit3DView* aresed3d;

public:
  AresEditSelectionListener (AresEdit3DView* aresed3d) : aresed3d (aresed3d) { }
  virtual ~AresEditSelectionListener () { }
  virtual void SelectionChanged (const csArray<iDynamicObject*>& current_objects);
};

class AppSelectionListener : public SelectionListener
{
private:
  AppAresEditWX* app;

public:
  AppSelectionListener (AppAresEditWX* app) : app (app) { }
  virtual ~AppSelectionListener () { }
  virtual void SelectionChanged (const csArray<iDynamicObject*>& current_objects);
};

/**
 * A snapshot of the current objects. This is used to remember the situation
 * before 'Play' is selected.
 */
class DynworldSnapshot
{
private:
  struct Obj
  {
    iDynamicCell* cell;
    iDynamicFactory* fact;
    bool isStatic;
    csReversibleTransform trans;
  };
  csArray<Obj> objects;

public:
  DynworldSnapshot (iPcDynamicWorld* dynworld);
  void Restore (iPcDynamicWorld* dynworld);
};


/**
 * The main logic behind the Ares Editor 3D view.
 */
class AresEdit3DView
{
private:
  AppAresEditWX* app;
  iObjectRegistry* object_reg;
  csRef<iPcDynamicWorld> dynworld;
  csRef<iELCM> elcm;
  iDynamicCell* dyncell;
  csRef<iCurvedMeshCreator> curvedMeshCreator;
  csRef<iRoomMeshCreator> roomMeshCreator;
  csRef<iNature> nature;
  csRef<iMarkerManager> markerMgr;

  DynworldSnapshot* snapshot;

  csRef<iGraphics3D> g3d;
  csRef<iKeyboardDriver> kbd;
  csRef<iEngine> engine;
  csRef<iLoader> loader;
  csRef<iVirtualClock> vc;
  csRef<iEventQueue> eventQueue;
  csRef<iVFS> vfs;
  csRef<iDecalManager> decalMgr;
  csRef<iParameterManager> pm;

  csRef<iCelPlLayer> pl;
  csRef<iCelEntity> zoneEntity;

#if USE_DECAL
  csRef<iDecalTemplate> cursorDecalTemplate;
  iDecal* cursorDecal;
#endif

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

  /// A pointer to the view which contains the camera.
  csRef<iView> view;
  int view_width;
  int view_height;

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
  csRef<DynfactCollectionValue> dynfactCollectionValue;

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

  csEventID FocusLost;

  int mouseX, mouseY;

  /**
   * Initialize physics.
   */
  bool InitPhysics ();

public:
  /**
   * Constructor.
   */
  AresEdit3DView (AppAresEditWX* app, iObjectRegistry* object_reg);

  /**
   * Destructor.
   */
  ~AresEdit3DView ();

  AppAresEditWX* GetApp () const { return app; }

  /**
   * Setup the 3D view.
   */
  bool Setup ();

  /**
   * Resize the view.
   */
  void ResizeView (int width, int height);

  iObjectRegistry* GetObjectRegistry () const { return object_reg; }
  
  /**
   * Do a test play of the game.
   */
  void Play ();

  /**
   * Exit play testing and restore the editing world.
   */
  void ExitPlay ();

  /**
   * Return true if we are in play mode.
   */
  bool IsPlaying () const { return snapshot != 0; }

  /**
   * Handle all the 3D related stuff like nature, camera, camera light,
   * physics simulation, ...
   */
  void Do3DPreFrameStuff ();

  void WriteText (const char* buf)
  {
    iGraphics2D* g2d = g3d->GetDriver2D ();
    g2d->Write (font, 200, g2d->GetHeight ()-20, colorWhite, -1, buf);
  }

  void Frame (EditingMode* editMode);
  bool OnMouseDown(iEvent&);
  bool OnMouseUp(iEvent&);
  bool OnMouseMove (iEvent&);

  bool IsDebugMode () const { return do_debug; }
  void SetDebugMode (bool b) { do_debug = b; }

  bool IsAutoTime () const { return do_auto_time; }
  void SetAutoTime (bool a) { do_auto_time = a; }
  void ModifyCurrentTime (csTicks t) { currentTime += t; }

  iGraphics3D* GetG3D () const { return g3d; }
  iGraphics2D* GetG2D () const { return g3d->GetDriver2D (); }
  iEngine* GetEngine () const { return engine; }
  CS::Physics::Bullet::iDynamicSystem* GetBulletSystem () const { return bullet_dynSys; }
  iCamera* GetCsCamera () const { return view->GetCamera (); }
  iCollideSystem* GetCollisionSystem () const { return cdsys; }
  iCurvedMeshCreator* GetCurvedMeshCreator () const { return curvedMeshCreator; }
  iRoomMeshCreator* GetRoomMeshCreator () const { return roomMeshCreator; }
  iPcDynamicWorld* GetDynamicWorld () const { return dynworld; }
  iDynamicCell* GetDynamicCell () const { return dyncell; }
  iKeyboardDriver* GetKeyboardDriver () const { return kbd; }
  iMarkerManager* GetMarkerManager () const { return markerMgr; }
  iNature* GetNature () const { return nature; }
  iCelPlLayer* GetPL () const { return pl; }
  iParameterManager* GetPM ();

  int GetMouseX () const { return mouseX; }
  int GetMouseY () const { return mouseY; }
  int GetViewWidth () const { return view_width; }
  int GetViewHeight () const { return view_height; }
  iView* GetView () const { return view; }

  Selection* GetSelection () const { return selection; }
  void SelectionChanged (const csArray<iDynamicObject*>& current_objects);

  /// Get all categories.
  const csHash<csStringArray,csString>& GetCategories () const { return categories; }
  /// Get the dynamic factory value.
  DynfactCollectionValue* GetDynfactCollectionValue () const { return dynfactCollectionValue; }

  /// Clear all items and categories.
  void ClearItems () { categories.DeleteAll (); }
  /// Add an item to a category (create the category if not already present).
  void AddItem (const char* category, const char* itemname);
  /// Remove an item.
  void RemoveItem (const char* category, const char* itemname);
  /**
   * Move an item to another category. If the item doesn't already exist
   * then this is equivalent to calling AddItem().
   */
  void ChangeCategory (const char* newCategory, const char* itemname);

  /// Spawn an item. 'trans' is an optional relative transform to use for the new item.
  iDynamicObject* SpawnItem (const csString& name, csReversibleTransform* trans = 0);

  /// Warp the camera to another cell.
  void WarpCell (iDynamicCell* cell);

  /// Return where an item would be spawned if we were to spawn it now.
  csReversibleTransform GetSpawnTransformation (const csString& name, csReversibleTransform* trans = 0);

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
  void OnExit();

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
   * Create a new project with the given assets.
   */
  void NewProject (const csArray<Asset>& assets);
};

enum
{
  ID_Quit = wxID_EXIT,
  ID_Open = wxID_OPEN,
  ID_Save = wxID_SAVE,
  ID_New = wxID_NEW,
  ID_Copy = wxID_COPY,
  ID_Paste = wxID_PASTE,
  ID_Delete = wxID_HIGHEST + 1000,
  ID_Cells,
  ID_Dynfacts,
  ID_Play,
  ID_FirstContextItem = wxID_HIGHEST + 10000,
};

class AppAresEditWX : public wxFrame
{
private:
  iObjectRegistry* object_reg;
  csRef<iGraphics3D> g3d;
  csRef<iVirtualClock> vc;
  csRef<iEngine> engine;
  csRef<iVFS> vfs;
  csRef<iWxWindow> wxwindow;
  csRef<FramePrinter> printer;

  AresEdit3DView* aresed3d;

  static bool SimpleEventHandler (iEvent& ev);
  bool HandleEvent (iEvent& ev);

  CS_DECLARE_EVENT_SHORTCUTS;
  csEventID MouseDown;
  csEventID MouseUp;
  csEventID MouseMove;
  csEventID KeyboardDown;
  csEventID FocusLost;

  UIManager* uiManager;

  NewProjectDialog* newprojectDialog;
  PlayMode* playMode;
  MainMode* mainMode;
  CurveMode* curveMode;
  RoomMode* roomMode;
  FoliageMode* foliageMode;
  EntityMode* entityMode;
  EditingMode* editMode;
  CameraWindow* camwin;

  void SetupMenuBar ();

  void OnMenuNew (wxCommandEvent& event);
  void OnMenuCells (wxCommandEvent& event);
  void OnMenuDynfacts (wxCommandEvent& event);
  void OnMenuPlay (wxCommandEvent& event);
  void OnMenuOpen (wxCommandEvent& event);
  void OnMenuSave (wxCommandEvent& event);
  void OnMenuQuit (wxCommandEvent& event);
  void OnMenuDelete (wxCommandEvent& event);
  void OnMenuCopy (wxCommandEvent& event);
  void OnMenuPaste (wxCommandEvent& event);
  void OnNotebookChange (wxNotebookEvent& event);
  void OnNotebookChanged (wxNotebookEvent& event);

public:
  AppAresEditWX (iObjectRegistry* object_reg);
  ~AppAresEditWX ();

  /**
   * Display an error notification.
   * \remarks
   * The error displayed with this function will be identified with the
   * application string name identifier set with SetApplicationName().
   * \sa \ref FormatterNotes
   */
  bool ReportError (const char* description, ...)
  {
    va_list args;
    va_start (args, description);
    csReportV (object_reg, CS_REPORTER_SEVERITY_ERROR,
      "ares", description, args);
    va_end (args);
    return false;
  }

  /// Set the help status message at the bottom of the frame.
  void SetStatus (const char* statusmsg, ...);
  /// Clear the help status message (go back to default).
  void ClearStatus ();

  bool Initialize ();
  bool InitWX ();
  void PushFrame ();
  void OnClose (wxCloseEvent& event);
  void OnIconize (wxIconizeEvent& event);
  void OnShow (wxShowEvent& event);
  void OnSize (wxSizeEvent& ev);
  void SaveFile (const char* filename);
  void LoadFile (const char* filename);
  void NewProject (const csArray<Asset>& assets);

  void SwitchToPlayMode ();
  void SwitchToMainMode ();
  void SwitchToCurveMode ();
  void SwitchToRoomMode ();
  void SwitchToFoliageMode ();
  void SwitchToEntityMode ();
  void SetCurveModeEnabled (bool cm);
  MainMode* GetMainMode () const { return mainMode; }

  void DoFrame () { aresed3d->Frame (editMode); }

  AresEdit3DView* GetAresView () const { return aresed3d; }
  iVFS* GetVFS () const { return vfs; }
  iObjectRegistry* GetObjectRegistry () const { return object_reg; }

  void SetMenuState ();

  CameraWindow* GetCameraWindow () const { return camwin; }
  UIManager* GetUIManager () const { return uiManager; }
  iVirtualClock* GetVC () const { return vc; }
  iEngine* GetEngine () const { return engine; }

  bool LoadResourceFile (const char* filename, wxString& searchPath);

  DECLARE_EVENT_TABLE ();

  class Panel : public wxPanel
  {
  public:
    Panel(wxWindow* parent, AppAresEditWX* s)
      : wxPanel (parent, wxID_ANY, wxDefaultPosition, wxDefaultSize), s (s)
    {}
    
    virtual void OnSize (wxSizeEvent& ev)
    { s->OnSize (ev); }

  private:
    AppAresEditWX* s;

    DECLARE_EVENT_TABLE()
  };
};

#endif // __apparesed_h
