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

#include "appares.h"
#include <celtool/initapp.h>

#include "celtool/persisthelper.h"
#include "tools/billboard.h"
#include "tools/elcm.h"
#include "physicallayer/pl.h"
#include "physicallayer/propfact.h"
#include "physicallayer/propclas.h"
#include "physicallayer/entity.h"
#include "behaviourlayer/bl.h"
#include "propclass/test.h"
#include "propclass/delegcam.h"
#include "propclass/cameras/tracking.h"
#include "propclass/mesh.h"
#include "propclass/meshsel.h"
#include "propclass/inv.h"
#include "propclass/jump.h"
#include "propclass/chars.h"
#include "propclass/move.h"
#include "propclass/tooltip.h"
#include "propclass/newcamera.h"
#include "propclass/gravity.h"
#include "propclass/timer.h"
#include "propclass/region.h"
#include "propclass/input.h"
#include "propclass/linmove.h"
#include "propclass/analogmotion.h"
#include "propclass/quest.h"
#include "propclass/trigger.h"
#include "propclass/zone.h"
#include "propclass/sound.h"
#include "propclass/wire.h"
#include "propclass/billboard.h"
#include "propclass/prop.h"
#include "propclass/mechsys.h"

#include "iassetmanager.h"

#define PATHFIND_VERBOSE 0

//-----------------------------------------------------------------------------

class AresDynamicCellCreator : public scfImplementation1<AresDynamicCellCreator,
  iDynamicCellCreator>
{
private:
  AppAres* ares;

public:
  AresDynamicCellCreator (AppAres* ares) :
    scfImplementationType (this), ares (ares) { }
  virtual ~AresDynamicCellCreator () { }
  virtual iDynamicCell* CreateCell (const char* name)
  {
    return ares->CreateCell (name);
  }
  virtual void FillCell (iDynamicCell* cell) { }
};

//-----------------------------------------------------------------------------

void MenuEntry::Move (int x, int y, int screenw)
{
  x1 = x;
  y1 = y;
  int xx = x + w/2;
  int a = int (255.0 * float (ABS (screenw/2 - xx)) / float (screenw/2));
  if (a < 0) a = 0;
  else if (a > 255) a = 255;
  alpha = a;
}

void MenuEntry::StartInterpolate (int destdx, int destdy, int destw, int desth, float seconds)
{
  currenttime = 0;
  totaltime = seconds;
  src_dx = dx;
  src_dy = dy;
  src_w = w;
  src_h = h;
  dest_dx = destdx;
  dest_dy = destdy;
  dest_w = destw;
  dest_h = desth;
}

void MenuEntry::Step (float seconds)
{
  if (fabs (totaltime) >= .001)
  {
    currenttime += seconds;
    if (currenttime >= totaltime)
    {
      dx = dest_dx;
      dy = dest_dy;
      w = dest_w;
      h = dest_h;
    }
    else
    {
      float f1 = (totaltime-currenttime)/totaltime;
      float f2 = currenttime/totaltime;
      dx = src_dx*f1 + dest_dx*f2;
      dy = src_dy*f1 + dest_dy*f2;
      w = src_w*f1 + dest_w*f2;
      h = src_h*f1 + dest_h*f2;
    }
  }
}

