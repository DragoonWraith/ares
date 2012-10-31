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

#include "apparesed.h"
#include "imap/objectcomment.h"
#include "cstool/objectcomment.h"

#include "aresview.h"
#include "include/imarker.h"
#include "ui/uimanager.h"
#include "ui/filereq.h"
#include "ui/newproject.h"
#include "ui/celldialog.h"
#include "ui/objectfinder.h"
#include "ui/resourcemover.h"
#include "camera.h"
#include "editor/imode.h"
#include "editor/iplugin.h"

#include "celtool/initapp.h"
#include "cstool/simplestaticlighter.h"
#include "celtool/persisthelper.h"
#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/parameters.h"

#include <csgeom/math3d.h>
#include "camerawin.h"
#include "selection.h"
#include "models/dynfactmodel.h"
#include "models/objects.h"
#include "models/assets.h"
#include "edcommon/transformtools.h"


/* Fun fact: should occur after csutil/event.h, otherwise, gcc may report
 * missing csMouseEventHelper symbols. */
#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/notebook.h>
#include <wx/splitter.h>
#include <wx/xrc/xmlres.h>
#include <wx/html/htmlwin.h>

// Defined in mingw includes apparently.
#undef SetJob

static size_t FindNotebookPage (wxNotebook* notebook, const char* name)
{
  wxString iname = wxString::FromUTF8 (name);
  for (size_t i = 0 ; i < notebook->GetPageCount () ; i++)
  {
    wxString pageName = notebook->GetPageText (i);
    if (pageName == iname) return i;
  }
  return csArrayItemNotFound;
}

void AppSelectionListener::SelectionChanged (
    const csArray<iDynamicObject*>& current_objects)
{
  app->SetMenuState ();
  app->GetMainMode ()->CurrentObjectsChanged (current_objects);
}

struct ManageAssetsCallbackImp : public NewProjectCallback
{
  AppAresEditWX* ares;
  ManageAssetsCallbackImp (AppAresEditWX* ares) : ares (ares) { }
  virtual ~ManageAssetsCallbackImp () { }
  virtual void OkPressed (const csArray<BaseAsset>& assets)
  {
    ares->ManageAssets (assets);
  }
};

struct LoadCallback : public OKCallback
{
  AppAresEditWX* ares;
  LoadCallback (AppAresEditWX* ares) : ares (ares) { }
  virtual ~LoadCallback () { }
  virtual void OkPressed (const char* filename)
  {
    ares->LoadFile (filename);
  }
};

struct SaveCallback : public OKCallback
{
  AppAresEditWX* ares;
  SaveCallback (AppAresEditWX* ares) : ares (ares) { }
  virtual ~SaveCallback () { }
  virtual void OkPressed (const char* filename)
  {
    ares->SaveFile (filename);
  }
};

class AresReporterListener :
  public ThreadedCallable<AresReporterListener>,
  public scfImplementation1<AresReporterListener,iReporterListener>
{
private:
  AppAresEditWX* app;
  iObjectRegistry* GetObjectRegistry() const { return app->GetObjectRegistry (); }

public:
  AresReporterListener (AppAresEditWX* app) : scfImplementationType (this), app (app) { }
  virtual ~AresReporterListener () { }

  THREADED_CALLABLE_DECL4(AresReporterListener, Report, csThreadReturn,
    iReporter*, reporter, int, severity, const char*, msgId, const char*,
    description, HIGH, false, false);
};


THREADED_CALLABLE_IMPL4(AresReporterListener, Report, iReporter*, int severity,
  const char* msgID, const char* description)
{
  app->ReceiveMessage (severity, msgID, description);
  return true;
}

// =========================================================================

BEGIN_EVENT_TABLE(AppAresEditWX, wxFrame)
  EVT_SHOW (AppAresEditWX::OnShow)
  EVT_ICONIZE (AppAresEditWX::OnIconize)
  EVT_UPDATE_UI (wxID_ANY, AppAresEditWX :: OnMenuUpdate)
  EVT_MENU (wxID_ANY, AppAresEditWX :: OnMenuItem)
  EVT_NOTEBOOK_PAGE_CHANGING (XRCID("mainNotebook"), AppAresEditWX :: OnNotebookChange)
  EVT_NOTEBOOK_PAGE_CHANGED (XRCID("mainNotebook"), AppAresEditWX :: OnNotebookChanged)
  EVT_BUTTON (XRCID("messagesClear_Button"), AppAresEditWX :: OnClearMessages)
  EVT_TEXT (XRCID("comments_Text"), AppAresEditWX :: OnChangeComment)
  EVT_IDLE (AppAresEditWX::OnIdle)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(AppAresEditWX::Panel, wxPanel)
  EVT_SIZE(AppAresEditWX::Panel::OnSize)
END_EVENT_TABLE()

static csStringID ID_NewProject = csInvalidStringID;
static csStringID ID_About = csInvalidStringID;
static csStringID ID_ManageAssets = csInvalidStringID;
static csStringID ID_ManageResources = csInvalidStringID;
static csStringID ID_Open = csInvalidStringID;
static csStringID ID_Save = csInvalidStringID;
static csStringID ID_SaveAs = csInvalidStringID;
static csStringID ID_Exit = csInvalidStringID;
static csStringID ID_UpdateObjects = csInvalidStringID;
static csStringID ID_FindObjectDialog = csInvalidStringID;
static csStringID ID_ConvertPhysics = csInvalidStringID;
static csStringID ID_ConvertOpcode = csInvalidStringID;
static csStringID ID_Join = csInvalidStringID;
static csStringID ID_Unjoin = csInvalidStringID;
static csStringID ID_ManageCells = csInvalidStringID;
static csStringID ID_SwitchMode = csInvalidStringID;
static csStringID ID_EntityParameters = csInvalidStringID;
static csStringID ID_ViewMessages = csInvalidStringID;
static csStringID ID_ViewComments = csInvalidStringID;
static csStringID ID_View3D = csInvalidStringID;
static csStringID ID_ViewControls = csInvalidStringID;

