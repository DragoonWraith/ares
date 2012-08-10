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
#include "icurvemesh.h"
#include "irooms.h"
#include "inature.h"
#include "editor/i3dview.h"
#include "editor/iapp.h"
#include "editor/icommand.h"
#include "common/worldload.h"

#include "aresed.h"
#include "camera.h"
#include "selection.h"
#include "config.h"

#include "propclass/dynworld.h"
#include "tools/elcm.h"
#include "celtool/ticktimer.h"

#include "ivideo/wxwin.h"
#include "csutil/custom_new_disable.h"
#include <wx/wx.h>
#include <wx/notebook.h>
#include "csutil/custom_new_enable.h"

#define USE_DECAL 0

class CameraWindow;
class AppAresEditWX;
class AresEdit3DView;
class Asset;

class DynfactCollectionValue;
class ObjectsValue;
class NewProjectDialog;
class UIManager;

struct iEditorPlugin;
struct iEditingMode;
struct iCommandHandler;

struct iCelPlLayer;
struct iCelEntity;
struct iMarkerManager;
struct iParameterManager;
struct iMarker;

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
 * The main logic behind the Ares Editor 3D view.
 */
class AresEdit3DView : public scfImplementation1<AresEdit3DView, i3DView>
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
  csRef<Camera> camera;

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
  csRef<ObjectsValue> objectsValue;

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

  /**
   * Setup the dynamic world and add a few standard dynamic factories
   * ('Player' and 'Node').
   */
  bool SetupDynWorld ();

#if USE_DECAL
  /// Setup the decals.
  bool SetupDecal ();
#endif

  /// Load a library file with the given VFS path.
  bool LoadLibrary (const char* path, const char* file);

  csEventID FocusLost;

  int mouseX, mouseY;

  /// Marker used for pasting.
  iMarker* pasteMarker;
  iMarker* constrainMarker;
  csString currentPasteMarkerContext;	// Name of the dynfact mesh currently in pasteMarker.
  int pasteConstrainMode;		// Current paste constrain mode.
  csVector3 pasteConstrain;
  bool gridMode;
  float gridSize;

  /// A paste buffer.
  csArray<PasteContents> pastebuffer;

  /// When there are items in this array we are waiting to spawn stuff.
  csArray<PasteContents> todoSpawn;

  /**
   * Paste the current paste buffer at the mouse position. Usually you
   * would not use this but use StartPasteSelection() instead.
   */
  void PasteSelection ();

  /**
   * Create the paste marker based on the current paste buffer (if needed).
   */
  void CreatePasteMarker ();

  /**
   * Make sure the paste marker is at the correct spot and active.
   */
  void PlacePasteMarker ();

  /**
   * Stop paste mode.
   */
  void StopPasteMode ();

  /**
   * Constrain a transform according to the given mode.
   */
  void ConstrainTransform (csReversibleTransform& tr, int mode, const csVector3& constrain,
      bool grid);

