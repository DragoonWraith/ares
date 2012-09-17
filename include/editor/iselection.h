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

#ifndef __iselection_h__
#define __iselection_h__

#include "csutil/scf.h"
#include <wx/wx.h>

struct iDynamicObject;

struct iSelectionIterator : public virtual iBase
{
  SCF_INTERFACE(iSelectionIterator,0,0,1);

  virtual bool HasNext () = 0;
  virtual iDynamicObject* Next () = 0;
};

/**
 * The current selection.
 */
struct iSelection : public virtual iBase
{
  SCF_INTERFACE(iSelection,0,0,1);

  /**
   * Return an iterator over all selected objects.
   */
  virtual csPtr<iSelectionIterator> GetIterator () = 0;

  /**
   * Get all objects in the selection.
   */
  virtual const csArray<iDynamicObject*>& GetObjects () const = 0;

  /**
   * Is something selected?
   */
  virtual bool HasSelection () const = 0;

  /**
   * Get the first selected object.
   */
  virtual iDynamicObject* GetFirst () const = 0;

  /**
   * Get the last selected object.
   */
  virtual iDynamicObject* GetLast () const = 0;

  /**
   * Get the number of selected objects.
   */
  virtual size_t GetSize () const = 0;

  /**
   * Set the selection to the given object.
   */
  virtual void SetCurrentObject (iDynamicObject* dynobj) = 0;

  /**
   * Add an object to the selection.
   */
  virtual void AddCurrentObject (iDynamicObject* dynobj) = 0;
};


#endif // __iselection_h__