// The global pointer to AresEd
AppAresEditWX* aresed = 0;

AppAresEditWX::AppAresEditWX (iObjectRegistry* object_reg, int w, int h)
  : wxFrame (0, -1, wxT ("AresEd"), wxDefaultPosition, wxSize (w, h)),
    scfImplementationType (this), ready (false)
{
  AppAresEditWX::object_reg = object_reg;
  camwin = 0;
  editMode = 0;
  newprojectDialog = 0;
  //oldPageIdx = csArrayItemNotFound;
  FocusLost = csevFocusLost (object_reg);
  config.AttachNew (new AresConfig (this));
  wantsFocus3D = 0;

  lastMessageSeverity = -1;
}

AppAresEditWX::~AppAresEditWX ()
{
printf ("DESTROY AppAresEditWX\n"); fflush (stdout);
  delete camwin;
}

iUIManager* AppAresEditWX::GetUI () const
{
  return static_cast<iUIManager*> (uiManager);
}

i3DView* AppAresEditWX::Get3DView () const
{
  return static_cast<i3DView*> (aresed3d);
}

void AppAresEditWX::FindObject ()
{
  aresed3d->GetModelRepository ()->RefreshObjectsValue ();
  uiManager->GetObjectFinderDialog ()->Show ();
}

void AppAresEditWX::RefreshModes ()
{
  for (size_t i = 0 ; i < plugins.GetSize () ; i++)
  {
    csRef<iEditingMode> mode = scfQueryInterface<iEditingMode> (plugins.Get (i));
    if (mode)
      mode->Refresh ();
  }
  UpdateTitle ();
  aresed3d->GetModelRepository ()->Refresh ();
}

void AppAresEditWX::ManageAssets (const csArray<BaseAsset>& assets)
{
  if (!assetManager->UpdateAssets (assets))
  {
    //@@@ Check? aresed3d->PostLoadMap ();
    uiManager->Error ("Error updating assets!");
    return;
  }
  aresed3d->PostLoadMap ();
  RefreshModes ();
}

bool AppAresEditWX::LoadFile (const char* filename)
{
  if (!aresed3d->SetupWorld ())
    return false;

  SetCurrentFile (vfs->GetCwd (), filename);

  if (!assetManager->LoadFile (currentFile))
  {
    aresed3d->PostLoadMap ();
    uiManager->Error ("Error loading file '%s' on path '%s'!", currentFile.GetData (),
	currentPath.GetData ());
    return false;
  }
  if (!aresed3d->PostLoadMap ())
    return false;
  RefreshModes ();
  return true;
}

void AppAresEditWX::SaveFile (const char* filename)
{
  SetCurrentFile (vfs->GetCwd (), filename);
  if (!assetManager->SaveFile (filename))
  {
    uiManager->Error ("Error saving file '%s' on path '%s'!", filename, currentPath.GetData ());
    return;
  }
  RefreshModes ();
}

void AppAresEditWX::ManageAssets ()
{
  csRef<ManageAssetsCallbackImp> cb;
  cb.AttachNew (new ManageAssetsCallbackImp (this));
  uiManager->GetNewProjectDialog ()->Show (cb, assetManager->GetAssets ());
}

void AppAresEditWX::ManageResources ()
{
  uiManager->GetResourceMoverDialog ()->Show ();
  UpdateTitle ();
}

bool AppAresEditWX::IsCleanupAllowed ()
{
  if (assetManager->IsModified ())
  {
    return uiManager->Ask ("The current project was modified. Do you want to abandon changes?");
  }
  return true;
}

void AppAresEditWX::NewProject ()
{
  if (!IsCleanupAllowed ()) return;

  SetCurrentFile ("", "");

  aresed3d->SetupWorld ();
  assetManager->NewProject ();
  aresed3d->PostLoadMap ();

  RefreshModes ();
}

bool AppAresEditWX::IsPlaying () const
{
  return editMode == playMode;
}

void AppAresEditWX::DoFrame ()
{
  aresed3d->Frame (editMode);
}

void AppAresEditWX::OpenFile ()
{
  if (!IsCleanupAllowed ()) return;

  if (currentPath.IsEmpty ())
    vfs->ChDir ("/saves");
  else
    vfs->ChDir (currentPath);
  uiManager->GetFileReqDialog ()->SetPath (vfs->GetCwd ());
  csRef<LoadCallback> cb;
  cb.AttachNew (new LoadCallback (this));
  uiManager->GetFileReqDialog ()->Show (cb);
}

void AppAresEditWX::SaveFile ()
{
  if (currentPath.IsEmpty ())
    vfs->ChDir ("/saves");
  else
    vfs->ChDir (currentPath);
  uiManager->GetFileReqDialog ()->SetPath (vfs->GetCwd ());
  csRef<SaveCallback> cb;
  cb.AttachNew (new SaveCallback (this));
  uiManager->GetFileReqDialog ()->Show (cb);
}

void AppAresEditWX::SetCurrentFile (const char* path, const char* file)
{
  currentFile = file;
  currentPath = path;
  UpdateTitle ();
}

void AppAresEditWX::UpdateTitle ()
{
  csRef<iConfigManager> cfgmgr = csQueryRegistry<iConfigManager> (object_reg);
  csString title = cfgmgr->GetStr ("WindowTitle", "Please set WindowTitle in AppAresEdit.cfg");
  if (!currentFile.IsEmpty ())
    title.AppendFmt (": %s%s", currentPath.GetData (), currentFile.GetData ());
  if (assetManager && assetManager->IsModified ())
    title += "*";
  SetTitle (wxString::FromUTF8 (title));
}

