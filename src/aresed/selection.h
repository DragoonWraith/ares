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

typedef csArray<iDynamicObject*>::Iterator SelectionIterator;

class SelectionListener : public csRefCount
{
public:
  virtual ~SelectionListener () { }
  virtual void SelectionChanged (const csArray<iDynamicObject*>& current_objects) = 0;
};

class Selection
{
private:
  AresEdit3DView* aresed3d;

  csArray<iDynamicObject*> current_objects;
  csRefArray<SelectionListener> listeners;

  void FireSelectionListeners ();

public:
  Selection (AresEdit3DView* aresed3d);

  SelectionIterator GetIterator () { return current_objects.GetIterator (); }
  csArray<iDynamicObject*>& GetObjects () { return current_objects; }
  int GetSize () const { return current_objects.GetSize (); }
  iDynamicObject* GetFirst () const { return current_objects[0]; }
  bool HasSelection () const { return GetSize () > 0; }

  void SetCurrentObject (iDynamicObject* dynobj);
  void AddCurrentObject (iDynamicObject* dynobj);

  void AddSelectionListener (SelectionListener* listener)
  {
    listeners.Push (listener);
  }
};

#endif // __aresed_selection_h

