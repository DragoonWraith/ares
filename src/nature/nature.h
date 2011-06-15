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

#ifndef __ARES_NATURE_IMP_H__
#define __ARES_NATURE_IMP_H__

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "iengine/engine.h"
#include "iengine/meshgen.h"
#include "iutil/virtclk.h"
#include "iutil/comp.h"

#include "include/inature.h"

CS_PLUGIN_NAMESPACE_BEGIN(Nature)
{

class Nature : public scfImplementation2<Nature, iNature, iComponent>
{
private:
  iObjectRegistry *object_reg;
  csRef<iEngine> engine;
  csRef<iVirtualClock> vc;
  csRef<iShaderManager> shaderMgr;
  csRef<iShaderVarStringSet> strings;

  iMeshGenerator* meshgen;

  CS::ShaderVarStringID string_sunDirection;
  CS::ShaderVarStringID string_sunTime;
  float sun_alfa;
  float sun_theta;
  float min_light;
  csTicks currentTime;

  /// The sun.
  csRef<iLight> sun;

  void MoveSun (float step, iCamera* camera);

public:
  Nature (iBase *iParent);
  virtual ~Nature ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  virtual void UpdateTime (csTicks ticks, iCamera* camera);
  virtual void InitSector (iSector* sector);

  virtual void SetFoliageDensityFactor (float factor);
  virtual float GetFoliageDensityFactor () const;
};

}
CS_PLUGIN_NAMESPACE_END(Nature)

#endif // __ARES_NATURE_IMP_H__
