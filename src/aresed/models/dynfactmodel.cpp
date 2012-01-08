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
#include "meshfactmodel.h"
#include "../tools/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"

using namespace Ares;

void CategoryCollectionValue::UpdateChildren ()
{
  if (!dirty) return;
  dirty = false;
  ReleaseChildren ();
  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  const csStringArray& items = categories.Get (category, csStringArray ());
  for (size_t i = 0 ; i < items.GetSize () ; i++)
  {
    csRef<ConstantStringValue> strValue;
    strValue.AttachNew (new ConstantStringValue (items[i]));
    children.Push (strValue);
    strValue->SetParent (this);
  }
}

Value* CategoryCollectionValue::FindChild (const char* name)
{
  csString sName = name;
  for (size_t i = 0 ; i < children.GetSize () ; i++)
    if (sName == children[i]->GetStringValue ())
      return children[i];
  return 0;
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

Value* DynfactCollectionValue::NewValue (size_t idx, Value* selectedValue,
    const DialogResult& suggestion)
{
  csString newname = suggestion.Get ("name", (const char*)0);
  if (newname.IsEmpty ()) return 0;	// @@@ Error report.
  Value* categoryValue = GetCategoryForValue (selectedValue);
  if (!categoryValue) return 0;	// @@@ Error report.

  aresed3d->AddItem (categoryValue->GetStringValue (), newname);
  csRef<ConstantStringValue> strValue;
  strValue.AttachNew (new ConstantStringValue (newname));

  iPcDynamicWorld* dynworld = aresed3d->GetDynamicWorld ();
  dynworld->AddFactory (newname, 1.0f, 1.0f);

  CategoryCollectionValue* categoryCollectionValue = static_cast<CategoryCollectionValue*> (categoryValue);
  categoryCollectionValue->Refresh ();
  FireValueChanged ();
  return categoryCollectionValue->FindChild (newname);
}

Value* DynfactCollectionValue::GetCategoryForValue (Value* value)
{
  for (size_t i = 0 ; i < children.GetSize () ; i++)
    if (children[i] == value) return value;
    else if (children[i]->IsChild (value)) return children[i];
  return 0;
}

//--------------------------------------------------------------------------

void DynfactRowModel::SearchNext ()
{
  while (idx >= items.GetSize ())
  {
    if (it.HasNext ())
    {
      items = it.Next (category);
      idx = 0;
    }
    else return;
  }
}

void DynfactRowModel::ResetIterator ()
{
  const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
  it = categories.GetIterator ();
  items.DeleteAll ();
  idx = 0;
  SearchNext ();
}

csStringArray DynfactRowModel::NextRow ()
{
  csString cat = category;
  csString item = items[idx];
  idx++;
  SearchNext ();
  return Tools::MakeArray (cat.GetData (), item.GetData (), (const char*)0);
}

csStringArray DynfactRowModel::EditRow (const csStringArray& origRow)
{
  UIManager* ui = aresed3d->GetApp ()->GetUIManager ();
  UIDialog* dialog = ui->CreateDialog ("New dynamic object");
  dialog->AddRow ();
  dialog->AddLabel ("Category:");
  dialog->AddText ("Category");
  dialog->AddRow ();
  //dialog->AddLabel ("Item:");
  //dialog->AddText ("Item");
  csRef<MeshfactRowModel> model = new MeshfactRowModel (aresed3d->GetEngine ());
  dialog->AddList ("Item", model, 0);
  if (origRow.GetSize () >= 1)
    dialog->SetValue ("Category", origRow[0]);
  if (origRow.GetSize () >= 2)
    dialog->SetValue ("Item", origRow[1]);
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    csString category = fields.Get ("Category", "");
    csString item = fields.Get ("Item", "");
    if (item.IsEmpty ())
    {
      ui->Error ("Please select a mesh factory!");
      delete dialog;
      return csStringArray ();
    }
    return Tools::MakeArray (category.GetData (), item.GetData (), (const char*)0);
  }
  delete dialog;
  return csStringArray ();
}

bool DynfactRowModel::DeleteRow (const csStringArray& row)
{
  aresed3d->RemoveItem (row[0], row[1]);
  return true;
}

bool DynfactRowModel::AddRow (const csStringArray& row)
{
  aresed3d->AddItem (row[0], row[1]);
  return true;
}

