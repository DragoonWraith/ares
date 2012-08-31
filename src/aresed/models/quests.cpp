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

#include "quests.h"
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

#include "tools/questmanager.h"

using namespace Ares;

static int CompareQuestValues (
    GenericStringArrayValue<iQuestFactory>* const & v1,
    GenericStringArrayValue<iQuestFactory>* const & v2)
{
  const char* s1 = v1->GetStringArrayValue ()->Get (QUEST_COL_NAME);
  const char* s2 = v2->GetStringArrayValue ()->Get (QUEST_COL_NAME);
  return strcmp (s1, s2);
}

void QuestsValue::BuildModel ()
{
  objectsHash.DeleteAll ();
  ReleaseChildren ();

  iAssetManager* assetManager = app->GetAssetManager ();
  if (!assetManager) return;

  csRef<iQuestManager> questmgr = csQueryRegistryOrLoad<iQuestManager> (app->GetObjectRegistry (),
	"cel.manager.quests");

  csRef<iQuestFactoryIterator> it = questmgr->GetQuestFactories ();
  while (it->HasNext ())
  {
    iQuestFactory* fact = it->Next ();
    csRef<GenericStringArrayValue<iQuestFactory> > child;
    child.AttachNew (new GenericStringArrayValue<iQuestFactory> (fact));
    csStringArray& array = child->GetArray ();
    if (assetManager->IsModified (fact->QueryObject ()))
      array.Push (csString (fact->GetName ()) + "*");
    else
      array.Push (fact->GetName ());
    objectsHash.Put (fact, child);
    values.Push (child);
  }
  values.Sort (CompareQuestValues);
  FireValueChanged ();
}

void QuestsValue::RefreshModel ()
{
  csRef<iQuestManager> questmgr = csQueryRegistryOrLoad<iQuestManager> (app->GetObjectRegistry (),
	"cel.manager.quests");

  csRef<iQuestFactoryIterator> it = questmgr->GetQuestFactories ();
  size_t cnt = 0;
  while (it->HasNext ())
  {
    it->Next ();
    cnt++;
  }

  if (cnt != objectsHash.GetSize ())
  {
    // Refresh needed!
    BuildModel ();
    return;
  }

  it = questmgr->GetQuestFactories ();
  while (it->HasNext ())
  {
    iQuestFactory* fact = it->Next ();
    StringArrayValue* child = objectsHash.Get (fact, 0);
    if (!child)
    {
      BuildModel ();
      return;
    }
  }
}


