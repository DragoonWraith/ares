/*
The MIT License

Copyright (c) 2011 by Jorrit Tyberghein

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
#include "cssysdef.h"

#include "nature.h"

#include "iutil/objreg.h"
#include "iengine/sector.h"
#include "iengine/camera.h"
#include "iengine/movable.h"
#include "iutil/vfs.h"
#include "imap/loader.h"
#include "csgfx/imagememory.h"


CS_PLUGIN_NAMESPACE_BEGIN(Nature)
{

//---------------------------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (Nature)

Nature::Nature (iBase *iParent)
  : scfImplementationType (this, iParent)
{  
  object_reg = 0;
  sun_alfa = 3.21f;
  sun_theta = 0.206f;
  min_light = 0.0f;
}

Nature::~Nature ()
{
}

bool Nature::Initialize (iObjectRegistry *object_reg)
{
  Nature::object_reg = object_reg;
  engine = csQueryRegistry<iEngine> (object_reg);
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  strings = csQueryRegistryTagInterface<iShaderVarStringSet> (object_reg, "crystalspace.shader.variablenameset");
  string_sunDirection = strings->Request ("sun direction");
  string_sunTime = strings->Request("timeOfDay");

  return true;
}

void Nature::CleanUp ()
{
  sun_alfa = 3.21f;
  sun_theta = 0.206f;
  min_light = 0.0f;
  foliage_density_maps.Empty ();
  sun = 0;
}

void Nature::InitSector (iSector* sector)
{
  iLightList* lightList = sector->GetLights ();
  sun = engine->CreateLight("Sun", csVector3 (10.0f), 9000, csColor (0.3f, 0.2f, 0.1f));
  lightList->Add (sun);
  // @@@ HARDCODED!
  meshgen = sector->GetMeshGeneratorByName ("grass");
}

iImage* Nature::GetFoliageDensityMapImage (size_t idx)
{
  iImage* image = foliage_density_maps[idx].image;
  if (!image)
  {
    csString image_name = foliage_density_maps[idx].image_name;
    csRef<iLoader> loader = csQueryRegistry<iLoader> (object_reg);
    csRef<iImage> source = loader->LoadImage (image_name);
    // @@@ Error checking.
    image = new csImageMemory (source);
    foliage_density_maps[idx].image.AttachNew (image);
  }
  return image;
}

void Nature::MoveSun (float step, iCamera* cam)
{
  //=[ Sun position ]===================================
  //TODO: Make the sun stay longer at its highest point at noon.
  float temp = step * 2.0f;
  if (temp > 1.0f) temp  = 2.0f - temp;

  sun_theta = (2.0f*temp - 1.0f)*0.85;
  sun_alfa = 1.605f * sin(-step * 2.0f*PI) - 3.21f;

  // Update the values.
  csVector3 sun_vec;
  sun_vec.x = cos(sun_theta)*sin(sun_alfa);
  sun_vec.y = sin(sun_theta);
  //if (sun_vec.y <= 0) sun_vec.y = 0;
  sun_vec.z = cos(sun_theta)*cos(sun_alfa);
  if (!shaderMgr)
    shaderMgr = csQueryRegistry<iShaderManager> (object_reg);
  csShaderVariable* var = shaderMgr->GetVariableAdd(string_sunDirection);
  var->SetValue(sun_vec);

  // Set the sun position.
  csReversibleTransform trans(csMatrix3(), (sun_vec*1000.0f)+cam->GetTransform().GetOrigin());
  trans.LookAt (sun_vec*-1, csVector3(0,1,0));
  sun->GetMovable()->SetTransform (trans);
  sun->GetMovable()->UpdateMove();

  //=[ Sun brightness ]===================================
  // This is just "Lambert's cosine law" shifted so midday is 0, and
  // multiplied by 1.9 instead of 2 to extend the daylight after sunset to
  float brightness = cos((step - 0.5f) * PI * 1.9f);
  //csColor sunlight(brightness * 1.5f);
  csColor sunlight(brightness);
  sunlight.ClampDown();
  // The ambient color is adjusted to give a slightly more yellow colour at
  // midday, graduating to a purplish blue at midnight. Adjust "min_light"
  // in options to make it playable at night.
  float amb = cos((step - 0.5f) * PI * 2.2f);
  csColor ambient((amb*0.125f)+0.075f+min_light, (amb*0.15f)+0.05f+min_light,
        (amb*0.1f)+0.08f+min_light);
  ambient.ClampDown();

  // Update the values.
  cam->GetSector ()->SetDynamicAmbientLight(ambient);
  sun->SetColor(sunlight);

  //=[ Clouds ]========================================
  //<shadervar type="vector3" name="cloudcol">0.98,0.59,0.46</shadervar>
  float brightnessc = (amb * 0.6f) + 0.4f;
  csRef<csShaderVariable> sv = shaderMgr->GetVariableAdd(string_sunTime);
  sv->SetValue(brightnessc);
}

void Nature::UpdateTime (csTicks ticks, iCamera* cam)
{
  if (!cam->GetSector ()) return;

  static float lastStep = -1000.0f;
  float step = float (ticks % 100000) / 100000.0;

  // Don't update if the time has not changed much.
  if ((step - lastStep) < 0.0001f && (step - lastStep) > -0.0001f) return;
  lastStep = step;

  MoveSun (step, cam);
}

void Nature::SetFoliageDensityFactor (float factor)
{
  meshgen->SetDefaultDensityFactor (factor);
}

float Nature::GetFoliageDensityFactor () const
{
  return meshgen->GetDefaultDensityFactor ();
}

}
CS_PLUGIN_NAMESPACE_END(Nature)