bool AresMenu::OnMouseDown (iEvent &ev)
{
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);
  for (size_t i = 0 ; i < menuGames.GetSize () ; i++)
  {
    const MenuEntry& me = menuGames[i];
    int x = me.x1 + me.dx;
    int y = me.y1 + me.dy;
    if (mouseX >= x && mouseX <= x+me.w && mouseY >= y && mouseY <= y+me.h)
    {
      gameFile = me.filename;
      menuMode = MENU_WAIT1;
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------

#define SCROLLMARGIN 40
#define WSEL 200
#define WUNSEL 140

AresMenu::AresMenu (AppAres* app)
{
  AresMenu::app = app;
  menuMode = MENU_LIST;
  mouseX = 0;
  mouseY = 0;
  menuBox = 0;
  menuLoading = 0;
}

AresMenu::~AresMenu ()
{
  CleanupMenu ();
}

static csString ExtractName (const csString& n)
{
  csString name = n;
  size_t idx = name.FindLast ('.');
  if (idx != csArrayItemNotFound) name = name.Slice (0, idx);
  idx = name.FindLast ('/');
  if (idx != csArrayItemNotFound) name = name.Slice (idx+1);
  return name;
}

bool AresMenu::SetupMenu ()
{
  menuFont = app->g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_LARGE);

  iTextureWrapper* txt = app->engine->CreateTexture ("bigicon_aresmenu",
      "/icons/bigicon_aresmenu.png", 0, CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS);
  txt->Register (app->g3d->GetTextureManager ());
  menuBox = new csSimplePixmap (txt->GetTextureHandle ());

  txt = app->engine->CreateTexture ("bigicon_loading",
      "/icons/bigicon_loading.png", 0, CS_TEXTURE_2D | CS_TEXTURE_NOMIPMAPS);
  txt->Register (app->g3d->GetTextureManager ());
  menuLoading = new csSimplePixmap (txt->GetTextureHandle ());

  menuGames.Empty ();
  csRef<iStringArray> vfsFiles = app->vfs->FindFiles ("/saves/");
  size_t idx = 0;
  for (size_t i = 0 ; i < vfsFiles->GetSize() ; i++)
  {
    csString file = (char*)vfsFiles->Get(i);
    if (file.Length () == 0) continue;
    if (!file.EndsWith (".ares")) continue;
    MenuEntry entry;
    entry.name = ExtractName (file);
    entry.filename = file;
    entry.x1 = 10;
    entry.y1 = 10;
    entry.w = WUNSEL;
    entry.h = WUNSEL;
    entry.alpha = 100;
    menuFont->GetDimensions (entry.name, entry.fontw, entry.fonth);
    menuGames.Push (entry);
    idx++;
  }

  menuMode = MENU_LIST;

  ActivateMenuEntry (0.0f);

  return true;
}

void AresMenu::MenuFrame ()
{
  iGraphics3D* g3d = app->g3d;
  if (menuMode == MENU_LIST)
  {
    float elapsed_time = app->vc->GetElapsedSeconds ();

    int w = g3d->GetDriver2D ()->GetWidth ();
    if (mouseX > w/2+SCROLLMARGIN)
      ActivateMenuEntry (menuActive + 10.0 * elapsed_time * float (mouseX - (w/2+SCROLLMARGIN)) / float (w));
    else if (mouseX < w/2-SCROLLMARGIN)
      ActivateMenuEntry (menuActive - 10.0 * elapsed_time * float ((w/2-SCROLLMARGIN) - mouseX) / float (w));

    g3d->BeginDraw (CSDRAW_2DGRAPHICS | CSDRAW_CLEARSCREEN | CSDRAW_CLEARZBUFFER);
    iGraphics2D* g2d = g3d->GetDriver2D ();
    int fg = g2d->FindRGB (255, 255, 255);
    int bg = -1;
    for (size_t i = 0 ; i < menuGames.GetSize () ; i++)
    {
      MenuEntry& me = menuGames[i];
      int x = me.x1 + me.dx;
      int y = me.y1 + me.dy;
      me.Step (elapsed_time);
      menuBox->DrawScaled (g3d, x, y, me.w, me.h, me.alpha);
      if (mouseX >= x && mouseX <= x+me.w && mouseY >= y && mouseY <= y+me.h)
      {
	if (!me.active)
	{
          me.StartInterpolate (-(WSEL-WUNSEL)/2, -(WSEL-WUNSEL)/2, WSEL, WSEL, 0.1);
	  me.active = true;
	}
      }
      else
      {
	if (me.active)
	{
          me.StartInterpolate (0, 0, WUNSEL, WUNSEL, 0.1);
	  me.active = false;
	}
      }
      g2d->Write (menuFont,
	    x + (me.w>>1) - (me.fontw>>1), y + (me.h>>1) - (me.fonth>>1),
	    fg, bg, me.name);
    }
  }
  else if (menuMode == MENU_WAIT1)
  {
    g3d->BeginDraw (CSDRAW_2DGRAPHICS | CSDRAW_CLEARSCREEN | CSDRAW_CLEARZBUFFER);
    int w = g3d->GetDriver2D ()->GetWidth ();
    int h = g3d->GetDriver2D ()->GetHeight ();
    menuLoading->Draw (g3d, (w-menuLoading->Width ())/2, (h-menuLoading->Height ())/2);
    menuMode = MENU_WAIT2;
  }
  else if (menuMode == MENU_WAIT2)
  {
    if (gameFile)
    {
      menuMode = MENU_GAME;
      app->StartGame (gameFile);
    }
  }
}

void AresMenu::OnMouseMove (iEvent& ev)
{
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);
}

