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

#include "apparesed.h"
#include "selection.h"

//---------------------------------------------------------------------------

Selection::Selection (AresEdit3DView* aresed3d) : aresed3d (aresed3d)
{
}

void Selection::FireSelectionListeners ()
{
  for (size_t i = 0 ; i < listeners.GetSize () ; i++)
    listeners[i]->SelectionChanged (current_objects);
}

void Selection::AddCurrentObject (iDynamicObject* dynobj)
{
  if (!dynobj) return;
  if (current_objects.Find (dynobj) != csArrayItemNotFound)
  {
    current_objects.Delete (dynobj);
    dynobj->SetHilight (false);
  }
  else
  {
    current_objects.Push (dynobj);
    dynobj->SetHilight (true);
  }
  FireSelectionListeners ();
}

void Selection::SetCurrentObject (iDynamicObject* dynobj)
{
  SelectionIterator it = current_objects.GetIterator ();
  while (it.HasNext ())
  {
    iDynamicObject* dynobj = it.Next ();
    dynobj->SetHilight (false);
  }
  current_objects.DeleteAll ();
  if (dynobj)
  {
    current_objects.Push (dynobj);
    dynobj->SetHilight (true);
  }
  FireSelectionListeners ();
}

