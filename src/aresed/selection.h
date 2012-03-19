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

#ifndef __aresed_selection_h
#define __aresed_selection_h

#include "editor/iselection.h"

class iDynamicObject;
class AresEdit3DView;

typedef csArray<iDynamicObject*>::Iterator SelectionIterator;

class SelectionIteratorImp : public scfImplementation1<SelectionIteratorImp,
  iSelectionIterator>
{
private:
  SelectionIterator it;

public:
  SelectionIteratorImp (const SelectionIterator& it) :
    scfImplementationType (this), it (it) { }
  virtual ~SelectionIteratorImp () { }

  virtual bool HasNext () { return it.HasNext (); }
  virtual iDynamicObject* Next () { return it.Next (); }
};


class SelectionListener : public csRefCount
{
public:
  virtual ~SelectionListener () { }
  virtual void SelectionChanged (const csArray<iDynamicObject*>& current_objects) = 0;
};

class Selection : public scfImplementation1<Selection, iSelection>
{
private:
  AresEdit3DView* aresed3d;

  csArray<iDynamicObject*> current_objects;
  csRefArray<SelectionListener> listeners;

  void FireSelectionListeners ();

public:
  Selection (AresEdit3DView* aresed3d);
  virtual ~Selection () { }

  SelectionIterator GetIteratorInt () { return current_objects.GetIterator (); }

  virtual csPtr<iSelectionIterator> GetIterator ()
  {
    iSelectionIterator* it = new SelectionIteratorImp (current_objects.GetIterator ());
    return it;
  }
  csArray<iDynamicObject*>& GetObjects () { return current_objects; }
  virtual const csArray<iDynamicObject*>& GetObjects () const { return current_objects; }
  virtual size_t GetSize () const { return current_objects.GetSize (); }
  virtual iDynamicObject* GetFirst () const { return current_objects[0]; }
  virtual bool HasSelection () const { return GetSize () > 0; }

  virtual void SetCurrentObject (iDynamicObject* dynobj);
  virtual void AddCurrentObject (iDynamicObject* dynobj);

  void AddSelectionListener (SelectionListener* listener)
  {
    listeners.Push (listener);
  }
};

#endif // __aresed_selection_h