void AppAresEditWX::SaveCurrentFile ()
{
  if (currentFile.IsEmpty ())
    SaveFile ();
  else
  {
    vfs->PushDir (currentPath);
    SaveFile (currentFile);
    vfs->PopDir ();
  }
}

void AppAresEditWX::Quit ()
{
  if (!IsCleanupAllowed ()) return;
  Close();
}

void AppAresEditWX::OnNotebookChange (wxNotebookEvent& event)
{
  //oldPageIdx = event.GetOldSelection ();
}

void AppAresEditWX::OnNotebookChanged (wxNotebookEvent& event)
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  int pageIdx = event.GetSelection ();
  wxString pageName = notebook->GetPageText (pageIdx);

  csRef<iEditingMode> newMode;
  csRef<iEditorPlugin> plugin = FindPlugin (pageName.mb_str (wxConvUTF8));
  if (!plugin) return;
  newMode = scfQueryInterface<iEditingMode> (plugin);
  if (!newMode) return;

  if (editMode != newMode)
  {
    if (editMode) editMode->Stop ();
    editMode = newMode;
    editMode->Start ();
  }
  SetMenuState ();

  //if (page && !page->IsEnabled () && oldPageIdx != csArrayItemNotFound)
  //{
    //printf ("REVERT %d\n", oldPageIdx);
    //notebook->ChangeSelection (oldPageIdx);
  //}
  //oldPageIdx = csArrayItemNotFound;
}

bool AppAresEditWX::HandleEvent (iEvent& ev)
{
  if (ev.Name == Frame)
  {
    if (editMode) editMode->FramePre ();
    if (aresed3d) DoFrame ();
    return true;
  }
  else if (CS_IS_KEYBOARD_EVENT(object_reg, ev))
  {
    utf32_char code = csKeyEventHelper::GetCookedCode (&ev);
    if (csKeyEventHelper::GetEventType (&ev) == csKeyEventTypeDown)
      return editMode->OnKeyboard (ev, code);
  }
  else if ((ev.Name == MouseMove))
  {
    int mouseX = csMouseEventHelper::GetX (&ev);
    int mouseY = csMouseEventHelper::GetY (&ev);
    if (aresed3d)
    {
      if (!IsPlaying () && aresed3d->OnMouseMove (ev))
	return true;
      else
	return editMode->OnMouseMove (ev, mouseX, mouseY);
    }
  }
  else if ((ev.Name == MouseDown))
  {
    uint but = csMouseEventHelper::GetButton (&ev);
    int mouseX = csMouseEventHelper::GetX (&ev);
    int mouseY = csMouseEventHelper::GetY (&ev);
    if (aresed3d)
    {
      if (!IsPlaying () &&
	  but == csmbRight &&
	  editMode->IsContextMenuAllowed () &&
	  !aresed3d->GetPaster ()->IsPasteSelectionActive ())
      {
	wxMenu contextMenu;
	bool camwinVis = camwin->IsVisible ();
	if (camwinVis)
	{
	  camwin->AddContextMenu (&contextMenu, mouseX, mouseY);
	}
	editMode->AddContextMenu (&contextMenu, mouseX, mouseY);

	PopupMenu (&contextMenu);
      }
      else
      {
        if (!IsPlaying () && aresed3d->OnMouseDown (ev))
	  return true;
        else
	  return editMode->OnMouseDown (ev, but, mouseX, mouseY);
      }
    }
  }
  else if ((ev.Name == MouseUp))
  {
    uint but = csMouseEventHelper::GetButton (&ev);
    int mouseX = csMouseEventHelper::GetX (&ev);
    int mouseY = csMouseEventHelper::GetY (&ev);
    if (aresed3d)
    {
      if (!IsPlaying () && aresed3d->OnMouseUp (ev))
	return true;
      else
	return editMode->OnMouseUp (ev, but, mouseX, mouseY);
    }
  }
  else
  {
    if (ev.Name == FocusLost) editMode->OnFocusLost ();
  }

  return false;
}

bool AppAresEditWX::SimpleEventHandler (iEvent& ev)
{
  return aresed ? aresed->HandleEvent (ev) : false;
}

bool AppAresEditWX::LoadResourceFile (const char* filename, wxString& searchPath)
{
  wxString resourceLocation;
  wxFileSystem wxfs;
  if (!wxfs.FindFileInPath (&resourceLocation, searchPath,
	wxString::FromUTF8 (filename))
      || !wxXmlResource::Get ()->Load (resourceLocation))
    return ReportError ("Could not load XRC resource file: %s!", filename);
  return true;
}