public:
  /**
   * Constructor.
   */
  AresEdit3DView (AppAresEditWX* app, iObjectRegistry* object_reg);

  /**
   * Destructor.
   */
  virtual ~AresEdit3DView ();

  AppAresEditWX* GetApp () const { return app; }

  virtual iAresEditor* GetApplication  ();

  iDynamicCell* CreateCell (const char* name);

  /**
   * Setup the 3D view.
   */
  bool Setup ();

  /**
   * Resize the view.
   */
  void ResizeView (int width, int height);

  iObjectRegistry* GetObjectRegistry () const { return object_reg; }

  void WriteText (const char* buf)
  {
    iGraphics2D* g2d = g3d->GetDriver2D ();
    g2d->Write (font, 200, g2d->GetHeight ()-20, colorWhite, -1, buf);
  }

  void Frame (iEditingMode* editMode);
  bool OnMouseDown(iEvent&);
  bool OnMouseUp(iEvent&);
  bool OnMouseMove (iEvent&);

  virtual bool IsDebugMode () const { return do_debug; }
  virtual void SetDebugMode (bool b) { do_debug = b; }

  virtual bool IsAutoTime () const { return do_auto_time; }
  virtual void SetAutoTime (bool a) { do_auto_time = a; }
  virtual void ModifyCurrentTime (csTicks t) { currentTime += t; }
  virtual csTicks GetCurrentTime () const { return currentTime; }
  virtual bool IsSimulation () const { return do_simulation; }

  virtual void ShowConstrainMarker (bool constrainx, bool constrainy, bool constrainz);
  virtual void MoveConstrainMarker (const csReversibleTransform& trans);
  virtual void HideConstrainMarker ();

  iGraphics3D* GetG3D () const { return g3d; }
  iGraphics2D* GetG2D () const { return g3d->GetDriver2D (); }
  iEngine* GetEngine () const { return engine; }
  virtual iDynamicSystem* GetDynamicSystem () const { return dynSys; }
  virtual CS::Physics::Bullet::iDynamicSystem* GetBulletSystem () const
  { return bullet_dynSys; }
  virtual iCamera* GetCsCamera () const { return view->GetCamera (); }
  iCollideSystem* GetCollisionSystem () const { return cdsys; }
  iCurvedMeshCreator* GetCurvedMeshCreator () const { return curvedMeshCreator; }
  iRoomMeshCreator* GetRoomMeshCreator () const { return roomMeshCreator; }
  virtual iPcDynamicWorld* GetDynamicWorld () const { return dynworld; }
  virtual iDynamicCell* GetDynamicCell () const { return dyncell; }
  iKeyboardDriver* GetKeyboardDriver () const { return kbd; }
  iMarkerManager* GetMarkerManager () const { return markerMgr; }
  iNature* GetNature () const { return nature; }
  iCelPlLayer* GetPL () const { return pl; }
  iParameterManager* GetPM ();
  virtual iELCM* GetELCM () const { return elcm; }

  int GetMouseX () const { return mouseX; }
  int GetMouseY () const { return mouseY; }
  virtual int GetViewWidth () const { return view_width; }
  virtual int GetViewHeight () const { return view_height; }
  virtual iView* GetView () const { return view; }
  virtual iEditorCamera* GetEditorCamera () const;
  virtual iLight* GetCameraLight () const { return camlight; }

  virtual iSelection* GetSelection () const { return selection; }
  virtual Selection* GetSelectionInt () const { return selection; }
  void SelectionChanged (const csArray<iDynamicObject*>& current_objects);

  /// Get all categories.
  const csHash<csStringArray,csString>& GetCategories () const { return categories; }
  /// Get the dynamic factory value.
  virtual Ares::Value* GetDynfactCollectionValue () const;
  /// Get the objects value.
  virtual Ares::Value* GetObjectsValue () const;
  virtual iDynamicObject* GetDynamicObjectFromObjects (Ares::Value* value);
  virtual size_t GetDynamicObjectIndexFromObjects (iDynamicObject* dynobj);

  /// Join two selected objects.
  void JoinObjects ();
  void UnjoinObjects ();

  /// Update all objects (after factory changes).
  void UpdateObjects ();


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
  virtual void ChangeCategory (const char* newCategory, const char* itemname);

  /// Spawn an item. 'trans' is an optional relative transform to use for the new item.
  virtual iDynamicObject* SpawnItem (const csString& name,
      csReversibleTransform* trans = 0);

  /**
   * When the physical properties of a factory change or a new factory is created
   * we need to change various internal settings for this.
   */
  virtual void RefreshFactorySettings (iDynamicFactory* fact);

  /// Warp the camera to another cell.
  void WarpCell (iDynamicCell* cell);

  /// Return where an item would be spawned if we were to spawn it now.
  csReversibleTransform GetSpawnTransformation ();

  /// Get the spawn position for the current camera transform.
  csVector3 GetBeamPosition (const char* fname);

  /// Get the camera.
  Camera* GetCamera () { return camera; }

  /**
   * Enable ragdoll for the selected object. This is a temporary
   * function to experiment with this feature.
   */
  //void EnableRagdoll ();

  /**
   * Delete all currently selected objects.
   */
  void DeleteSelectedObjects ();

  /**
   * Set the static state of the current selected objects.
   */
  virtual void SetStaticSelectedObjects (bool st);

  /**
   * Set the name of the current selected objects (only works for the first
   * selected object).
   */
  virtual void ChangeNameSelectedObject (const char* name);

  /**
   * Calculate a segment representing a beam that starts from camera
   * position towards a given point on screen.
   */
  virtual csSegment3 GetBeam (int x, int y, float maxdist = 1000.0f);
  
  /**
   * Calculate a segment representing a beam that starts from camera
   * position towards a given point on screen as pointed to by the mouse.
   */
  virtual csSegment3 GetMouseBeam (float maxdist = 1000.0f);
  
  /**
   * Given a beam, get the dynamic object at that position.
   */
  virtual iDynamicObject* TraceBeam (const csSegment3& beam, csVector3& isect);

  /**
   * Given a beam, see if something is selected at the position.
   * This can be a dynamic object but also static geometry.
   */
  virtual bool TraceBeamHit (const csSegment3& beam, csVector3& isect);

  /**
   * Hit a beam with the terrain and return the intersection point.
   */
  virtual bool TraceBeamTerrain (const csVector3& start, const csVector3& end,
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
  bool LoadFile (const char* filename);

  /**
   * Create a new project with the given assets.
   */
  void NewProject (const csArray<Asset>& assets);

  void CopySelection ();
  virtual void StartPasteSelection ();
  virtual void StartPasteSelection (const char* name);
  virtual bool IsPasteSelectionActive () const { return todoSpawn.GetSize () > 0; }
  virtual void SetPasteConstrain (int mode);
  virtual int GetPasteConstrain () const { return pasteConstrainMode; }
  virtual bool IsClipboardFull () const { return pastebuffer.GetSize () > 0; }

  virtual void ToggleGridMode ();
  virtual bool IsGridModeEnabled () const { return gridMode; }
  virtual float GetGridSize () const { return gridSize; }
};