void AresMenu::ActivateMenuEntry (float entry)
{
  if (entry <= 0.0f) entry = 0.0f;
  else if (entry >= float (menuGames.GetSize ())) entry = float (menuGames.GetSize ());
  int w = app->g3d->GetDriver2D ()->GetWidth ();
  int h = app->g3d->GetDriver2D ()->GetHeight ();
  menuActive = entry;
  for (int i = 0 ; i < int (menuGames.GetSize ()) ; i++)
  {
    MenuEntry& me = menuGames[i];
    int x = (float (i)-menuActive)*190.0f + w/2;
    int y = (h-WUNSEL)/2;
    me.Move (x, y, w);
  }
}

void AresMenu::CleanupMenu ()
{
  delete menuBox;
  menuBox = 0;
  delete menuLoading;
  menuLoading = 0;
}

//-----------------------------------------------------------------------------

AppAres::AppAres () : menu (this)
{
  SetApplicationName ("Ares");
  currentTime = 31000;
}

AppAres::~AppAres ()
{
}

void AppAres::OnExit ()
{
  if (pl) pl->CleanCache ();
  printer.Invalidate ();
}

void AppAres::Frame ()
{
  if (menu.GetMode () != MENU_GAME)
  {
    menu.MenuFrame ();
  }
  else
  {
    float elapsed_time = vc->GetElapsedSeconds ();
    nature->UpdateTime (currentTime, camera);
    currentTime += csTicks (elapsed_time * 1000);

    //if (do_simulation)
    {
      float dynamicSpeed = 1.0f;
      dyn->Step (elapsed_time / dynamicSpeed);
    }

    dynworld->PrepareView (camera, elapsed_time);
  }
}


bool AppAres::OnMouseMove (iEvent& ev)
{
  if (menu.GetMode () == MENU_LIST)
    menu.OnMouseMove (ev);
  return false;
}

bool AppAres::OnMouseDown (iEvent &ev)
{
  if (menu.GetMode () == MENU_LIST)
    return menu.OnMouseDown (ev);
  return false;
}

bool AppAres::OnKeyboard (iEvent &ev)
{
  // We got a keyboard event.
  if (csKeyEventHelper::GetEventType (&ev) == csKeyEventTypeDown)
  {
    // The user pressed a key (as opposed to releasing it).
    utf32_char code = csKeyEventHelper::GetCookedCode (&ev);
    if (code == CSKEY_ESC)
    {
      // The user pressed escape. For now we will simply exit the
      // application. The proper way to quit a Crystal Space application
      // is by broadcasting a cscmdQuit event. That will cause the
      // main runloop to stop. To do that we get the event queue from
      // the object registry and then post the event.
      csRef<iEventQueue> q = csQueryRegistry<iEventQueue> (object_reg);
      q->GetEventOutlet ()->Broadcast (csevQuit (object_reg));
    }
  }
  return false;
}