bool AppAresEditWX::Initialize ()
{
  if (!celInitializer::RequestPlugins (object_reg,
	CS_REQUEST_VFS,
	CS_REQUEST_PLUGIN ("crystalspace.graphics2d.wxgl", iGraphics2D),
	CS_REQUEST_OPENGL3D,
	CS_REQUEST_ENGINE,
	CS_REQUEST_FONTSERVER,
	CS_REQUEST_IMAGELOADER,
	CS_REQUEST_LEVELLOADER,
	CS_REQUEST_REPORTER,
	CS_REQUEST_REPORTERLISTENER,
	CS_REQUEST_PLUGIN ("cel.physicallayer", iCelPlLayer),
	CS_REQUEST_PLUGIN ("cel.tools.elcm", iELCM),
	CS_REQUEST_PLUGIN ("crystalspace.collisiondetection.opcode", iCollideSystem),
	CS_REQUEST_PLUGIN ("crystalspace.dynamics.bullet", iDynamics),
	CS_REQUEST_PLUGIN ("crystalspace.decal.manager", iDecalManager),
	CS_REQUEST_PLUGIN ("utility.nature", iNature),
	CS_REQUEST_PLUGIN ("utility.marker", iMarkerManager),
	CS_REQUEST_PLUGIN ("utility.curvemesh", iCurvedMeshCreator),
	CS_REQUEST_PLUGIN ("utility.rooms", iRoomMeshCreator),
	CS_REQUEST_PLUGIN ("utility.assetmanager", iAssetManager),
	CS_REQUEST_END))
    return ReportError ("Can't initialize plugins!");

  //csEventNameRegistry::Register (object_reg);
  if (!csInitializer::SetupEventHandler (object_reg, SimpleEventHandler))
    return ReportError ("Can't initialize event handler!");

  CS_INITIALIZE_EVENT_SHORTCUTS (object_reg);

  KeyboardDown = csevKeyboardDown (object_reg);
  MouseMove = csevMouseMove (object_reg, 0);
  MouseUp = csevMouseUp (object_reg, 0);
  MouseDown = csevMouseDown (object_reg, 0);

  // Check for commandline help.
  if (csCommandLineHelper::CheckHelp (object_reg))
  {
    csCommandLineHelper::Help (object_reg);
    return false;
  }

  // The virtual clock.
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  if (vc == 0) return ReportError ("Can't find the virtual clock!");

  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  if (g3d == 0) return ReportError ("Can't find the iGraphics3D plugin!");

  vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs) return ReportError ("Can't find the iVFS plugin!");

  engine = csQueryRegistry<iEngine> (object_reg);
  if (!engine) return ReportError ("Can't find the engine plugin!");

  engine->SetSaveableFlag (true);

  if (ID_NewProject == csInvalidStringID)
  {
    csRef<iCelPlLayer> pl = csQueryRegistry<iCelPlLayer> (object_reg);
    ID_NewProject = pl->FetchStringID ("NewProject");
    ID_About = pl->FetchStringID ("About");
    ID_ManageAssets = pl->FetchStringID ("ManageAssets");
    ID_ManageResources = pl->FetchStringID ("ManageResources");
    ID_Open = pl->FetchStringID ("Open");
    ID_Save = pl->FetchStringID ("Save");
    ID_SaveAs = pl->FetchStringID ("SaveAs");
    ID_Exit = pl->FetchStringID ("Exit");
    ID_UpdateObjects = pl->FetchStringID ("UpdateObjects");
    ID_FindObjectDialog = pl->FetchStringID ("FindObjectDialog");
    ID_ConvertPhysics = pl->FetchStringID ("ConvertPhysics");
    ID_ConvertOpcode = pl->FetchStringID ("ConvertOpcode");
    ID_Join = pl->FetchStringID ("Join");
    ID_Unjoin = pl->FetchStringID ("Unjoin");
    ID_ManageCells = pl->FetchStringID ("ManageCells");
    ID_SwitchMode = pl->FetchStringID ("SwitchMode");
    ID_EntityParameters = pl->FetchStringID ("EntityParameters");
    ID_ViewMessages = pl->FetchStringID ("ViewMessages");
    ID_ViewComments = pl->FetchStringID ("ViewComments");
    ID_View3D = pl->FetchStringID ("View3D");
    ID_ViewControls = pl->FetchStringID ("ViewControls");
  }

  if (!config->ReadConfig ())
    return false;

  if (!InitWX ())
    return false;

  csRef<iStandardReporterListener> stdrep = csQueryRegistry<iStandardReporterListener> (object_reg);
  stdrep->RemoveMessages (CS_REPORTER_SEVERITY_BUG, false);
  stdrep->RemoveMessages (CS_REPORTER_SEVERITY_ERROR, false);
  stdrep->RemoveMessages (CS_REPORTER_SEVERITY_WARNING, false);
  stdrep->RemoveMessages (CS_REPORTER_SEVERITY_NOTIFY, false);
  stdrep->RemoveMessages (CS_REPORTER_SEVERITY_DEBUG, false);
  csRef<iReporter> reporter = csQueryRegistry<iReporter> (object_reg);
  csRef<AresReporterListener> repListener;
  repListener.AttachNew (new AresReporterListener (this));
  reporter->AddReporterListener (repListener);

  printer.AttachNew (new FramePrinter (object_reg));

  if (!ParseCommandLine ())
    return false;

  // Set the window title.
  UpdateTitle ();

  csRef<iDataBuffer> path = vfs->GetRealPath ("/icons/aresed_logo.gif");
  wxIcon icon (wxString::FromUTF8 (path->GetData ()), wxBITMAP_TYPE_GIF);
  SetIcon (icon);

  ready = true;

  uiManager->About ();

  return true;
}

bool AppAresEditWX::ParseCommandLine ()
{
  csRef<iCommandLineParser> cmdline (
  	csQueryRegistry<iCommandLineParser> (object_reg));
  const char* val = cmdline->GetName ();
  if (val)
  {
    if (!LoadFile (val))
      return false;
  }
  return true;
}

