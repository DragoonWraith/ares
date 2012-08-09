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

class ObjectsHashIterator : public Ares::ValueIterator
{
private:
  ObjectsHash::GlobalIterator it;

public:
  ObjectsHashIterator (const ObjectsHash::GlobalIterator& it) : it (it) { }
  virtual ~ObjectsHashIterator () { }
  virtual void Reset () { it.Reset (); }
  virtual bool HasNext () { return it.HasNext (); }
  virtual Value* NextChild (csString* name = 0)
  {
    csPtrKey<iDynamicObject> dynobj;
    StringArrayValue* child = it.Next (dynobj);
    return child;
  }
};

csPtr<ValueIterator> ObjectsValue::GetIterator ()
{
  return new ObjectsHashIterator (dynobjs.GetIterator ());
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
    uint id = obj->GetID ();
    iDynamicFactory* fact = obj->GetFactory ();
    const char* entityName = obj->GetEntityName ();
    const char* factName = fact->GetName ();
    const csReversibleTransform& trans = obj->GetTransform ();
    float dist = sqrt (csSquaredDist::PointPoint (trans.GetOrigin (), origin));

    csRef<StringArrayValue> child = View::CreateStringArray (
	VALUE_LONG, id,
	VALUE_STRING, entityName,
	VALUE_STRING, factName,
	VALUE_FLOAT, trans.GetOrigin ().x,
	VALUE_FLOAT, trans.GetOrigin ().y,
	VALUE_FLOAT, trans.GetOrigin ().z,
	VALUE_FLOAT, dist,
	VALUE_NONE);
    dynobjs.Put (obj, child);
  }
  FireValueChanged ();
}

void ObjectsValue::RefreshModel ()
{
  iDynamicCell* cell = app->GetAresView ()->GetDynamicCell ();
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

