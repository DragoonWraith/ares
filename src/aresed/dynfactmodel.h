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

#ifndef __aresed_dynfactmodel_h
#define __aresed_dynfactmodel_h

#include "ui/rowmodel.h"
#include "tools/tools.h"

class AresEdit3DView;

class DynfactRowModel : public RowModel
{
private:
  AresEdit3DView* aresed3d;
  csHash<csStringArray,csString>::ConstGlobalIterator it;
  csString category;
  csStringArray items;
  size_t idx;

  void SearchNext ()
  {
    if (idx >= items.GetSize ())
    {
      if (it.HasNext ())
      {
	items = it.Next (category);
	idx = 0;
      }
    }
  }

public:
  DynfactRowModel (AresEdit3DView* aresed3d) : aresed3d (aresed3d) { }
  virtual ~DynfactRowModel () { }

  virtual void ResetIterator ()
  {
    const csHash<csStringArray,csString>& categories = aresed3d->GetCategories ();
    it = categories.GetIterator ();
    items.DeleteAll ();
    idx = 0;
    SearchNext ();
  }
  virtual bool HasRows () { return idx < items.GetSize (); }
  virtual csStringArray NextRow ()
  {
    csString cat = category;
    csString item = items[idx];
    SearchNext ();
    return Tools::MakeArray (cat.GetData (), item.GetData (), (const char*)0);
  }

  virtual void StartUpdate ()
  {
  }
  virtual bool UpdateRow (const csStringArray& row)
  {
#if 0
    iCelPlLayer* pl = entPanel->GetPL ();
    csString name = row[0];
    iCelEntityTemplate* t = pl->FindEntityTemplate (name);
    if (!t)
    {
      entPanel->GetUIManager ()->Error ("Can't find entity template '%s'!", name.GetData ());
      return false;
    }
    tpl->AddParent (t);
#endif
    return true;
  }

  virtual const char* GetColumns () { return "Category,Item"; }
  virtual bool IsEditAllowed () const { return false; }
};


#endif // __aresed_dynfactmodel_h
