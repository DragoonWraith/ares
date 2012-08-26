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

#include "factories.h"
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

#include "propclass/dynworld.h"

using namespace Ares;

static int CompareFactoryValues (
    GenericStringArrayValue<iDynamicFactory>* const & v1,
    GenericStringArrayValue<iDynamicFactory>* const & v2)
{
  const char* s1 = v1->GetStringArrayValue ()->Get (FACTORY_COL_NAME);
  const char* s2 = v2->GetStringArrayValue ()->Get (FACTORY_COL_NAME);
  return strcmp (s1, s2);
}

void FactoriesValue::BuildModel ()
{
  objectsHash.DeleteAll ();
  ReleaseChildren ();

  iPcDynamicWorld* dynworld = app->Get3DView ()->GetDynamicWorld ();
  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    csRef<GenericStringArrayValue<iDynamicFactory> > child;
    child.AttachNew (new GenericStringArrayValue<iDynamicFactory> (fact));
    csStringArray& array = child->GetArray ();
    array.Push (fact->GetName ());
    csString v;
    v.Format ("%d", fact->GetObjectCount ());
    array.Push (v);
    objectsHash.Put (fact, child);
    values.Push (child);
  }
  values.Sort (CompareFactoryValues);
  FireValueChanged ();
}

void FactoriesValue::RefreshModel ()
{
  iPcDynamicWorld* dynworld = app->Get3DView ()->GetDynamicWorld ();
  if (dynworld->GetFactoryCount () != objectsHash.GetSize ())
  {
    // Refresh needed!
    BuildModel ();
    return;
  }

  for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
  {
    iDynamicFactory* fact = dynworld->GetFactory (i);
    StringArrayValue* child = objectsHash.Get (fact, 0);
    if (!child)
    {
      BuildModel ();
      return;
    }
  }
}


