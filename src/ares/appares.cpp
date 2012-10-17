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

AppAres::AppAres ()
{
  SetApplicationName ("Ares");
  currentTime = 31000;
  menu = MENU_LIST;
  mouseX = 0;
  mouseY = 0;
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
  if (menu == MENU_LIST)
  {
    iGraphics2D* g2d = g3d->GetDriver2D ();
    g2d->Clear (g2d->FindRGB (0, 0, 0));
    int fg = g2d->FindRGB (255, 255, 255);
    int fgSel = g2d->FindRGB (0, 0, 0);
    int bg = g2d->FindRGB (0, 0, 0);
    int bgSel = g2d->FindRGB (128, 128, 128);
    for (size_t i = 0 ; i < menuGames.GetSize () ; i++)
    {
      const MenuEntry& me = menuGames[i];
      if (mouseX >= me.x1 && mouseX <= me.x2 && mouseY >= me.y1 && mouseY <= me.y2)
        g2d->Write (menuFont, me.x1, me.y1, fgSel, bgSel, me.name);
      else
        g2d->Write (menuFont, me.x1, me.y1, fg, bg, me.name);
    }
  }
  else if (menu == MENU_WAIT1)
  {
    iGraphics2D* g2d = g3d->GetDriver2D ();
    g2d->Clear (g2d->FindRGB (0, 0, 0));
    g2d->Write (menuFont, 100, 100, g2d->FindRGB (255, 128, 128), g2d->FindRGB (0, 0, 0),
	"Loading ...");
    menu = MENU_WAIT2;
  }
  else if (menu == MENU_WAIT2)
  {
    if (gameFile)
      StartGame (gameFile);
  }
  else
  {
    // We let the entity system do this so there is nothing here.
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
  if (menu != MENU_LIST) return false;
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);
  return false;
}

bool AppAres::OnMouseDown (iEvent &ev)
{
  if (menu != MENU_LIST) return false;
  mouseX = csMouseEventHelper::GetX (&ev);
  mouseY = csMouseEventHelper::GetY (&ev);
  for (size_t i = 0 ; i < menuGames.GetSize () ; i++)
  {
    const MenuEntry& me = menuGames[i];
    if (mouseX >= me.x1 && mouseX <= me.x2 && mouseY >= me.y1 && mouseY <= me.y2)
    {
      gameFile = me.name;
      menu = MENU_WAIT1;
      return true;
    }
  }
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

bool AppAres::InitPhysics ()
{
  if (dynworld->IsPhysicsEnabled ())
  {
    dyn = csQueryRegistry<iDynamics> (GetObjectRegistry ());
    if (!dyn) return ReportError ("Error loading bullet plugin!");

    dynSys = dyn->CreateSystem ();
    if (!dynSys) return ReportError ("Error creating dynamic system!");
    //dynSys->SetLinearDampener(.3f);
    dynSys->SetRollingDampener(.995f);
    dynSys->SetGravity (csVector3 (0.0f, -9.81f, 0.0f));

    bullet_dynSys = scfQueryInterface<CS::Physics::Bullet::iDynamicSystem> (dynSys);
    bullet_dynSys->SetInternalScale (1.0f);
    bullet_dynSys->SetStepParameters (0.005f, 2, 10);
  }

  return true;
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
    csRef<iDynamicSystem> ds = dyn->FindSystem (systemname);
    if (!ds)
    {
      ds = dyn->CreateSystem ();
      ds->QueryObject ()->SetName (systemname);
    }
    if (!ds) { ReportError ("Error creating dynamic system!"); return 0; }

    //ds->SetLinearDampener(.3f);
    ds->SetRollingDampener(.995f);
    ds->SetGravity (csVector3 (0.0f, -19.81f, 0.0f));

    csRef<CS::Physics::Bullet::iDynamicSystem> bullet_ds = scfQueryInterface<
      CS::Physics::Bullet::iDynamicSystem> (ds);
    //@@@ (had to disable because bodies might alredy exist!) bullet_ds->SetInternalScale (1.0f);
    bullet_ds->SetStepParameters (0.005f, 2, 10);
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
	CS_REQUEST_PLUGIN ("cel.persistence.xml", iCelPersistence),
	CS_REQUEST_PLUGIN ("crystalspace.collisiondetection.opcode", iCollideSystem),
	CS_REQUEST_PLUGIN ("crystalspace.sndsys.element.loader", iSndSysLoader),
	//CS_REQUEST_PLUGIN ("crystalspace.sndsys.renderer.software", iSndSysRenderer),
	CS_REQUEST_PLUGIN("crystalspace.dynamics.bullet", iDynamics),
	CS_REQUEST_PLUGIN("utility.nature", iNature),
	CS_REQUEST_PLUGIN("utility.curvemesh", iCurvedMeshCreator),
	CS_REQUEST_PLUGIN("utility.rooms", iRoomMeshCreator),
	CS_REQUEST_PLUGIN("utility.assetmanager", iAssetManager),
	CS_REQUEST_END))
  {
    return ReportError ("Can't initialize plugins!");
  }

  csBaseEventHandler::Initialize (object_reg);

  // Now we need to setup an event handler for our application.
  // Crystal Space is fully event-driven. Everything (except for this
  // initialization) happens in an event.
  if (!RegisterQueue (object_reg, csevAllEvents (object_reg)))
    return ReportError ("Can't setup event handler!");

  return true;
}