iDynamicCell* AppAres::CreateCell (const char* name)
{
  iSector* s = engine->FindSector (name);
  if (!s)
    s = engine->CreateSector (name);

  csRef<iDynamicSystem> ds;
  if (dynworld->IsPhysicsEnabled ())
  {
    dyn = csQueryRegistry<iDynamics> (GetObjectRegistry ());
    if (!dyn) { ReportError ("Error loading bullet plugin!"); return 0; }

    csString systemname = "ares.dynamics.system.";
    systemname += name;
    ds = dyn->FindSystem (systemname);
    if (!ds)
    {
      ds = dyn->CreateSystem ();
      ds->QueryObject ()->SetName (systemname);
    }
    if (!ds) { ReportError ("Error creating dynamic system!"); return 0; }

    //ds->SetLinearDampener(.3f);
    ds->SetRollingDampener(.995f);
    ds->SetGravity (csVector3 (0.0f, -19.81f, 0.0f));

    csRef<CS::Physics::Bullet::iDynamicSystem> bullet_dynSys = scfQueryInterface<
      CS::Physics::Bullet::iDynamicSystem> (ds);
    //@@@ (had to disable because bodies might alredy exist!) bullet_ds->SetInternalScale (1.0f);
    bullet_dynSys->SetStepParameters (0.005f, 2, 10);

    // Find the terrain mesh. @@@ HARDCODED!
    csRef<iMeshWrapper> terrainWrapper = engine->FindMeshObject ("Terrain");
    if (terrainWrapper)
    {
      csRef<iTerrainSystem> terrain =
        scfQueryInterface<iTerrainSystem> (terrainWrapper->GetMeshObject ());
      if (!terrain)
      {
        ReportError("Error cannot find the terrain interface!");
        return 0;
      }

      // Create a terrain collider for each cell of the terrain
      for (size_t i = 0; i < terrain->GetCellCount (); i++)
        bullet_dynSys->AttachColliderTerrain (terrain->GetCell (i));
    }
  }

  iDynamicCell* cell = dynworld->AddCell (name, s, ds);
  return cell;
}

bool AppAres::LoadLibrary (const char* path, const char* file)
{
  // Set current VFS dir to the level dir, helps with relative paths in maps
  vfs->PushDir (path);
  if (!loader->LoadLibraryFile (file))
  {
    vfs->PopDir ();
    return ReportError("Couldn't load library file %s!", path);
  }
  vfs->PopDir ();
  return true;
}

bool AppAres::OnInitialize (int argc, char* argv[])
{
  if (!celInitializer::SetupConfigManager (object_reg,
  	"/appdata/AppAres.cfg"))
  {
    return ReportError ("Can't setup config file!");
  }

  if (!celInitializer::RequestPlugins (object_reg,
  	CS_REQUEST_VFS,
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
	CS_REQUEST_PLUGIN ("crystalspace.sndsys.element.loader", iSndSysLoader),
	CS_REQUEST_PLUGIN("crystalspace.dynamics.bullet", iDynamics),
	CS_REQUEST_PLUGIN ("crystalspace.decal.manager", iDecalManager),
	CS_REQUEST_PLUGIN("utility.nature", iNature),
	CS_REQUEST_PLUGIN("utility.curvemesh", iCurvedMeshCreator),
	CS_REQUEST_PLUGIN("utility.rooms", iRoomMeshCreator),
	CS_REQUEST_PLUGIN("utility.assetmanager", iAssetManager),
	CS_REQUEST_END))
  {
    return ReportError ("Can't initialize plugins!");
  }

  csBaseEventHandler::Initialize (object_reg);

  csRef<iStandardReporterListener> stdrep = csQueryRegistry<iStandardReporterListener> (object_reg);
  stdrep->SetPopupOutput (CS_REPORTER_SEVERITY_ERROR, false);
  stdrep->SetPopupOutput (CS_REPORTER_SEVERITY_WARNING, false);
  stdrep->SetPopupOutput (CS_REPORTER_SEVERITY_NOTIFY, false);

  // Now we need to setup an event handler for our application.
  // Crystal Space is fully event-driven. Everything (except for this
  // initialization) happens in an event.
  if (!RegisterQueue (object_reg, csevAllEvents (object_reg)))
    return ReportError ("Can't setup event handler!");

  return true;
}