bool AppAresEditWX::InitPlugins ()
{
  const csArray<PluginConfig>& configplug = config->GetPlugins ();
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  for (size_t i = 0 ; i < configplug.GetSize () ; i++)
  {
    const PluginConfig& pc = configplug.Get (i);
    csString plugin = pc.plugin;
    printf ("Loading editor plugin %s\n", plugin.GetData ());
    fflush (stdout);

    csRef<iDataBuffer> buf = vfs->GetRealPath ("/ares/data/windows");
    csString path (buf->GetData ());
    wxString searchPath (wxString::FromUTF8 (path));
    for (size_t j = 0 ; j < pc.resources.GetSize () ; j++)
    {
      if (!LoadResourceFile (pc.resources.Get (j), searchPath)) return false;
    }

    csRef<iEditorPlugin> plug = csLoadPluginCheck<iEditorPlugin> (object_reg, plugin);
    if (!plug)
    {
      return ReportError ("Could not load plugin '%s'!", plugin.GetData ());
    }
    csString pluginName = plug->GetPluginName ();

    plug->SetApplication (static_cast<iAresEditor*> (this));

    if (pc.addToNotebook)
    {
      wxPanel* panel = new wxPanel (notebook, wxID_ANY, wxDefaultPosition,
	wxDefaultSize, wxTAB_TRAVERSAL);
      notebook->AddPage (panel, wxString::FromUTF8 (pluginName), false);
      panel->SetToolTip (wxString::FromUTF8 (pc.tooltip));
      wxBoxSizer* sizer = new wxBoxSizer (wxVERTICAL);
      panel->SetSizer (sizer);
      panel->Layout ();
      plug->SetParent (panel);
    }
    else
    {
      plug->SetParent (this);
    }

    csRef<iEditingMode> emode = scfQueryInterface<iEditingMode> (plug);
    if (emode)
    {
      emode->AllocContextHandlers (this);
      if (pc.mainMode)
      {
	mainMode = emode;
	mainModeName = pluginName;
      }
      else if (pluginName == "Play")
	playMode = emode;
    }
    if (pluginName == "DynfactEditor")
      dynfactDialog = plug;

    plugins.Push (plug);
  }

  return true;
}

void AppAresEditWX::RegisterModification (const csArray<iObject*>& resources)
{
  if (resources.GetSize () == 0) return;
  iAsset* asset = 0;
  bool noplace = false;
  for (size_t i = 0 ; i < resources.GetSize () ; i++)
  {
    iObject* resource = resources[i];
    if (!assetManager->RegisterModification (resource))
    {
      if (!assetManager->IsResourceWithoutAsset (resource))
      {
	if (asset || noplace)
	{
	  assetManager->PlaceResource (resource, asset);
	  assetManager->RegisterModification (resource);
	}
	else
	{
	  csString title = "Select an asset for these resources";
	  csRef<Ares::Value> assets = aresed3d->GetModelRepository ()->GetWritableAssetsValue ();
          Value* assetVal = uiManager->AskDialog (title, assets, "Writable,Path,File,Mount",
	      ASSET_COL_WRITABLE, ASSET_COL_PATH, ASSET_COL_FILE, ASSET_COL_MOUNT);
          if (assetVal)
          {
	    AssetsValue* av = static_cast<AssetsValue*> ((Ares::Value*)assets);
	    asset = av->GetObjectFromValue (assetVal);
	    assetManager->PlaceResource (resource, asset);
	    assetManager->RegisterModification (resource);
          }
	  if (!asset)
	  {
	    uiManager->Message ("Warning! These assets will not be saved!");
	    assetManager->PlaceResource (resource, 0);
	    noplace = true;
	  }
	}
      }
    }
  }
  UpdateTitle ();
}

void AppAresEditWX::RegisterModification (iObject* resource)
{
  if (!resource)
  {
    assetManager->RegisterModification ();
  }
  else if (!assetManager->RegisterModification (resource))
  {
    if (!assetManager->IsResourceWithoutAsset (resource))
    {
      csString title;
      title.Format ("Select an asset for this resource '%s'", resource->GetName ());
      csRef<Ares::Value> assets = aresed3d->GetModelRepository ()->GetWritableAssetsValue ();
      Value* assetVal = uiManager->AskDialog (title, assets, "Writable,Path,File,Mount",
	      ASSET_COL_WRITABLE, ASSET_COL_PATH, ASSET_COL_FILE, ASSET_COL_MOUNT);
      if (assetVal)
      {
	AssetsValue* av = static_cast<AssetsValue*> ((Ares::Value*)assets);
	iAsset* asset = av->GetObjectFromValue (assetVal);
	assetManager->PlaceResource (resource, asset);
	assetManager->RegisterModification (resource);
	UpdateTitle ();
	return;
      }

      uiManager->Message ("Warning! This asset will not be saved!");
      assetManager->PlaceResource (resource, 0);
    }
  }
  UpdateTitle ();
}

