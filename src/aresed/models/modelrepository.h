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

#ifndef __modelrepository_h
#define __modelrepository_h

#include "editor/imodelrepository.h"

class AresEdit3DView;
class AppAresEditWX;

class DynfactCollectionValue;
class FactoriesValue;
class ObjectsValue;
class TemplatesValue;
class ClassesValue;
class ActionsValue;

/**
 * The model repository.
 */
class ModelRepository : public scfImplementation1<ModelRepository, iModelRepository>
{
private:
  AppAresEditWX* app;
  csRef<DynfactCollectionValue> dynfactCollectionValue;
  csRef<FactoriesValue> factoriesValue;
  csRef<ObjectsValue> objectsValue;
  csRef<TemplatesValue> templatesValue;
  csRef<ClassesValue> classesValue;
  csRef<ActionsValue> actionsValue;

  /// Debug drawing enabled.
public:
  ModelRepository (AresEdit3DView* view3d, AppAresEditWX* app);
  virtual ~ModelRepository ();

  ObjectsValue* GetObjectsValueInt () { return objectsValue; }
  TemplatesValue* GetTemplatesValueInt () { return templatesValue; }

  // Refresh the models after load or save.
  virtual void Refresh ();

  virtual Ares::Value* GetDynfactCollectionValue () const;
  virtual Ares::Value* GetFactoriesValue () const;
  virtual Ares::Value* GetObjectsValue () const;
  virtual Ares::Value* GetTemplatesValue () const;
  virtual csRef<Ares::Value> GetWritableAssetsValue () const;
  virtual csRef<Ares::Value> GetAssetsValue () const;
  virtual csRef<Ares::Value> GetResourcesValue () const;
  virtual csRef<Ares::Value> GetQuestsValue () const;
  virtual Ares::Value* GetClassesValue () const;
  virtual Ares::Value* GetActionsValue () const;
  virtual csRef<Ares::Value> GetObjectsWithEntityValue () const;
  virtual void RefreshObjectsValue ();
  virtual iDynamicObject* GetDynamicObjectFromObjects (Ares::Value* value);
  virtual iObject* GetResourceFromResources (Ares::Value* value);
  virtual iAsset* GetAssetFromAssets (Ares::Value* value);
  virtual size_t GetDynamicObjectIndexFromObjects (iDynamicObject* dynobj);
  virtual size_t GetTemplateIndexFromTemplates (iCelEntityTemplate* tpl);
};

#endif // __modelrepository_h

