/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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

#include <crystalspace.h>

#include "resources.h"
#include "edcommon/tools.h"
#include "edcommon/inspect.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/questmanager.h"
#include "iengine/light.h"

using namespace Ares;

static int CompareResourceValues (
    GenericStringArrayValue<iObject>* const & v1,
    GenericStringArrayValue<iObject>* const & v2)
{
  const char* s1 = v1->GetStringArrayValue ()->Get (RESOURCE_COL_NAME);
  const char* s2 = v2->GetStringArrayValue ()->Get (RESOURCE_COL_NAME);
  return strcmp (s1, s2);
}

void ResourcesValue::AddChild (const char* type, iObject* resource, int usage)
{
  iAssetManager* assetManager = app->GetAssetManager ();

  csRef<GenericStringArrayValue<iObject> > child;
  child.AttachNew (new GenericStringArrayValue<iObject> (resource));
  csStringArray& array = child->GetArray ();
  array.Push (resource->GetName ());
  array.Push (assetManager->IsModified (resource) ? "*" : " ");
  array.Push (type);
  iAsset* asset = assetManager->GetAssetForResource (resource);
  if (asset)
  {
    array.Push (asset->GetNormalizedPath ());
    array.Push (asset->GetFile ());
    array.Push (asset->GetMountPoint ());
  }
  else
  {
    array.Push ("-");
    array.Push ("-");
    array.Push ("-");
  }

  csString usageStr;
  if (usage >= 0)
    usageStr.Format ("%d", usage);
  if (assetManager->IsLocked (resource))
    usageStr.Append (" LOCK");
  array.Push (usageStr);

  objectsHash.Put (resource, child);
  values.Push (child);
}


void ResourcesValue::BuildModel ()
{
  wxBusyCursor wait;
  //wxSafeYield ();

  objectsHash.DeleteAll ();
  ReleaseChildren ();

  iAssetManager* assetManager = app->GetAssetManager ();
  if (!assetManager) return;

  iCelPlLayer* pl = app->Get3DView ()->GetPL ();
  iPcDynamicWorld* dynworld = app->Get3DView ()->GetDynamicWorld ();

  ResourceCounter counter (app->GetObjectRegistry (), dynworld);
  counter.CountResources ();
  const csHash<int,csString>& templateCounter = counter.GetTemplateCounter ();
  const csHash<int,csString>& questCounter = counter.GetQuestCounter ();
  const csHash<int,csString>& lightCounter = counter.GetLightCounter ();

  {
    for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
    {
      iDynamicFactory* fact = dynworld->GetFactory (i);
      AddChild ("dynfact", fact->QueryObject (), fact->GetObjectCount ());
    }
  }

  {
    csRef<iCelEntityTemplateIterator> it = pl->GetEntityTemplates ();
    while (it->HasNext ())
    {
      iObject* o = it->Next ()->QueryObject ();
      AddChild ("template", o, templateCounter.Get (o->GetName (), 0));
    }
  }

  csRef<iQuestManager> questMgr = csQueryRegistry<iQuestManager> (app->GetObjectRegistry ());
  if (questMgr)
  {
    csRef<iQuestFactoryIterator> it = questMgr->GetQuestFactories ();
    while (it->HasNext ())
    {
      iObject* o = it->Next ()->QueryObject ();
      AddChild ("quest", o, questCounter.Get (o->GetName (), 0));
    }
  }

  {
    iLightFactoryList* lf = app->GetEngine ()->GetLightFactories ();
    for (size_t i = 0 ; i < (size_t)lf->GetCount () ; i++)
    {
      iObject* o = lf->Get (i)->QueryObject ();
      AddChild ("lightfact", o, lightCounter.Get (o->GetName (), 0));
    }
  }

  values.Sort (CompareResourceValues);
  FireValueChanged ();
}

void ResourcesValue::RefreshModel ()
{
  BuildModel ();
}


