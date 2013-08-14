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

#include "wizards.h"
#include "edcommon/tools.h"
#include "../ui/uimanager.h"
#include "../apparesed.h"
#include "../aresview.h"

using namespace Ares;

static int CompareWizardValue (
    GenericStringArrayValue<Wizard>* const & v1,
    GenericStringArrayValue<Wizard>* const & v2)
{
  const char* s1 = v1->GetStringArrayValue ()->Get (WIZARD_COL_NAME);
  const char* s2 = v2->GetStringArrayValue ()->Get (WIZARD_COL_NAME);
  return strcmp (s1, s2);
}

void WizardsValue::AddWizard (Wizard* wizard)
{
  csRef<GenericStringArrayValue<Wizard> > child;
  child.AttachNew (new GenericStringArrayValue<Wizard> (wizard));
  csStringArray& array = child->GetArray ();
  array.Push (wizard->name);
  array.Push (wizard->description);
  objectsHash.Put (wizard, child);
  values.Push (child);
}

void WizardsValue::BuildModel ()
{
  objectsHash.DeleteAll ();
  ReleaseChildren ();

  iEditorConfig* config = app->GetConfig ();
  const csPDelArray<Wizard>& wizards = type == "template" ?
    config->GetTemplateWizards ()
    :
    config->GetQuestWizards ();
  for (size_t i = 0 ; i < wizards.GetSize () ; i++)
  {
    AddWizard (wizards.Get (i));
  }

  values.Sort (CompareWizardValue);
  FireValueChanged ();
}

void WizardsValue::RefreshModel ()
{
  BuildModel ();
}