bool AppAres::PostLoadMap ()
{
  iCelEntityTemplate* worldTpl = pl->FindEntityTemplate ("World");
  iCelEntityTemplate* playerTpl = pl->FindEntityTemplate ("Player");

  pl->ApplyTemplate (world, worldTpl, (iCelParameterBlock*)0);

  iDynamicCell* dyncell = 0;
  // Pick the first cell containing the player or else
  // the first cell.
  iDynamicFactory* playerFact = dynworld->FindFactory ("Player");
  iDynamicObject* playerObj = 0;
  csRef<iDynamicCellIterator> it = dynworld->GetCells ();
  while (it->HasNext ())
  {
    csRef<iDynamicCell> cell = it->NextCell ();
    if (!dyncell) dyncell = cell;
    for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
    {
      iDynamicObject* dynobj = cell->GetObject (i);
      if (dynobj->GetFactory () == playerFact)
      {
	playerObj = dynobj;
	break;
      }
    }
    if (playerObj)
    {
      dyncell = cell;
      break;
    }
  }
  
  if (!dyncell)
    return ReportError ("Can't find suitable start location!");
  if (!playerObj)
    return ReportError ("Can't find player!");

  dynworld->SetCurrentCell (dyncell);
  sector = dyncell->GetSector ();

  csRef<iPcMechanicsSystem> mechsys = celQueryPropertyClassEntity<iPcMechanicsSystem> (world);
  mechsys->SetDynamicSystem (dynworld->GetCurrentCell ()->GetDynamicSystem ());

  player = pl->CreateEntity (playerTpl, "Player", (iCelParameterBlock*)0);
  csRef<iPcCamera> cam = celQueryPropertyClassEntity<iPcCamera> (player);
  camera = cam->GetCamera ();

  csRef<iPcMesh> pcmesh = celQueryPropertyClassEntity<iPcMesh> (player);
  // @@@ Need support for setting transform on pcmesh.
  csReversibleTransform playerTrans = playerObj->GetTransform ();
  pcmesh->MoveMesh (sector, playerTrans.GetOrigin ());

  // Now delete the dummy player object.
  dyncell->DeleteObject (playerObj);

  if (dynworld->IsPhysicsEnabled ())
  {
    csRef<iPcMechanicsObject> mechPlayer = celQueryPropertyClassEntity<iPcMechanicsObject> (player);
    iRigidBody* body = mechPlayer->GetBody ();
    playerTrans.RotateThis (csVector3 (0, 1, 0), M_PI);
    body->SetTransform (playerTrans);
  }

  csRef<iELCM> elcm = csQueryRegistry<iELCM> (object_reg);
  elcm->SetPlayer (player);

  // Initialize collision objects for all loaded objects.
  csColliderHelper::InitializeCollisionWrappers (cdsys, engine);

  nature->InitSector (sector);

  engine->Prepare ();
  //CS::Lighting::SimpleStaticLighter::ShineLights (sector, engine, 4);

  return true;
}

