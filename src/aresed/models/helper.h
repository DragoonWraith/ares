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

#ifndef __aresed_helper_h
#define __aresed_helper_h

class AppAresEditWX;

#include "edcommon/model.h"

template <class T>
class GenericStringArrayValue : public Ares::StringArrayValue
{
private:
  T* obj;

public:
  GenericStringArrayValue (T* obj) : obj (obj) { }
  virtual ~GenericStringArrayValue () { }

  T* GetObject () const { return obj; }
};

template <class T>
class GenericStringArrayValueIterator : public Ares::ValueIterator
{
private:
  csRefArray<GenericStringArrayValue<T> > children;
  size_t idx;

public:
  GenericStringArrayValueIterator (const csRefArray<GenericStringArrayValue<T> >& children) :
	children (children), idx (0) { }
  virtual ~GenericStringArrayValueIterator () { }
  virtual void Reset () { idx = 0; }
  virtual bool HasNext () { return idx < children.GetSize (); }
  virtual Ares::Value* NextChild (csString* name = 0)
  {
    idx++;
    return children[idx-1];
  }
};

template <class T>
class GenericStringArrayCollectionValue : public Ares::Value
{
protected:
  AppAresEditWX* app;

  typedef csHash<GenericStringArrayValue<T>*,csPtrKey<T> > ObjectsHash;
  ObjectsHash objectsHash;
  csRefArray<GenericStringArrayValue<T> > values;

  void ReleaseChildren ()
  {
    typename ObjectsHash::GlobalIterator it = objectsHash.GetIterator ();
    while (it.HasNext ())
    {
      csPtrKey<T> obj;
      Ares::StringArrayValue* child = it.Next (obj);
      child->SetParent (0);
    }
    objectsHash.DeleteAll ();
    values.DeleteAll ();
  }

public:
  GenericStringArrayCollectionValue (AppAresEditWX* app) : app (app) { }
  virtual ~GenericStringArrayCollectionValue () { }

  virtual Ares::ValueType GetType () const { return Ares::VALUE_COLLECTION; }

  virtual void BuildModel () { }
  virtual void RefreshModel () { }

  virtual void Refresh () { BuildModel (); }
  virtual csPtr<Ares::ValueIterator> GetIterator ()
  {
    return new GenericStringArrayValueIterator<T> (values);
  }
  virtual Value* GetChild (size_t idx)
  {
    return values[idx];
  }
  size_t FindObject (T* obj) const
  {
    for (size_t i = 0 ; i < values.GetSize () ; i++)
      if (values[i]->GetObject () == obj)
        return i;
    return csArrayItemNotFound;
  }
  T* GetObjectFromValue (Ares::Value* value)
  {
    GenericStringArrayValue<T>* v = static_cast<GenericStringArrayValue<T>*> (value);
    return v->GetObject ();
  }
};

#endif // __aresed_helper_h

