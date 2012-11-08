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
#include "iassetmanager.h"

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
class UIManager;

struct iEditorPlugin;
struct iEditingMode;
struct iCommandHandler;

struct iCelPlLayer;
struct iCelEntity;
struct iMarkerManager;
struct iParameterManager;
struct iMarker;

class AppSelectionListener : public SelectionListener
{
private:
  AppAresEditWX* app;

public:
  AppSelectionListener (AppAresEditWX* app) : app (app) { }
  virtual ~AppSelectionListener () { }
  virtual void SelectionChanged (const csArray<iDynamicObject*>& current_objects);
};

enum
{
  ID_FirstContextItem = wxID_HIGHEST + 10000,
};

struct MenuCommand
{
  csRef<iCommandHandler> target;	// If 0 the command will be send to the current mode.
  csString command;
  csStringID commandID;			// The ID of the command of the menu.
  csString args;
  csString help;
  MenuCommand () : commandID (csInvalidStringID) { }
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

  csRef<iAssetManager> assetManager;

  bool ready;	// True if we are ready to process events.

  csString currentPath;
  csString currentFile;

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

  csRefArray<iEditorPlugin> plugins;
  csRef<iEditingMode> mainMode;
  csString mainModeName;
  csRef<iEditingMode> playMode;
  iEditingMode* editMode;
  CameraWindow* camwin;
  iEditorPlugin* FindPlugin (const char* name);
  void RefreshModes ();

  // For messages.
  int lastMessageSeverity;
  csString lastMessageID;
  csString messages;

  // For comments.
  csWeakRef<iObject> objectForComment;

  csHash<MenuCommand,int> menuCommands;
  bool SetupMenuBar ();
  void OnMenuUpdate (wxUpdateUIEvent& event);
  void OnMenuItem (wxCommandEvent& event);

  void NewProject ();
  void ManageAssets ();
  void ManageResources ();
  void OpenFile ();
  void SaveFile ();
  void SaveCurrentFile ();
  void Quit ();
  void FindObject ();
  void SetCurrentFile (const char* path, const char* file);

  void OnNotebookChange (wxNotebookEvent& event);
  void OnNotebookChanged (wxNotebookEvent& event);
  void OnClearMessages (wxCommandEvent& event);
  void OnChangeComment (wxCommandEvent& event);

  // View the specific bottom page. If toggle is true then the
  // page will close if it was already open.
  void ViewBottomPage (const char* pagename, bool toggle);
  void View3D ();
  void ViewControls ();

public:
  AppAresEditWX (iObjectRegistry* object_reg, int w, int h, long style);
  virtual ~AppAresEditWX ();

  /**
   * Display an error notification.
   * \remarks
   * The error displayed with this function will be identified with the
   * application string name identifier set with SetApplicationName().
   * \sa \ref FormatterNotes
   */
  virtual bool ReportError (const char* description, ...)
  {
    va_list args;
    va_start (args, description);
    csReportV (object_reg, CS_REPORTER_SEVERITY_ERROR,
      "ares", description, args);
    va_end (args);
    return false;
  }

  /**
   * A message was received from the reporter.
   */
  void ReceiveMessage (int severity, const char* msgID, const char* description);

  /// Update the title of this frame.
  virtual void UpdateTitle ();

  /// Set the help status message at the bottom of the frame.
  virtual void SetStatus (const char* statusmsg, ...);
  /// Clear the help status message (go back to default).
  virtual void ClearStatus ();

  /// Append a menu item.
  void AppendMenuItem (wxMenu* menu, int id, const char* label,
       const char* targetName, const char* command, const char* args, const char* help);

  bool Initialize ();
  bool ParseCommandLine ();
  bool InitPlugins ();
  bool InitWX ();
  void PushFrame ();
  void OnClose (wxCloseEvent& event);
  void OnIconize (wxIconizeEvent& event);
  void OnShow (wxShowEvent& event);
  void OnSize (wxSizeEvent& ev);
  void OnIdle (wxIdleEvent& event);
  void SaveFile (const char* filename);
  bool LoadFile (const char* filename);
  void ManageAssets (const csArray<BaseAsset>& assets);

  virtual void RegisterModification (iObject* resource = 0);
  virtual void RegisterModification (const csArray<iObject*>& resources);
  bool IsCleanupAllowed ();

  void SwitchToMode (const char* name);
  virtual void SwitchToMainMode ();
  void SetCurveModeEnabled (bool cm);
  iEditingMode* GetMainMode () const { return mainMode; }
  iEditorConfig* GetConfig () const;

  /**
   * Return true if we are in play mode.
   */
  bool IsPlaying () const;

  void DoFrame ();

  virtual iAssetManager* GetAssetManager () const { return assetManager; }

  AresEdit3DView* GetAresView () const { return aresed3d; }
  iVFS* GetVFS () const { return vfs; }
  virtual iObjectRegistry* GetObjectRegistry () const { return object_reg; }
  iGraphics3D* GetG3D () const { return g3d; }

  virtual void SetMenuState ();

  /// Command handler functions.
  virtual bool Command (csStringID id, const csString& args);
  virtual bool IsCommandValid (csStringID id, const csString& args,
      iSelection* selection, size_t pastesize);
  virtual csPtr<iString> GetAlternativeLabel (csStringID id,
      iSelection* selection, size_t pastesize)
  {
    return 0;
  }

  CameraWindow* GetCameraWindow () const { return camwin; }
  virtual void ShowCameraWindow ();
  virtual void HideCameraWindow ();

  virtual void SetFocus3D ();

  virtual void SetObjectForComment (const char* type, iObject* objForComment);

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
