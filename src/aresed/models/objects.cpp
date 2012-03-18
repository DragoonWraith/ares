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

void ObjectsValue::UpdateChildren ()
{
  if (!dirty) return;
  dirty = false;
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

    csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
    composite->AddChild ("ID", NEWREF(LongValue,new LongValue(id)));
    composite->AddChild ("Entity", NEWREF(StringValue,new StringValue(entityName)));
    composite->AddChild ("Factory", NEWREF(StringValue,new StringValue(factName)));
    composite->AddChild ("X", NEWREF(FloatValue,new FloatValue(trans.GetOrigin ().x)));
    composite->AddChild ("Y", NEWREF(FloatValue,new FloatValue(trans.GetOrigin ().y)));
    composite->AddChild ("Z", NEWREF(FloatValue,new FloatValue(trans.GetOrigin ().z)));
    composite->AddChild ("Distance", NEWREF(FloatValue,new FloatValue(dist)));
    children.Push (composite);
    composite->SetParent (this);
  }
}

