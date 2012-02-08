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

#include "common/worldload.h"

#define PATHFIND_VERBOSE 0

//-----------------------------------------------------------------------------

AppAres::AppAres ()
{
  SetApplicationName ("Ares");
  worldLoader = 0;
  currentTime = 31000;
}

AppAres::~AppAres ()
{
  delete worldLoader;
}

void AppAres::OnExit ()
{
  if (pl) pl->CleanCache ();
  printer.Invalidate ();
}

void AppAres::Frame ()
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

void AppAres::CreateActionIcon ()
{
  csRef<iCelEntity> entity = pl->CreateEntity ("action_icon", 0, 0,
      "pc2d.billboard",
      CEL_PROPCLASS_END);
  if (!entity) return;

  iTextureWrapper* txt = engine->CreateTexture ("action_icon", "/lib/stdtex/tile.png",
      0, CS_TEXTURE_2D);
  txt->Register (g3d->GetTextureManager ());
  engine->CreateMaterial ("action_icon", txt);

  csRef<iPcBillboard> pcbb = celQueryPropertyClassEntity<iPcBillboard> (entity);
  pcbb->SetBillboardName ("action_icon");
  iBillboard* bb = pcbb->GetBillboard ();
  bb->SetMaterialName ("action_icon");
  bb->SetPosition (1000, 1000);
  bb->SetSize (30000, 30000);
  //bb->GetFlags ().SetAll (CEL_BILLBOARD_VISIBLE);
  bb->GetFlags ().SetAll (0);
}

void AppAres::CreateSettingBar ()
{
  csRef<iCelEntity> entity = pl->CreateEntity ("setting_bar", 0, 0,
      "pc2d.billboard",
      CEL_PROPCLASS_END);
  if (!entity) return;

  csRef<iPcBillboard> pcbb = celQueryPropertyClassEntity<iPcBillboard> (entity);
  pcbb->SetBillboardName ("setting_bar");
  iBillboard* bb = pcbb->GetBillboard ();
  bb->SetText ("Setting:");
  bb->SetPosition (100000, 1000);
  bb->SetSize (200000, 30000);
  bb->GetFlags ().SetAll (0);
}

