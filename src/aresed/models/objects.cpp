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

#include "objects.h"
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"

using namespace Ares;

class ObjectsIterator : public ValueIterator
{
private:
  csRefArray<DynobjValue> children;
  size_t idx;

public:
  ObjectsIterator (const csRefArray<DynobjValue>& children) :
	children (children), idx (0) { }
  virtual ~ObjectsIterator () { }
  virtual void Reset () { idx = 0; }
  virtual bool HasNext () { return idx < children.GetSize (); }
  virtual Value* NextChild (csString* name = 0)
  {
    idx++;
    return children[idx-1];
  }
};


csPtr<ValueIterator> ObjectsValue::GetIterator ()
{
  return new ObjectsIterator (values);
}

void ObjectsValue::ReleaseChildren ()
{
  ObjectsHash::GlobalIterator it = dynobjs.GetIterator ();
  while (it.HasNext ())
  {
    csPtrKey<iDynamicObject> dynobj;
    StringArrayValue* child = it.Next (dynobj);
    child->SetParent (0);
  }
  dynobjs.DeleteAll ();
  values.DeleteAll ();
}

static int CompareDynobjValues (DynobjValue* const & v1, DynobjValue* const & v2)
{
  const char* s1 = v1->GetStringArrayValue ()->Get (DYNOBJ_COL_FACTORY);
  const char* s2 = v2->GetStringArrayValue ()->Get (DYNOBJ_COL_FACTORY);
  return strcmp (s1, s2);
}


void ObjectsValue::BuildModel ()
{
  dynobjs.DeleteAll ();
  ReleaseChildren ();
  iPcDynamicWorld* dynworld = app->GetAresView ()->GetDynamicWorld ();
  iCamera* camera = app->GetAresView ()->GetCsCamera ();
  const csVector3& origin = camera->GetTransform ().GetOrigin ();
  iDynamicCell* cell = app->GetAresView ()->GetDynamicCell ();
  for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
  {
    iDynamicObject* obj = cell->GetObject (i);

    csRef<DynobjValue> child;
    child.AttachNew (new DynobjValue (obj));
    csStringArray& array = child->GetArray ();
    csString fmt;

    fmt.Format ("%d", obj->GetID ());
    array.Push (fmt);
    array.Push (obj->GetEntityName ());
    iDynamicFactory* fact = obj->GetFactory ();
    array.Push (fact->GetName ());

    const csReversibleTransform& trans = obj->GetTransform ();
    fmt.Format ("%g", trans.GetOrigin ().x);
    array.Push (fmt);
    fmt.Format ("%g", trans.GetOrigin ().y);
    array.Push (fmt);
    fmt.Format ("%g", trans.GetOrigin ().z);
    array.Push (fmt);

    float dist = sqrt (csSquaredDist::PointPoint (trans.GetOrigin (), origin));
    fmt.Format ("%g", dist);
    array.Push (fmt);

    dynobjs.Put (obj, child);
    values.Push (child);
  }
  values.Sort (CompareDynobjValues);
  FireValueChanged ();
}

void ObjectsValue::RefreshModel ()
{
  iDynamicCell* cell = app->GetAresView ()->GetDynamicCell ();
  if (!cell) return;
  if (cell->GetObjectCount () != dynobjs.GetSize ())
  {
    // Refresh needed!
    BuildModel ();
    return;
  }

  for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
  {
    iDynamicObject* obj = cell->GetObject (i);
    StringArrayValue* child = dynobjs.Get (obj, 0);
    if (!child)
    {
      BuildModel ();
      return;
    }
  }
}

size_t ObjectsValue::FindDynObj (iDynamicObject* dynobj) const
{
  for (size_t i = 0 ; i < values.GetSize () ; i++)
    if (values[i]->GetDynamicObject () == dynobj)
      return i;
  return csArrayItemNotFound;
}


