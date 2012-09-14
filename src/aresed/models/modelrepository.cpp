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

#include "dynfactmodel.h"
#include "factories.h"
#include "objects.h"
#include "templates.h"
#include "assets.h"
#include "resources.h"
#include "quests.h"
#include "modelrepository.h"


ModelRepository::ModelRepository (AresEdit3DView* view3d, AppAresEditWX* app) :
  scfImplementationType (this), app (app)
{
  dynfactCollectionValue.AttachNew (new DynfactCollectionValue (view3d));
  factoriesValue.AttachNew (new FactoriesValue (app));
  objectsValue.AttachNew (new ObjectsValue (app));
  templatesValue.AttachNew (new TemplatesValue (app));
}

ModelRepository::~ModelRepository ()
{
}

void ModelRepository::Refresh ()
{
  dynfactCollectionValue->Refresh ();
  factoriesValue->Refresh ();
  objectsValue->Refresh ();
  templatesValue->Refresh ();
}

csRef<Ares::Value> ModelRepository::GetObjectsWithEntityValue () const
{
  csRef<Ares::Value> value;
  value.AttachNew (new ObjectsValue (app, true));
  value->Refresh ();
  return value;
}

csRef<Ares::Value> ModelRepository::GetWritableAssetsValue () const
{
  csRef<Ares::Value> value;
  value.AttachNew (new AssetsValue (app, true));
  value->Refresh ();
  return value;
}

csRef<Ares::Value> ModelRepository::GetAssetsValue () const
{
  csRef<Ares::Value> value;
  value.AttachNew (new AssetsValue (app, false));
  value->Refresh ();
  return value;
}

csRef<Ares::Value> ModelRepository::GetResourcesValue () const
{
  csRef<Ares::Value> value;
  value.AttachNew (new ResourcesValue (app));
  value->Refresh ();
  return value;
}

csRef<Ares::Value> ModelRepository::GetQuestsValue () const
{
  csRef<Ares::Value> value;
  value.AttachNew (new QuestsValue (app));
  value->Refresh ();
  return value;
}

Ares::Value* ModelRepository::GetDynfactCollectionValue () const
{
  return dynfactCollectionValue;
}

Ares::Value* ModelRepository::GetObjectsValue () const
{
  return objectsValue;
}

Ares::Value* ModelRepository::GetFactoriesValue () const
{
  factoriesValue->RefreshModel ();
  return factoriesValue;
}

Ares::Value* ModelRepository::GetTemplatesValue () const
{
  templatesValue->RefreshModel ();
  return templatesValue;
}

void ModelRepository::RefreshObjectsValue ()
{
  objectsValue->BuildModel ();
}

iDynamicObject* ModelRepository::GetDynamicObjectFromObjects (Ares::Value* value)
{
  GenericStringArrayValue<iDynamicObject>* dv = static_cast<GenericStringArrayValue<iDynamicObject>*> (value);
  return dv->GetObject ();
}

iObject* ModelRepository::GetResourceFromResources (Ares::Value* value)
{
  GenericStringArrayValue<iObject>* dv = static_cast<GenericStringArrayValue<iObject>*> (value);
  return dv->GetObject ();
}

iAsset* ModelRepository::GetAssetFromAssets (Ares::Value* value)
{
  GenericStringArrayValue<iAsset>* dv = static_cast<GenericStringArrayValue<iAsset>*> (value);
  return dv->GetObject ();
}

size_t ModelRepository::GetDynamicObjectIndexFromObjects (iDynamicObject* dynobj)
{
  return objectsValue->FindObject (dynobj);
}

size_t ModelRepository::GetTemplateIndexFromTemplates (iCelEntityTemplate* tpl)
{
  return templatesValue->FindObject (tpl);
}


// =========================================================================


