/*
The MIT License

Copyright (c) 2011 by Jorrit Tyberghein

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
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

using namespace Ares;

void CategoryCollectionValue::UpdateChildren ()
{
  if (!dirty) return;
  dirty = false;
  ReleaseChildren ();
  iAssetManager* assetManager = aresed3d->GetApp ()->GetAssetManager ();
  iPcDynamicWorld* dynworld = aresed3d->GetDynamicWorld ();

  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  const csStringArray& items = categories.Get (category, csStringArray ());
  for (size_t i = 0 ; i < items.GetSize () ; i++)
  {
    csRef<StringValue> strValue;
    csString name = items[i];
    iDynamicFactory* factory = dynworld->FindFactory (name);
    if (factory && assetManager->IsModified (factory->QueryObject ()))
      name += "*";
    strValue.AttachNew (new StringValue (name));
    children.Push (strValue);
    strValue->SetParent (this);
  }
}

void DynfactCollectionValue::UpdateChildren ()
{
  if (!dirty) return;
  dirty = false;
  ReleaseChildren ();
  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  csHash<csStringArray,csString>::ConstGlobalIterator it = categories.GetIterator ();
  while (it.HasNext ())
  {
    csString category;
    it.Next (category);
    csRef<CategoryCollectionValue> catValue;
    catValue.AttachNew (new CategoryCollectionValue (aresed3d, category));
    children.Push (catValue);
    catValue->SetParent (this);
  }
}

static void CorrectFactoryName (csString& name)
{
  if (name[name.Length ()-1] == '*')
    name = name.Slice (0, name.Length ()-1);
}

bool DynfactCollectionValue::DeleteValue (Value* child)
{
  iUIManager* ui = aresed3d->GetApp ()->GetUIManager ();
  if (!child)
  {
    ui->Error ("Please select an item!");
    return false;
  }
  Value* categoryValue = GetCategoryForValue (child);
  if (!categoryValue)
  {
    ui->Error ("Please select an item!");
    return false;
  }
  if (categoryValue == child)
  {
    ui->Error ("You can't delete an entire category at once!");
    return false;
  }

  iPcDynamicWorld* dynworld = aresed3d->GetDynamicWorld ();
  csString factoryName = child->GetStringValue ();
  CorrectFactoryName (factoryName);
  iDynamicFactory* factory = dynworld->FindFactory (factoryName);
  CS_ASSERT (factory != 0);
  if (factory->GetObjectCount () > 0)
  {
    if (ui->Ask ("There are still %d objects using this factory. Do you want to delete them?",
	factory->GetObjectCount ()))
    {
      aresed3d->GetSelection ()->SetCurrentObject (0);
      csRef<iDynamicCellIterator> it = dynworld->GetCells ();
      while (it->HasNext ())
      {
	iDynamicCell* cell = it->NextCell ();
	csArray<iDynamicObject*> toDelete;
	for (size_t i = 0 ; i < cell->GetObjectCount () ; i++)
	{
	  iDynamicObject* o = cell->GetObject (i);
	  if (o->GetFactory () == factory)
	    toDelete.Push (o);
	}
	for (size_t i = 0 ; i < toDelete.GetSize () ; i++)
	  cell->DeleteObject (toDelete[i]);
      }
    }
    else
      return false;
  }
  dynworld->RemoveFactory (factory);
  aresed3d->GetApp ()->GetAssetManager ()->RegisterRemoval (factory->QueryObject ());

  aresed3d->RemoveItem (categoryValue->GetStringValue (), factoryName);

  dirty = true;
  FireValueChanged ();

  return true;
}

static float Gf (const DialogResult& suggestion, const char* name)
{
  float f;
  csScanStr (suggestion.Get (name, "0"), "%f", &f);
  return f;
}

Value* DynfactCollectionValue::NewValue (size_t idx, Value* selectedValue,
    const DialogResult& suggestion)
{
  csString newname = suggestion.Get ("name", (const char*)0);
  if (newname.IsEmpty ())
  {
    aresed3d->GetApp ()->GetUIManager ()->Error ("Enter a valid mesh!");
    return 0;
  }
  Value* categoryValue = GetCategoryForValue (selectedValue);
  if (!categoryValue)
  {
    aresed3d->GetApp ()->GetUIManager ()->Error ("Please select a category!");
    return 0;
  }

  csRef<StringValue> strValue;
  strValue.AttachNew (new StringValue (newname));

  iPcDynamicWorld* dynworld = aresed3d->GetDynamicWorld ();

  iDynamicFactory* fact;
  // Check if it is a logic factory.
  if (suggestion.Contains ("minx"))
  {
    csBox3 logicBox (
	Gf (suggestion, "minx"), Gf (suggestion, "miny"), Gf (suggestion, "minz"),
	Gf (suggestion, "maxx"), Gf (suggestion, "maxy"), Gf (suggestion, "maxz"));
    fact = dynworld->AddLogicFactory (newname, 1.0f, 1.0f, logicBox);
  }
  else if (suggestion.Contains ("light"))
  {
    fact = dynworld->AddLightFactory (newname, 1.0f);
  }
  else
  {
    fact = dynworld->AddFactory (newname, 1.0f, 1.0f);
  }

  fact->SetAttribute ("category", categoryValue->GetStringValue ());
  aresed3d->RefreshFactorySettings (fact);
  aresed3d->GetApp ()->RegisterModification (fact->QueryObject ());

  categoryValue->Refresh ();
  FireValueChanged ();
  return View::FindChild (categoryValue, newname);
}

Value* DynfactCollectionValue::GetCategoryForValue (Value* value)
{
  for (size_t i = 0 ; i < children.GetSize () ; i++)
    if (children[i] == value) return value;
    else if (children[i]->IsChild (value)) return children[i];
  return 0;
}