void AppAres::CreateActor ()
{
  // The Real Camera
  entity_cam = pl->CreateEntity ("camera", 0, 0,
    "pcinput.standard",
    "pcmove.analogmotion",
    "pcmove.jump",
    "pcmove.grab",
    "pccamera.delegate",
    "pccamera.mode.tracking",
    "pcobject.mesh",
    "pcobject.mesh.select",
    "pcmove.linear",
    "pcmove.actor.wasd",
    //"pc2d.tooltip",
    "pctools.inventory",
    "pctools.timer",
    "pcsound.listener",
    "pclogic.trigger",
    "pcmisc.test",
    "pctools.properties",
    "pctools.bag",
    CEL_PROPCLASS_END);
  if (!entity_cam) return;

  csRef<iPcCommandInput> pcinp = celQueryPropertyClassEntity<iPcCommandInput> (entity_cam);
  pcinp->Bind ("JoystickButton4", "ready");
  pcinp->Bind ("JoystickButton6", "lockon");
  pcinp->Bind ("JoystickButton2", "resetcam");
  pcinp->Bind ("JoystickAxis0", "joyaxis0");
  pcinp->Bind ("JoystickAxis1", "joyaxis1");
  pcinp->Bind ("MouseAxis0_centered", "mouseaxis0");

  pcinp->Bind ("e", "action");
  pcinp->Bind ("z", "ready");
  pcinp->Bind ("x", "lockon");
  pcinp->Bind ("c", "resetcam");
  pcinp->Bind ("left", "left");
  pcinp->Bind ("right", "right");
  pcinp->Bind ("up", "up");
  pcinp->Bind ("down", "down");
  pcinp->Bind ("a", "left");
  pcinp->Bind ("d", "right");
  pcinp->Bind ("w", "up");
  pcinp->Bind ("s", "down");
  pcinp->Bind ("space", "jump");
  pcinp->Bind ("m", "freeze");
  pcinp->Bind ("shift", "roll");
  pcinp->Bind ("`", "showstates");
  pcinp->Bind ("[", "camleft");
  pcinp->Bind ("]", "camright");
  pcinp->Bind ("pageup", "camup");
  pcinp->Bind ("pagedown", "camdown");

  pcinp->Bind ("pad5", "settings");
  pcinp->Bind ("pad2", "settings_down");
  pcinp->Bind ("pad8", "settings_up");
  pcinp->Bind ("pad4", "settings_left");
  pcinp->Bind ("pad6", "settings_right");
  pcinp->Bind ("pad7", "settings_fastleft");
  pcinp->Bind ("pad9", "settings_fastright");
  pcinp->Bind ("pad1", "settings_slowleft");
  pcinp->Bind ("pad3", "settings_slowright");

  csRef<iPcJump> jump = celQueryPropertyClassEntity<iPcJump> (entity_cam);
  jump->SetBoostJump (false);
  jump->SetJumpHeight (1.0f);
  //jump->SetDoubleJumpSpeed (7.0f);

  csRef<iPcTrackingCamera> trackcam = celQueryPropertyClassEntity<iPcTrackingCamera> (entity_cam);
  trackcam->SetPanSpeed (2.5f);
  trackcam->SetTiltSpeed (1.2f);

  csRef<iPcCamera> cam = celQueryPropertyClassEntity<iPcCamera> (entity_cam);
  camera = cam->GetCamera ();

  csRef<iPcMesh> pcmesh = celQueryPropertyClassEntity<iPcMesh> (entity_cam);
  pcmesh->SetPath ("/lib/kwartz");
  pcmesh->SetMesh ("kwartz_fact", "kwartz.lib");
  pcmesh->MoveMesh (sector, csVector3 (0, 10, 0));

  csRef<iPcLinearMovement> pclinmove = celQueryPropertyClassEntity<iPcLinearMovement> (entity_cam);
  pclinmove->InitCD (
      csVector3 (0.5f,  0.8f, 0.5f),
      csVector3 (0.5f,  0.4f, 0.5f),
      csVector3 (0.0f, -0.4f, 0.0f));

  csRef<iPcTrigger> trigger = celQueryPropertyClassEntity<iPcTrigger> (entity_cam);
  trigger->SetupTriggerSphere (0, csVector3 (0), 1.0);
  trigger->SetFollowEntity (true);
}

