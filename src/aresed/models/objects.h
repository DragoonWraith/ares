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

class DynobjValue : public Ares::StringArrayValue
{
private:
  iDynamicObject* dynobj;

public:
  DynobjValue (iDynamicObject* dynobj) : dynobj (dynobj) { }
  virtual ~DynobjValue () { }

  iDynamicObject* GetDynamicObject () const { return dynobj; }
};

typedef csHash<DynobjValue*,csPtrKey<iDynamicObject> > ObjectsHash;

class ObjectsValue : public Ares::Value
{
private:
  AppAresEditWX* app;

  ObjectsHash dynobjs;
  csRefArray<DynobjValue> values;

  void ReleaseChildren ();

public:
  ObjectsValue (AppAresEditWX* app) : app (app) { }
  virtual ~ObjectsValue () { }

  virtual Ares::ValueType GetType () const { return Ares::VALUE_COLLECTION; }

  void BuildModel ();
  void RefreshModel ();

  virtual csPtr<Ares::ValueIterator> GetIterator ();
  virtual Value* GetChild (size_t idx) { return values[idx]; }
  size_t FindDynObj (iDynamicObject* dynobj) const;
};

#endif // __aresed_objects_h

