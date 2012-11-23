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

#include "../apparesed.h"
#include "../aresview.h"
#include "../models/helper.h"
#include "sanitychecker.h"
#include "uimanager.h"
#include "edcommon/listctrltools.h"
#include "edcommon/uitools.h"
#include "edcommon/sanity.h"

#include "celtool/stdparams.h"
#include "physicallayer/entitytpl.h"

class SanityCheckerValue : public GenericStringArrayCollectionValue<SanityResult>
{
private:
  SanityChecker* checker;

public:
  SanityCheckerValue (SanityChecker* checker) : GenericStringArrayCollectionValue<SanityResult> (0),
    checker (checker) { }
  virtual ~SanityCheckerValue () { }

  virtual void BuildModel ()
  {
    csArray<SanityResult> results = checker->GetResults ();

    objectsHash.DeleteAll ();
    ReleaseChildren ();

    for (size_t i = 0 ; i < results.GetSize () ; i++)
    {
      SanityResult& result = results.Get (i);
      csRef<GenericStringArrayValue<SanityResult> > child;
      child.AttachNew (new GenericStringArrayValue<SanityResult> (&result));
      csStringArray& array = child->GetArray ();
      array.Push (result.name);
      array.Push (result.message);
      csString idx;
      idx.Format ("%d", i);
      array.Push (idx);
      objectsHash.Put (&result, child);
      values.Push (child);
    }
    FireValueChanged ();
  }

  virtual void RefreshModel () { BuildModel (); }
};

SanityCheckerUI::SanityCheckerUI (UIManager* uiManager) : uiManager (uiManager)
{
}

class SanityCallback : public scfImplementation1<SanityCallback,iUIDialogCallback>
{
private:
  UIManager* uiManager;
  SanityChecker* checker;
  SanityCheckerValue* value;

public:
  SanityCallback (UIManager* uiManager, SanityChecker* checker,
      SanityCheckerValue* value) : scfImplementationType (this),
    uiManager (uiManager), checker (checker), value (value)
  {
  }
  virtual ~SanityCallback () { delete checker; }

  virtual void ButtonPressed (iUIDialog* dialog, const char* button)
  {
    csString b = button;
    if (b == "Refresh")
    {
      checker->ClearResults ();
      checker->CheckAll ();
      value->BuildModel ();
    }
    else if (b == "Fix")
    {
      const DialogValues& result = dialog->GetFieldValues ();
      Value* row = result.Get ("name", 0);
      if (row)
      {
        csString idxS = row->GetStringArrayValue ()->Get (2);
        int idx;
        csScanStr (idxS, "%d", &idx);
        const csArray<SanityResult>& results = checker->GetResults ();
        SanityResult result = results[idx];
	if (result.resource)
          uiManager->GetApp ()->OpenEditor (result.resource);
	if (result.object)
          uiManager->GetApp ()->OpenEditor (result.object);
      }
    }
  }

  SanityChecker* GetChecker () { return checker; }
};

void SanityCheckerUI::Show ()
{
  if (dialog)
  {
    dialog->Close ();
    dialog = 0;
  }
  SanityChecker* checker = new SanityChecker (uiManager->GetApp ()->GetObjectRegistry (),
      uiManager->GetApp ()->Get3DView ()->GetDynamicWorld ());
  checker->CheckAll ();

  csRef<SanityCheckerValue> value;
  value.AttachNew (new SanityCheckerValue (checker));

  csRef<SanityCallback> cb;
  cb.AttachNew (new SanityCallback (uiManager, checker, value));

  if (!dialog)
  {
    dialog = uiManager->CreateDialog ("Sanity checker", 900);
    dialog->AddRow ();
    dialog->AddListIndexed ("name", value, 0, false, 400, "Name,Message", 0, 1);
    dialog->AddRow ();
    dialog->AddButton ("Refresh");
    dialog->AddButton ("Fix");
    dialog->AddSpace ();
    dialog->AddButton ("Ok");
    dialog->DisableAutomaticOkCancel ();
  }

  dialog->ShowNonModal (cb);
}