bool AppAresEditWX::InitWX ()
{
  // Load the frame from an XRC file
  wxXmlResource::Get ()->InitAllHandlers ();

  csRef<iDataBuffer> buf = vfs->GetRealPath ("/ares/data/windows");
  csString path (buf->GetData ());
  wxString searchPath (wxString::FromUTF8 (path));
  if (!LoadResourceFile ("AresMainFrame.xrc", searchPath)) return false;
  if (!LoadResourceFile ("FileRequester.xrc", searchPath)) return false;
  if (!LoadResourceFile ("CameraPanel.xrc", searchPath)) return false;
  if (!LoadResourceFile ("NewProjectDialog.xrc", searchPath)) return false;
  if (!LoadResourceFile ("CellDialog.xrc", searchPath)) return false;
  if (!LoadResourceFile ("EntityParameterDialog.xrc", searchPath)) return false;
  if (!LoadResourceFile ("ObjectFinderDialog.xrc", searchPath)) return false;
  if (!LoadResourceFile ("ResourceMoverDialog.xrc", searchPath)) return false;

  wxPanel* mainPanel = wxXmlResource::Get ()->LoadPanel (this, wxT ("AresMainPanel"));
  if (!mainPanel) return ReportError ("Can't find main panel!");

  // Find the panel where to place the wxgl canvas
  wxPanel* panel = XRCCTRL (*this, "main3DPanel", wxPanel);
  if (!panel) return ReportError ("Can't find main3DPanel!");
  panel->DragAcceptFiles (false);

  // Create the wxgl canvas
  iGraphics2D* g2d = g3d->GetDriver2D ();
  g2d->AllowResize (true);
  wxwindow = scfQueryInterface<iWxWindow> (g2d);
  if (!wxwindow) return ReportError ("Canvas is no iWxWindow plugin!");

  wxPanel* panel1 = new AppAresEditWX::Panel (panel, this);
  panel->GetSizer ()->Add (panel1, 1, wxALL | wxEXPAND);
  wxwindow->SetParent (panel1);

  Show (true);

  // Open the main system. This will open all the previously loaded plug-ins.
  if (!csInitializer::OpenApplication (object_reg))
    return ReportError ("Error opening system!");

  /* Manually focus the GL canvas.
     This is so it receives keyboard events (and conveniently translate these
     into CS keyboard events/update the CS keyboard state).
   */
  wxwindow->GetWindow ()->SetFocus ();

  aresed3d = new AresEdit3DView (this, object_reg);
  if (!aresed3d->Setup ())
    return false;

  assetManager = csQueryRegistry<iAssetManager> (object_reg);
  assetManager->SetZone (aresed3d->GetDynamicWorld ());

  uiManager.AttachNew (new UIManager (this, wxwindow->GetWindow ()));

  if (!InitPlugins ()) return false;
  editMode = 0;

  wxPanel* leftPanePanel = XRCCTRL (*this, "leftPanePanel", wxPanel);
  leftPanePanel->SetMinSize (wxSize (270,-1));
  camwin = new CameraWindow (leftPanePanel, aresed3d);
  camwin->AllocContextHandlers (this);

  SelectionListener* listener = new AppSelectionListener (this);
  aresed3d->GetSelectionInt ()->AddSelectionListener (listener);

  RefreshModes ();

  if (!SetupMenuBar ())
    return false;

  SwitchToMainMode ();

  wxSplitterWindow* topBottomSplitter = XRCCTRL (*this, "topBottom_Splitter", wxSplitterWindow);
  topBottomSplitter->Unsplit ();
  ViewControls ();

  return true;
}

void AppAresEditWX::SetFocus3D ()
{
  wantsFocus3D = 10;
}

void AppAresEditWX::SetStatus (const char* statusmsg, ...)
{
  va_list args;
  va_start (args, statusmsg);
  csString str;
  str.FormatV (statusmsg, args);
  va_end (args);
  if (GetStatusBar ())
    GetStatusBar ()->SetStatusText (wxString::FromUTF8 (str), 0);
}

void AppAresEditWX::ClearStatus ()
{
  if (editMode)
  {
    SetStatus ("%s", editMode->GetStatusLine ()->GetData ());
  }
  else
    SetStatus ("");
}

void AppAresEditWX::SetObjectForComment (const char* type, iObject* objForComment)
{
  objectForComment = objForComment;
  if (objectForComment)
  {
    csRef<iObjectComment> comment = CS::GetChildObject<iObjectComment> (objectForComment);
    csString commentString;
    if (comment)
      commentString = comment->GetComment ()->GetData ();
    wxTextCtrl* comments_Text = XRCCTRL (*this, "comments_Text", wxTextCtrl);
    comments_Text->ChangeValue (wxString::FromUTF8 (commentString));
    wxStaticText* comments_Label = XRCCTRL (*this, "comments_Label", wxStaticText);
    csString label;
    label.Format ("Comment for %s %s", type, objectForComment->GetName ());
    comments_Label->SetLabel (wxString::FromUTF8 (label));
    bool modifiable = assetManager->IsModifiable (objForComment);
    comments_Text->SetEditable (modifiable);
  }
}

void AppAresEditWX::OnChangeComment (wxCommandEvent& event)
{
  if (objectForComment)
  {
    wxTextCtrl* comments_Text = XRCCTRL (*this, "comments_Text", wxTextCtrl);
    csString text = (const char*)comments_Text->GetValue ().mb_str (wxConvUTF8);
    csRef<iObjectComment> comment = CS::GetChildObject<iObjectComment> (objectForComment);
    if (!comment)
    {
      comment.AttachNew (new CS::Persistence::ObjectComment ());
      objectForComment->ObjAdd (comment->QueryObject ());
    }
    comment->GetComment ()->Replace (text);
    RegisterModification (objectForComment);
  }
}

void AppAresEditWX::ReceiveMessage (int severity, const char* msgID, const char* description)
{
  bool popup = false;
  if (severity != lastMessageSeverity || lastMessageID != msgID)
  {
    popup = true;
    csString color;
    switch (severity)
    {
      case CS_REPORTER_SEVERITY_DEBUG: color = "green"; break;
      case CS_REPORTER_SEVERITY_BUG: color = "green"; break;
      case CS_REPORTER_SEVERITY_ERROR: color = "red"; break;
      case CS_REPORTER_SEVERITY_WARNING: color = "orange"; break;
      case CS_REPORTER_SEVERITY_NOTIFY: color = "black"; popup = false; break;
      default: color = "green";
    }
    lastMessageSeverity = severity;
    lastMessageID = msgID;
    messages.AppendFmt ("<font color='%s'><b>%s</b></font><br>", color.GetData (), msgID);
  }
  messages.AppendFmt ("&nbsp;&nbsp;%s<br>", description);
  if (popup)
  {
    ViewBottomPage ("Messages", false);
  }

  wxHtmlWindow* message_Html = XRCCTRL (*this, "message_Html", wxHtmlWindow);
  message_Html->SetPage (wxString::FromUTF8 ("<html><body>"+messages+"</body></html>"));
  message_Html->Scroll (0, 1000000000);
}

void AppAresEditWX::OnClearMessages (wxCommandEvent& event)
{
  messages = "";
  lastMessageID = csInvalidStringID;
  lastMessageSeverity = -1;
  wxHtmlWindow* message_Html = XRCCTRL (*this, "message_Html", wxHtmlWindow);
  message_Html->SetPage (wxString::FromUTF8 ("<html><body>"+messages+"</body></html>"));
}

