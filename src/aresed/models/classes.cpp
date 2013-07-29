/*
The MIT License

Copyright (c) 2013 by Jorrit Tyberghein

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

#include "classes.h"
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

using namespace Ares;

static int CompareClassValues (
    GenericStringArrayValue<const char>* const & v1,
    GenericStringArrayValue<const char>* const & v2)
{
  const char* s1 = v1->GetStringArrayValue ()->Get (CLASS_COL_NAME);
  const char* s2 = v2->GetStringArrayValue ()->Get (CLASS_COL_NAME);
  return strcmp (s1, s2);
}

void ClassesValue::AddClass (const char* cls, const char* description)
{
  csRef<GenericStringArrayValue<const char> > child;
  child.AttachNew (new GenericStringArrayValue<const char> (cls));
  csStringArray& array = child->GetArray ();
  array.Push (cls);
  array.Push (description);
  objectsHash.Put (cls, child);
  values.Push (child);
}

void ClassesValue::BuildModel ()
{
  objectsHash.DeleteAll ();
  ReleaseChildren ();

  AddClass ("ares.note", "Gamecontroller shows the 'note' icon on hover");
  AddClass ("ares.info", "Gamecontroller shows the 'info' icon on hover and shows information when requested");
  AddClass ("ares.pickup", "Gamecontroller allows picking up of this item");
  AddClass ("ares.noactivate", "Gamecontroller does not allow activation of this item");

  values.Sort (CompareClassValues);
  FireValueChanged ();
}

void ClassesValue::RefreshModel ()
{
  BuildModel ();
}