bool AppAres::PostLoadMap ()
{
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

  // Find the terrain mesh. @@@ HARDCODED!
  csRef<iMeshWrapper> terrainWrapper = engine->FindMeshObject ("Terrain");
  if (terrainWrapper)
  {
    csRef<iTerrainSystem> terrain =
      scfQueryInterface<iTerrainSystem> (terrainWrapper->GetMeshObject ());
    if (!terrain)
    {
      ReportError("Error cannot find the terrain interface!");
      return false;
    }

    // Create a terrain collider for each cell of the terrain
    for (size_t i = 0; i < terrain->GetCellCount (); i++)
      bullet_dynSys->AttachColliderTerrain (terrain->GetCell (i));
  }

  nature->InitSector (sector);

  engine->Prepare ();
  //CS::Lighting::SimpleStaticLighter::ShineLights (sector, engine, 4);

  return true;
}

bool AppAres::SetupMenu ()
{
  menuGames.Empty ();
  csRef<iStringArray> vfsFiles = vfs->FindFiles ("/saves/");
  for (size_t i = 0; i < vfsFiles->GetSize(); i++)
  {
    csString file = (char*)vfsFiles->Get(i);
    if (file.Length () == 0) continue;
    if (!file.EndsWith (".ares")) continue;
    MenuEntry entry;
    entry.name = file;
    entry.x1 = 10;
    entry.x2 = g3d->GetDriver2D ()->GetWidth ()-20;
    entry.y1 = 10 + i*30;
    entry.y2 = 10 + i*30 + 26;
    menuGames.Push (entry);
  }

  menu = MENU_LIST;

  return true;
}

bool AppAres::StartGame (const char* filename)
{
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

  iCelEntityTemplate* worldTpl = pl->FindEntityTemplate ("World");
  if (!worldTpl)
  {
    if (!LoadLibrary ("/appdata/", "world.xml"))
      return ReportError ("Error loading world library!");
    worldTpl = pl->FindEntityTemplate ("World");
  }

  iCelEntityTemplate* playerTpl = pl->FindEntityTemplate ("Player");
  if (!playerTpl)
  {
    if (!LoadLibrary ("/appdata/", "player.xml"))
      return ReportError ("Error loading player library!");
    playerTpl = pl->FindEntityTemplate ("Player");
  }

  if (!InitPhysics ())
    return false;

  // Create the scene
  pl->ApplyTemplate (world, worldTpl, (iCelParameterBlock*)0);

  player = pl->CreateEntity (playerTpl, "Player", (iCelParameterBlock*)0);
  csRef<iPcCamera> cam = celQueryPropertyClassEntity<iPcCamera> (player);
  camera = cam->GetCamera ();

  if (!PostLoadMap ())
    return ReportError ("Error during PostLoadMap()!");

  menu = MENU_GAME;

  return true;
}

bool AppAres::Application ()
{
  // i.e. all windows will be opened.
  if (!OpenApplication (object_reg))
    return ReportError ("Error opening system!");

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

  g3d = csQueryRegistry<iGraphics3D> (object_reg);
  if (!g3d) return ReportError ("No iGraphics3D plugin!");

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

  menuFont = g3d->GetDriver2D ()->GetFontServer ()->LoadFont (CSFONT_LARGE);

  if (!SetupMenu ())
    return false;

  printer.AttachNew (new FramePrinter (object_reg));

  // This calls the default runloop. This will basically just keep
  // broadcasting process events to keep the game going.
  Run ();

  return true;
}