void AppAresEditWX::ViewBottomPage (const char* name, bool toggle)
{
  wxSplitterWindow* topBottomSplitter = XRCCTRL (*this, "topBottom_Splitter", wxSplitterWindow);
  wxPanel* topPanel = XRCCTRL (*this, "top_Panel", wxPanel);
  wxPanel* bottomPanel = XRCCTRL (*this, "bottom_Panel", wxPanel);
  wxNotebook* notebook = XRCCTRL (*this, "bottom_Notebook", wxNotebook);

  size_t idx = FindNotebookPage (notebook, name);

  if (bottomPanel->IsShown ())
  {
    wxWindow* page = notebook->GetCurrentPage ();
    if (page == notebook->GetPage (idx))
    {
      if (toggle)
        topBottomSplitter->Unsplit ();
    }
    else
    {
      notebook->ChangeSelection (idx);
    }
  }
  else
  {
    wxSize totsize = GetSize ();
    topBottomSplitter->Unsplit ();
    topBottomSplitter->SplitHorizontally (topPanel, bottomPanel, totsize.y - 250);
    notebook->ChangeSelection (idx);
  }
}

void AppAresEditWX::View3D ()
{
  wxSplitterWindow* leftRightSplitter = XRCCTRL (*this, "leftRight_Splitter", wxSplitterWindow);
  wxPanel* leftPanel = XRCCTRL (*this, "leftPanePanel", wxPanel);
  wxPanel* rightPanel = XRCCTRL (*this, "main3DPanel", wxPanel);
  wxSize size = leftPanel->GetMinSize ();
  leftRightSplitter->Unsplit ();
  leftRightSplitter->SplitVertically (leftPanel, rightPanel, size.x);
}

void AppAresEditWX::ViewControls ()
{
  wxSplitterWindow* leftRightSplitter = XRCCTRL (*this, "leftRight_Splitter", wxSplitterWindow);
  wxPanel* leftPanel = XRCCTRL (*this, "leftPanePanel", wxPanel);
  wxPanel* rightPanel = XRCCTRL (*this, "main3DPanel", wxPanel);
  wxSize size = leftPanel->GetMinSize ();
  leftRightSplitter->Unsplit ();
  leftRightSplitter->SplitVertically (leftPanel, rightPanel, size.x);
}

bool AppAresEditWX::Command (csStringID id, const csString& args)
{
  if (id == ID_NewProject) NewProject ();
  else if (id == ID_About) uiManager->About ();
  else if (id == ID_ManageAssets) ManageAssets ();
  else if (id == ID_ManageResources) ManageResources ();
  else if (id == ID_Open) OpenFile ();
  else if (id == ID_Save) SaveCurrentFile ();
  else if (id == ID_SaveAs) SaveFile ();
  else if (id == ID_Exit) Quit ();
  else if (id == ID_UpdateObjects) aresed3d->UpdateObjects ();
  else if (id == ID_FindObjectDialog) FindObject ();
  else if (id == ID_ConvertPhysics) aresed3d->ConvertPhysics ();
  else if (id == ID_ConvertOpcode) aresed3d->ConvertOpcode ();
  else if (id == ID_Join) aresed3d->JoinObjects ();
  else if (id == ID_Unjoin) aresed3d->UnjoinObjects ();
  else if (id == ID_ManageCells) uiManager->GetCellDialog ()->Show ();
  else if (id == ID_SwitchMode) SwitchToMode (args);
  else if (id == ID_EntityParameters) aresed3d->EntityParameters ();
  else if (id == ID_ViewMessages) ViewBottomPage ("Messages", true);
  else if (id == ID_ViewComments) ViewBottomPage ("Comments", true);
  else if (id == ID_View3D) View3D ();
  else if (id == ID_ViewControls) ViewControls ();
  else return false;
  return true;
}

bool AppAresEditWX::IsCommandValid (csStringID id, const csString& args,
      iSelection* selection, size_t pastesize)
{
  size_t selsize = selection->GetSize ();
  if (id == ID_Join) return selsize == 2 && editMode == mainMode;
  if (id == ID_Unjoin) return selsize > 0 && editMode == mainMode;
  if (id == ID_EntityParameters) return selsize == 1 && editMode == mainMode;

  return true;
}

void AppAresEditWX::OnMenuUpdate (wxUpdateUIEvent& event)
{
  int id = event.GetId ();
  MenuCommand mc = menuCommands.Get (id, MenuCommand ());
  if (mc.commandID == csInvalidStringID) return;
  csRef<iCommandHandler> handler = mc.target;
  if (editMode && !handler) handler = scfQueryInterface<iCommandHandler> (editMode);
  if (handler)
  {
    bool enabled = handler ? handler->IsCommandValid (mc.commandID, mc.args,
	aresed3d->GetSelection (), aresed3d->GetPaster ()->GetClipboardSize ()) : false;
    event.Enable (enabled);
    csRef<iString> label = handler->GetAlternativeLabel (mc.commandID, aresed3d->GetSelection (),
	aresed3d->GetPaster ()->GetClipboardSize ());
    if (label)
      event.SetText (wxString::FromUTF8 (label->GetData ()));
  }
}

void AppAresEditWX::OnMenuItem (wxCommandEvent& event)
{
  int id = event.GetId ();
  MenuCommand mc = menuCommands.Get (id, MenuCommand ());
  if (mc.commandID == csInvalidStringID)
  {
    printf ("Unhandled menu item %d!\n", id);
    fflush (stdout);
    return;
  }
  csRef<iCommandHandler> handler = mc.target;
  if (editMode && !handler) handler = scfQueryInterface<iCommandHandler> (editMode);
  if (handler)
    handler->Command (mc.commandID, mc.args);
}