bool AppAres::StartGame (const char* filename)
{
  menu.CleanupMenu ();

  nature = csQueryRegistry<iNature> (object_reg);
  if (!nature)
    return ReportError("Failed to locate nature plugin!");

  world = pl->CreateEntity ("World", 0, 0, "pcworld.dynamic", CEL_PROPCLASS_END);
  if (!world)
    return ReportError ("Failed to create World entity!");
  dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (world);
  {
    csRef<iDynamicCellCreator> cellCreator;
    cellCreator.AttachNew (new AresDynamicCellCreator (this));
    dynworld->SetDynamicCellCreator (cellCreator);
  }

  assetManager = csQueryRegistry<iAssetManager> (object_reg);
  assetManager->SetZone (dynworld);

  if (!assetManager->LoadFile (filename))
    return ReportError ("Error loading '%s'!", filename);

  if (!pl->FindEntityTemplate ("World"))
    if (!LoadLibrary ("/appdata/", "world.xml"))
      return ReportError ("Error loading world library!");

  if (!pl->FindEntityTemplate ("Player"))
    if (!LoadLibrary ("/appdata/", "player.xml"))
      return ReportError ("Error loading player library!");

  if (!PostLoadMap ())
    return ReportError ("Error during PostLoadMap()!");

  csRef<iNativeWindow> natwin = scfQueryInterface<iNativeWindow> (g3d->GetDriver2D ());
  if (natwin)
  {
    natwin->SetWindowTransparent (false);
  }

  // The window is open, so lets make it disappear! 
  if (natwin)
  {
    natwin->SetWindowDecoration (iNativeWindow::decoCaption, true);
    natwin->SetWindowDecoration (iNativeWindow::decoClientFrame, true);
  }

  return true;
}

bool AppAres::ParseCommandLine ()
{
  csRef<iCommandLineParser> cmdline (
  	csQueryRegistry<iCommandLineParser> (object_reg));
  const char* val = cmdline->GetName ();
  if (val)
  {
    menu.SetMode (MENU_GAME);
    StartGame (val);
    return true;
  }
  return false;
}

bool AppAres::Application ()
{
  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  if (!g3d) return ReportError ("No iGraphics3D plugin!");

  csRef<iNativeWindow> natwin = scfQueryInterface<iNativeWindow> (g3d->GetDriver2D ());
  if (natwin)
  {
    natwin->SetWindowTransparent (true);
  }

  // i.e. all windows will be opened.
  if (!OpenApplication (object_reg))
    return ReportError ("Error opening system!");

  // The window is open, so lets make it disappear! 
  if (natwin)
  {
    natwin->SetWindowDecoration (iNativeWindow::decoCaption, false);
    natwin->SetWindowDecoration (iNativeWindow::decoClientFrame, false);
  }

  // The virtual clock.
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  if (!vc) return ReportError ("Can't find the virtual clock!");

  vfs = csQueryRegistry<iVFS> (object_reg);
  if (!vfs) return ReportError("Failed to locate vfs!");

  // Find the pointer to engine plugin
  engine = csQueryRegistry<iEngine> (object_reg);
  if (!engine) return ReportError ("No iEngine plugin!");

  cdsys = csQueryRegistry<iCollideSystem> (object_reg);
  if (!cdsys) return ReportError ("Failed to locate CD system!");

  loader = csQueryRegistry<iLoader> (object_reg);
  if (!loader) return ReportError ("No iLoader plugin!");

  kbd = csQueryRegistry<iKeyboardDriver> (object_reg);
  if (!kbd) return ReportError ("No iKeyboardDriver plugin!");

  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  if (!pl) return ReportError ("CEL physical layer missing!");

  curvedMeshCreator = csQueryRegistry<iCurvedMeshCreator> (object_reg);
  if (!curvedMeshCreator) return ReportError("Failed to load the curved mesh creator plugin!");

  // Setup the name of the application window
  iNativeWindow* nw = g3d->GetDriver2D()->GetNativeWindow ();
  if (nw) nw->SetTitle ("Ares");

  if (!LoadLibrary ("/aresnode/", "library"))
    return ReportError ("Error loading library!");

  if (!menu.SetupMenu ())
    return false;

  printer.AttachNew (new FramePrinter (object_reg));

  ParseCommandLine ();

  // This calls the default runloop. This will basically just keep
  // broadcasting process events to keep the game going.
  Run ();

  return true;
}
