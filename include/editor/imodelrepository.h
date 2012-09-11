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

#ifndef __imodelrepository_h__
#define __imodelrepository_h__

#include "csutil/scf.h"

struct iDynamicObject;
struct iAsset;
struct iObject;
struct iCelEntityTemplate;


namespace Ares
{
  class Value;
}

enum DynObjValueColumns
{
  DYNOBJ_COL_ID = 0,
  DYNOBJ_COL_ENTITY,
  DYNOBJ_COL_TEMPLATE,
  DYNOBJ_COL_FACTORY,
  DYNOBJ_COL_X,
  DYNOBJ_COL_Y,
  DYNOBJ_COL_Z,
  DYNOBJ_COL_DISTANCE,
};

enum TemplateValueColumns
{
  TEMPLATE_COL_NAME = 0,
};

enum QuestValueColumns
{
  QUEST_COL_NAME = 0,
};

enum FactoryValueColumns
{
  FACTORY_COL_NAME = 0,
  FACTORY_COL_USAGE,
};

enum AssetValueColumns
{
  ASSET_COL_WRITABLE = 0,
  ASSET_COL_PATH,
  ASSET_COL_FILE,
  ASSET_COL_MOUNT,
};

enum ResourceValueColumns
{
  RESOURCE_COL_NAME = 0,
  RESOURCE_COL_TYPE,
  RESOURCE_COL_ASSET_PATH,
  RESOURCE_COL_ASSET_FILE,
  RESOURCE_COL_ASSET_MOUNT,
};

/**
 * A repository of all values useful for the editor.
 */
struct iModelRepository : public virtual iBase
{
  SCF_INTERFACE(iModelRepository,0,0,1);

  /**
   * Get the value for the collection of dynamic factories.
   * This version is useful for binding to a tree.
   */
  virtual Ares::Value* GetDynfactCollectionValue () const = 0;

  /**
   * Get the value for the collection of dynamic factories.
   * This version is useful for binding to a list.
   */
  virtual Ares::Value* GetFactoriesValue () const = 0;

  /**
   * Get the value for all the objects.
   */
  virtual Ares::Value* GetObjectsValue () const = 0;

  /**
   * Get the value for all the templates.
   */
  virtual Ares::Value* GetTemplatesValue () const = 0;

  /**
   * Get a value for writable assets.
   */
  virtual csRef<Ares::Value> GetWritableAssetsValue () const = 0;

  /**
   * Get a value for all assets.
   */
  virtual csRef<Ares::Value> GetAssetsValue () const = 0;

  /**
   * Get a value for all resources.
   */
  virtual csRef<Ares::Value> GetResourcesValue () const = 0;

  /**
   * Get a value for all quests.
   */
  virtual csRef<Ares::Value> GetQuestsValue () const = 0;

  /**
   * Refresh all standard values.
   */
  virtual void Refresh () = 0;

  /**
   * Refresh the objects value.
   */
  virtual void RefreshObjectsValue () = 0;

  /**
   * Given a value out of a component that was bound to the objects value
   * this function returns the dynamic object corresponding with that value.
   */
  virtual iDynamicObject* GetDynamicObjectFromObjects (Ares::Value* value) = 0;

  /**
   * Given a value out of a component that was bound to the resources value
   * this function returns the resource corresponding with that value.
   */
  virtual iObject* GetResourceFromResources (Ares::Value* value) = 0;

  /**
   * Given a value out of a component that was bound to the assets value
   * this function returns the asset corresponding with that value.
   */
  virtual iAsset* GetAssetFromAssets (Ares::Value* value) = 0;

  /**
   * Given a dynamic object, find the index of that object it would have in
   * the object list.
   */
  virtual size_t GetDynamicObjectIndexFromObjects (iDynamicObject* dynobj) = 0;

  /**
   * Given a template, find the index of that object it would have in
   * the template list.
   */
  virtual size_t GetTemplateIndexFromTemplates (iCelEntityTemplate* tpl) = 0;
};


#endif // __imodelrepository_h__