void AppAresEditWX::AppendMenuItem (wxMenu* menu, int id, const char* label,
    const char* targetName, const char* command, const char* args, const char* help)
{
  csRef<iCommandHandler> target;
  if (targetName && *targetName == '*' && !*(targetName+1))
  {
    // Current editmode.
  }
  else if (targetName && *targetName)
  {
    iEditorPlugin* plug = FindPlugin (targetName);
    target = scfQueryInterface<iCommandHandler> (plug);
    if (!target)
      ReportError ("Target '%s' has no command handler!", targetName);
  }
  else if (!target) target = static_cast<iCommandHandler*> (this);

  menu->Append (id, wxString::FromUTF8 (label));
  MenuCommand mc;
  mc.target = target;
  mc.command = command;
  csRef<iCelPlLayer> pl = csQueryRegistry<iCelPlLayer> (object_reg);
  mc.commandID = pl->FetchStringID (command);
  mc.args = args;
  mc.help = help;

  menu->SetHelpString (id, wxString::FromUTF8 (mc.help));

  menuCommands.Put (id, mc);
}

bool AppAresEditWX::SetupMenuBar ()
{
  wxMenuBar* menuBar = config->BuildMenuBar ();
  if (!menuBar) return false;
  SetMenuBar (menuBar);
  menuBar->Reparent (this);

  CreateStatusBar ();
  SetMenuState ();
  return true;
}

void AppAresEditWX::ShowCameraWindow ()
{
  camwin->Show ();
}

void AppAresEditWX::HideCameraWindow ()
{
  camwin->Hide ();
}

void AppAresEditWX::SetMenuState ()
{
  ClearStatus ();
  wxMenuBar* menuBar = GetMenuBar ();
  if (!menuBar) return;

  // Should menus be globally disabled?
  bool dis = aresed3d ? aresed3d->GetPaster ()->IsPasteSelectionActive () : true;
  if (IsPlaying ()) dis = true;
  if (dis)
  {
    menuBar->EnableTop (0, false);
    return;
  }
  menuBar->EnableTop (0, true);
}

void AppAresEditWX::PushFrame ()
{
  if (!ready) return;
  static bool lock = false;
  if (lock) return;
  lock = true;

  if (wantsFocus3D)
  {
    wantsFocus3D--;
    if (wantsFocus3D <= 0)
    {
      printf ("Setting focus for real!\n");
      wxwindow->GetWindow ()->SetFocus ();
    }
  }

  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  csRef<iVirtualClock> vc (csQueryRegistry<iVirtualClock> (object_reg));

  if (vc)
    vc->Advance();
  q->Process();
  lock = false;
}

void AppAresEditWX::OnSize (wxSizeEvent& event)
{
  if (!wxwindow->GetWindow ()) return;

  wxSize size = event.GetSize();
  printf ("OnSize %d,%d\n", size.x, size.y); fflush (stdout);

  wxwindow->GetWindow ()->SetSize (size);
  aresed3d->ResizeView (size.x, size.y);
  // TODO: ... but here the CanvasResize event has still not been catched by iGraphics3D
}

void AppAresEditWX::OnIdle (wxIdleEvent& event)
{
  PushFrame();
}

void AppAresEditWX::OnClose (wxCloseEvent& event)
{
  csPrintf("got close event\n");
  
  // Tell CS we're going down
  csRef<iEventQueue> q (csQueryRegistry<iEventQueue> (object_reg));
  if (q) q->GetEventOutlet()->Broadcast (csevQuit(object_reg));
  
  // WX will destroy the 'AppAresEditWX' instance
  aresed = 0;
}

void AppAresEditWX::OnIconize (wxIconizeEvent& event)
{
  csPrintf("got iconize %d\n", (int) event.Iconized ());
}

void AppAresEditWX::OnShow (wxShowEvent& event)
{
  csPrintf("got show %d\n", (int) event.GetShow ());
}

iEditorConfig* AppAresEditWX::GetConfig () const
{
  return static_cast<iEditorConfig*> (config);
}

void AppAresEditWX::SwitchToMainMode ()
{
  SwitchToMode (mainModeName);
}

void AppAresEditWX::SetCurveModeEnabled (bool cm)
{
  wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
  size_t pageIdx = FindNotebookPage (notebook, "Curve");
  if (pageIdx != csArrayItemNotFound)
  {
    wxNotebookPage* page = notebook->GetPage (pageIdx);
    if (cm) page->Enable ();
    else page->Disable ();
  }
}

iEditorPlugin* AppAresEditWX::FindPlugin (const char* name)
{
  csString n = name;
  for (size_t i = 0 ; i < plugins.GetSize () ; i++)
  {
    if (n == plugins.Get (i)->GetPluginName ())
      return plugins.Get (i);
  }
  return 0;
}

void AppAresEditWX::SwitchToMode (const char* name)
{
  printf ("SwitchToMode %s\n", name); fflush (stdout);
  iEditorPlugin* plugin = FindPlugin (name);
  csRef<iEditingMode> mode;
  if (plugin) mode = scfQueryInterface<iEditingMode> (plugin);
  if (mode)
  {
    wxNotebook* notebook = XRCCTRL (*this, "mainNotebook", wxNotebook);
    size_t pageIdx = FindNotebookPage (notebook, name);
    if (pageIdx != csArrayItemNotFound)
      notebook->ChangeSelection (pageIdx);
    if (editMode) editMode->Stop ();
    editMode = mode;
    editMode->Start ();
    SetMenuState ();
  }
  else
  {
    CS_ASSERT (false);
    printf ("Can't find mode '%s'!\n", name);
    fflush (stdout);
  }
}


