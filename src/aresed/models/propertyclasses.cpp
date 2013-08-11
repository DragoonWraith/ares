/*
The MIT License

Copyright (c) 2013 by Jorrit Tyberghein

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

#include "propertyclasses.h"
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

#include "physicallayer/entitytpl.h"

using namespace Ares;


static int CompareDynobjValues (
    GenericStringArrayValue<iCelPropertyClassTemplate>* const & v1,
    GenericStringArrayValue<iCelPropertyClassTemplate>* const & v2)
{
  const char* s1 = v1->GetStringArrayValue ()->Get (PC_COL_ENTITY);
  const char* s2 = v2->GetStringArrayValue ()->Get (PC_COL_ENTITY);
  return strcmp (s1, s2);
}


void PCValue::BuildModel ()
{
  dirty = false;
  objectsHash.DeleteAll ();
  ReleaseChildren ();
  iCamera* camera = app->GetAresView ()->GetCsCamera ();
  const csVector3& origin = camera->GetTransform ().GetOrigin ();
  iDynamicCell* cell = app->GetAresView ()->GetDynamicCell ();
  if (!cell)
  {
    FireValueChanged ();
    return;
  }
  for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
  {
    iDynamicObject* obj = cell->GetObject (i);
    iCelEntityTemplate* tpl = obj->GetEntityTemplate ();
    if (!obj->GetEntityName () || !tpl) continue;

    for (size_t j = 0 ; j < tpl->GetPropertyClassTemplateCount () ; j++)
    {
      iCelPropertyClassTemplate* pctpl = tpl->GetPropertyClassTemplate (j);
      if (pcname == pctpl->GetName ())
      {
        csRef<GenericStringArrayValue<iCelPropertyClassTemplate> > child;
        child.AttachNew (new GenericStringArrayValue<iCelPropertyClassTemplate> (pctpl));
        csStringArray& array = child->GetArray ();
        csString fmt;

        fmt.Format ("%d", obj->GetID ());
        array.Push (fmt);
        array.Push (obj->GetEntityName ());
        array.Push (pctpl->GetTag ());

        csString tplName = obj->GetEntityTemplate ()->GetName ();
        if (obj->GetEntityParameters ())
          tplName += '#';
        array.Push (tplName);
        iDynamicFactory* fact = obj->GetFactory ();
        array.Push (fact->GetName ());

        objectsHash.Put (pctpl, child);
        values.Push (child);
      }
    }
  }
  values.Sort (CompareDynobjValues);
  FireValueChanged ();
}

void PCValue::RefreshModel ()
{
  iDynamicCell* cell = app->GetAresView ()->GetDynamicCell ();
  if (!cell)
  {
    BuildModel ();
    return;
  }
  BuildModel ();
}