void AppAres::ConnectWires ()
{
  iCelEntity* action_icon = pl->FindEntity ("action_icon");

  iCelPropertyClass* pc;
  csRef<iPcWire> wire;
  csRef<celOneParameterBlock> params;
  size_t idx;

  pc = pl->CreatePropertyClass (entity_cam, "pclogic.wire");
  // 1: For debugging, print out when we are near an entity.
  wire = scfQueryInterface<iPcWire> (pc);
  wire->AddInput ("cel.trigger.entity.enter");
  idx = wire->AddOutput (pl->FetchStringID ("cel.test.action.Print"));
  wire->MapParameterExpression (idx, "message", "'We found '+@entity");
  // 2: Add the name of the entity to the bag.
  idx = wire->AddOutput (pl->FetchStringID ("cel.bag.action.AddString"));
  wire->MapParameterExpression (idx, "value", "entname(@entity)");
  // 3: Set the visibility state of the billboard.
  params.AttachNew (new celOneParameterBlock (pl->FetchStringID ("name"), "visible"));
  idx = wire->AddOutput (pl->FetchStringID ("cel.billboard.action.SetProperty"),
      action_icon->QueryMessageChannel (), params);
  // Use > 1 because there is also the 'camera' entity itself.
  wire->MapParameterExpression (idx, "value", "property(pc(pctools.bag),id(size))>1");

  // 1: For debugging, print out when we are far from an entity.
  pc = pl->CreatePropertyClass (entity_cam, "pclogic.wire");
  wire = scfQueryInterface<iPcWire> (pc);
  wire->AddInput ("cel.trigger.entity.leave");
  idx = wire->AddOutput (pl->FetchStringID ("cel.test.action.Print"));
  wire->MapParameterExpression (idx, "message", "'We leave '+@entity");
  // 2: Remove the name of the entity from the bag.
  idx = wire->AddOutput (pl->FetchStringID ("cel.bag.action.RemoveString"));
  wire->MapParameterExpression (idx, "value", "entname(@entity)");
  // 3: Set the visibility state of the billboard.
  params.AttachNew (new celOneParameterBlock (pl->FetchStringID ("name"), "visible"));
  idx = wire->AddOutput (pl->FetchStringID ("cel.billboard.action.SetProperty"),
      action_icon->QueryMessageChannel (), params);
  // Use > 1 because there is also the 'camera' entity itself.
  wire->MapParameterExpression (idx, "value", "property(pc(pctools.bag),id(size))>1");

  // If the action key is pressed we send out a 'cel.game.action' to the current entities.
  pc = pl->CreatePropertyClass (entity_cam, "pclogic.wire");
  wire = scfQueryInterface<iPcWire> (pc);
  wire->AddInput ("cel.input.action.up");
  params.AttachNew (new celOneParameterBlock (pl->FetchStringID ("msgid"), "cel.game.action"));
  wire->AddOutput (pl->FetchStringID ("cel.bag.action.SendMessage"), 0, params);
}

bool AppAres::InitPhysics ()
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

  return true;
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
  	"/this/AppAres.cfg"))
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
	CS_REQUEST_PLUGIN ("cel.persistence.xml", iCelPersistence),
	CS_REQUEST_PLUGIN ("crystalspace.collisiondetection.opcode",
		iCollideSystem),
	CS_REQUEST_PLUGIN ("crystalspace.sndsys.element.loader", iSndSysLoader),
	CS_REQUEST_PLUGIN ("crystalspace.sndsys.renderer.software",
		iSndSysRenderer),
	CS_REQUEST_PLUGIN("crystalspace.dynamics.bullet", iDynamics),
	CS_REQUEST_PLUGIN("utility.nature", iNature),
	CS_REQUEST_PLUGIN("utility.curvemesh", iCurvedMeshCreator),
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
  dyncell = dynworld->AddCell ("outside", sector, dynSys);
  dynworld->SetCurrentCell (dyncell);

  // Initialize collision objects for all loaded objects.
  csColliderHelper::InitializeCollisionWrappers (cdsys, engine);

  CreateActor ();

  // Find the terrain mesh
  csRef<iMeshWrapper> terrainWrapper = engine->FindMeshObject ("Terrain");
  if (!terrainWrapper)
  {
    ReportError("Error cannot find the terrain mesh!");
    return false;
  }

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

  nature->InitSector (sector);

  engine->Prepare ();
  //CS::Lighting::SimpleStaticLighter::ShineLights (sector, engine, 4);

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

  // Create the scene
  zoneEntity = pl->CreateEntity ("zone", 0, 0,
      "pcworld.dynamic", CEL_PROPCLASS_END);
  if (!zoneEntity) return ReportError ("Failed to create zone entity!");
  dynworld = celQueryPropertyClassEntity<iPcDynamicWorld> (entity_cam);

  nature = csQueryRegistry<iNature> (object_reg);
  if (!nature) return ReportError("Failed to locate nature plugin!");

  worldLoader = new WorldLoader (object_reg);
  worldLoader->SetZone (dynworld);

  if (!InitPhysics ())
    return false;

  worldLoader->LoadFile ("/saves/testworld");
  sector = engine->FindSector ("room");

  if (!PostLoadMap ())
    return ReportError ("Error during PostLoadMap()!");

  printer.AttachNew (new FramePrinter (object_reg));

  // This calls the default runloop. This will basically just keep
  // broadcasting process events to keep the game going.
  Run ();

  return true;
}
