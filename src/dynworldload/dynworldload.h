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

#ifndef __ARES_DYNWORLDLOAD_IMP_H__
#define __ARES_DYNWORLDLOAD_IMP_H__

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "iutil/comp.h"
#include "iengine/engine.h"
#include "imap/reader.h"
#include "csutil/strhash.h"

#include "include/icurvemesh.h"
#include "include/irooms.h"
#include "include/inature.h"

#include "physicallayer/pl.h"
#include "propclass/dynworld.h"
#include "tools/dynworldload.h"

CS_PLUGIN_NAMESPACE_BEGIN(DynWorldLoader)
{

class DynamicWorldLoader : public scfImplementation2<DynamicWorldLoader,
  iComponent, iDynamicWorldLoaderExtension>
{
private:
  csRef<iCelPlLayer> pl;
  csRef<iCurvedMeshCreator> curvedMeshCreator;
  csRef<iRoomMeshCreator> roomMeshCreator;
  csRef<iNature> nature;
  csRef<iSyntaxService> synldr;
  csRef<iEngine> engine;
  csStringHash xmltokens;

  bool ParseCurve (iDocumentNode* node, iPcDynamicWorld* dynworld);
  bool ParseRoom (iDocumentNode* node, iPcDynamicWorld* dynworld);
  bool ParseFoliageDensity (iDocumentNode* node, iPcDynamicWorld* dynworld);

public:
  DynamicWorldLoader (iBase *iParent);
  virtual ~DynamicWorldLoader ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  virtual bool Parse (iDocumentNode* node, iPcDynamicWorld* dynworld);
};

}
CS_PLUGIN_NAMESPACE_END(DynWorldLoader)

#endif // __ARES_DYNWORLDLOAD_IMP_H__