enum
{
  ID_FirstContextItem = wxID_HIGHEST + 10000,
};

struct MenuCommand
{
  csRef<iCommandHandler> target;
  csString command;
  csString args;
};

class AppAresEditWX : public wxFrame, public scfImplementation2<AppAresEditWX,
  iAresEditor,iCommandHandler>
{
private:
  iObjectRegistry* object_reg;
  csRef<iGraphics3D> g3d;
  csRef<iVirtualClock> vc;
  csRef<iEngine> engine;
  csRef<iVFS> vfs;
  csRef<iWxWindow> wxwindow;
  csRef<FramePrinter> printer;

  csRef<AresConfig> config;
  csRef<AresEdit3DView> aresed3d;
  csRef<iEditorPlugin> dynfactDialog;

  static bool SimpleEventHandler (iEvent& ev);
  bool HandleEvent (iEvent& ev);

  CS_DECLARE_EVENT_SHORTCUTS;
  csEventID MouseDown;
  csEventID MouseUp;
  csEventID MouseMove;
  csEventID KeyboardDown;
  csEventID FocusLost;

  csRef<UIManager> uiManager;

  int wantsFocus3D;

  NewProjectDialog* newprojectDialog;

  csRefArray<iEditorPlugin> plugins;
  csRef<iEditingMode> mainMode;
  csString mainModeName;
  csRef<iEditingMode> playMode;
  iEditingMode* editMode;
  CameraWindow* camwin;
  iEditorPlugin* FindPlugin (const char* name);
  void RefreshModes ();

  csHash<MenuCommand,int> menuCommands;
  bool SetupMenuBar ();
  void OnMenuItem (wxCommandEvent& event);

  void NewProject ();
  void OpenFile ();
  void SaveFile ();
  void Quit ();
  void FindObject ();

  void OnNotebookChange (wxNotebookEvent& event);
  void OnNotebookChanged (wxNotebookEvent& event);

public:
  AppAresEditWX (iObjectRegistry* object_reg, int w, int h);
  virtual ~AppAresEditWX ();

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
  virtual void SetStatus (const char* statusmsg, ...);
  /// Clear the help status message (go back to default).
  virtual void ClearStatus ();

  /// Append a menu item.
  void AppendMenuItem (wxMenu* menu, int id, const char* label,
       const char* targetName, const char* command, const char* args);

  bool Initialize ();
  bool ParseCommandLine ();
  bool InitPlugins ();
  bool InitWX ();
  void PushFrame ();
  void OnClose (wxCloseEvent& event);
  void OnIconize (wxIconizeEvent& event);
  void OnShow (wxShowEvent& event);
  void OnSize (wxSizeEvent& ev);
  void SaveFile (const char* filename);
  bool LoadFile (const char* filename);
  void NewProject (const csArray<Asset>& assets);

  void SwitchToMode (const char* name);
  virtual void SwitchToMainMode ();
  void SetCurveModeEnabled (bool cm);
  iEditingMode* GetMainMode () const { return mainMode; }
  iEditorConfig* GetConfig () const;

  /**
   * Return true if we are in play mode.
   */
  bool IsPlaying () const;

  void DoFrame () { aresed3d->Frame (editMode); }

  AresEdit3DView* GetAresView () const { return aresed3d; }
  iVFS* GetVFS () const { return vfs; }
  iObjectRegistry* GetObjectRegistry () const { return object_reg; }
  iGraphics3D* GetG3D () const { return g3d; }

  virtual void SetMenuState ();

  /// Command handler functions.
  virtual bool Command (const char* name, const char* args);
  virtual bool IsCommandValid (const char* name, const char* args,
      iSelection* selection, bool haspaste,
      const char* currentmode);

  CameraWindow* GetCameraWindow () const { return camwin; }
  virtual void ShowCameraWindow ();
  virtual void HideCameraWindow ();

  virtual void SetFocus3D ();

  UIManager* GetUIManager () const { return uiManager; }
  virtual iUIManager* GetUI () const;
  virtual i3DView* Get3DView () const;
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
