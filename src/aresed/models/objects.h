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

#ifndef __aresed_objects_h
#define __aresed_objects_h

#include "edcommon/model.h"

class AppAresEditWX;
struct iMeshFactoryList;
struct iDynamicObject;

#if 0
class DynObjValue : public Ares::AbstractCompositeValue
{
public:
  iDynamicObject* dynobj;

protected:
  virtual size_t GetChildCount () { return 7; }
  virtual const char* GetName (size_t idx)
  {
    switch (idx)
    {
      case DYNOBJ_COL_ID: return "ID";
      case DYNOBJ_COL_ENTITY: return "Entity";
      case DYNOBJ_COL_FACTORY: return "Factory";
      case DYNOBJ_COL_X: return "X";
      case DYNOBJ_COL_Y: return "Y";
      case DYNOBJ_COL_Z: return "Z";
      case DYNOBJ_COL_DISTANCE: return "Distance";
    }
    return "?";
  }

public:
  DynObjValue (iDynamicObject* dynobj) : dynobj (dynobj)
  {
  };
  virtual ~DynObjValue () { }

  virtual Value* GetChild (size_t idx)
  {
    return children[idx];
  }
};
#endif

class ObjectsValue : public Ares::StandardCollectionValue
{
private:
  AppAresEditWX* app;

  csArray<iDynamicObject*> dynobjs;

protected:
  virtual void UpdateChildren ();
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  ObjectsValue (AppAresEditWX* app) : app (app) { }
  virtual ~ObjectsValue () { }

  size_t FindDynObj (iDynamicObject* obj) { return dynobjs.Find (obj); }
};

#endif // __aresed_objects_h

