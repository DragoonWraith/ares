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

#include "templatepanel.h"
#include "entitymode.h"
#include "edcommon/listctrltools.h"
#include "edcommon/tools.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/iapp.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "propclass/chars.h"

//--------------------------------------------------------------------------

using namespace Ares;

class AbstractTplCollectionValue : public StandardCollectionValue
{
protected:
  EntityTemplatePanel* entPanel;
  iCelEntityTemplate* tpl;

public:
  AbstractTplCollectionValue (EntityTemplatePanel* entPanel) : entPanel (entPanel), tpl (0) { }
  virtual ~AbstractTplCollectionValue () { }

  void SetTemplate (iCelEntityTemplate* tpl)
  {
    AbstractTplCollectionValue::tpl = tpl;
    dirty = true;
  }
};

//--------------------------------------------------------------------------

class ParentsCollectionValue : public AbstractTplCollectionValue
{
private:
  iCelEntityTemplate* FindTemplate (const char* name)
  {
    iCelPlLayer* pl = entPanel->GetPL ();
    iCelEntityTemplate* t = pl->FindEntityTemplate (name);
    if (!t)
    {
      entPanel->GetUIManager ()->Error ("Can't find template '%s'!", name);
      return 0;
    }
    return t;
  }

protected:
  virtual void UpdateChildren ()
  {
    if (!tpl) return;
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    csRef<iCelEntityTemplateIterator> it = tpl->GetParents ();
    while (it->HasNext ())
    {
      iCelEntityTemplate* parent = it->Next ();
      csString name = parent->GetName ();
      NewCompositeChild (VALUE_STRING, "Template", name.GetData (), VALUE_NONE);
    }
  }

public:
  ParentsCollectionValue (EntityTemplatePanel* entPanel) : AbstractTplCollectionValue (entPanel) { }
  virtual ~ParentsCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    if (!tpl) return false;
    Value* nameValue = child->GetChildByName ("Template");
    iCelEntityTemplate* t = FindTemplate (nameValue->GetStringValue ());
    if (!t) return false;
    tpl->RemoveParent (t);
    entPanel->GetEntityMode ()->RegisterModification (tpl);
    Refresh ();
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!tpl) return 0;
    csString name = suggestion.Get ("Template", (const char*)0);
    iCelEntityTemplate* t = FindTemplate (name);
    if (!t) return 0;
    tpl->AddParent (t);
    entPanel->GetEntityMode ()->RegisterModification (tpl);
    Value* value = NewCompositeChild (VALUE_STRING, "Template", name.GetData (), VALUE_NONE);
    FireValueChanged ();
    return value;
  }
};

//--------------------------------------------------------------------------

class CharacteristicsCollectionValue : public AbstractTplCollectionValue
{
private:
  Value* NewChild (const char* name, float f)
  {
    return NewCompositeChild (
	VALUE_STRING, "Name", name,
	VALUE_FLOAT, "Value", f,
	VALUE_NONE);
  }

protected:
  virtual void UpdateChildren ()
  {
    if (!tpl) return;
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    csRef<iCharacteristicsIterator> it = tpl->GetCharacteristics ()->GetAllCharacteristics ();
    while (it->HasNext ())
    {
      float f;
      csString name = it->Next (f);
      NewChild (name, f);
    }
  }

public:
  CharacteristicsCollectionValue (EntityTemplatePanel* entPanel) : AbstractTplCollectionValue (entPanel) { }
  virtual ~CharacteristicsCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    if (!tpl) return false;
    Value* nameValue = child->GetChildByName ("Name");
    iTemplateCharacteristics* chars = tpl->GetCharacteristics ();
    chars->ClearCharacteristic (nameValue->GetStringValue ());
    entPanel->GetEntityMode ()->RegisterModification (tpl);
    Refresh ();
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!tpl) return 0;
    csString name = suggestion.Get ("Name", (const char*)0);
    csString value = suggestion.Get ("Value", (const char*)0);
    iTemplateCharacteristics* chars = tpl->GetCharacteristics ();
    float f;
    csScanStr (value, "%f", &f);
    chars->SetCharacteristic (name, f);
    entPanel->GetEntityMode ()->RegisterModification (tpl);
    Value* child = NewChild (name, f);
    FireValueChanged ();
    return child;
  }
  virtual bool UpdateValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!tpl) return false;
    csString name = suggestion.Get ("Name", (const char*)0);
    csString value = suggestion.Get ("Value", (const char*)0);
    float f;
    csScanStr (value, "%f", &f);
    iTemplateCharacteristics* chars = tpl->GetCharacteristics ();
    chars->SetCharacteristic (name, f);
    entPanel->GetEntityMode ()->RegisterModification (tpl);
    selectedValue->GetChildByName ("Name")->SetStringValue (name);
    selectedValue->GetChildByName ("Value")->SetFloatValue (f);
    FireValueChanged ();
    return true;
  }
};

//--------------------------------------------------------------------------

class ClassesCollectionValue : public AbstractTplCollectionValue
{
protected:
  virtual void UpdateChildren ()
  {
    if (!tpl) return;
    if (!dirty) return;
    dirty = false;
    ReleaseChildren ();
    iCelPlLayer* pl = entPanel->GetPL ();
    const csSet<csStringID>& classes = tpl->GetClasses ();
    csSet<csStringID>::GlobalIterator it = classes.GetIterator ();
    while (it.HasNext ())
    {
      csStringID classID = it.Next ();
      csString className = pl->FetchString (classID);
      NewCompositeChild (VALUE_STRING, "Class", className.GetData (), VALUE_NONE);
    }
  }

public:
  ClassesCollectionValue (EntityTemplatePanel* entPanel) : AbstractTplCollectionValue (entPanel) { }
  virtual ~ClassesCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    if (!tpl) return false;
    iCelPlLayer* pl = entPanel->GetPL ();
    Value* nameValue = child->GetChildByName ("Class");
    csStringID id = pl->FetchStringID (nameValue->GetStringValue ());
    tpl->RemoveClass (id);
    entPanel->GetEntityMode ()->RegisterModification (tpl);
    Refresh ();
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!tpl) return 0;
    iCelPlLayer* pl = entPanel->GetPL ();
    csString name = suggestion.Get ("Class", (const char*)0);
    csStringID id = pl->FetchStringID (name);
    tpl->AddClass (id);
    entPanel->GetEntityMode ()->RegisterModification (tpl);
    Value* value = NewCompositeChild (VALUE_STRING, "Class", name.GetData (), VALUE_NONE);
    FireValueChanged ();
    return value;
  }
};


//--------------------------------------------------------------------------

EntityTemplatePanel::EntityTemplatePanel (wxWindow* parent, iUIManager* uiManager,
    EntityMode* emode) :
  View (this), uiManager (uiManager), emode (emode), tpl (0)
{
  pl = emode->GetPL ();
  parentSizer = parent->GetSizer (); 
  parentSizer->Add (this, 0, wxALL | wxEXPAND);
  wxXmlResource::Get()->LoadPanel (this, parent, wxT ("EntityTemplatePanel"));

  wxListCtrl* list;
  csRef<iUIDialog> dialog;

  DefineHeading ("templateParentsList", "Template", "Template");
  parentsCollectionValue.AttachNew (new ParentsCollectionValue (this));
  Bind (parentsCollectionValue, "templateParentsList");
  list = XRCCTRL (*this, "templateParentsList", wxListCtrl);
  dialog = uiManager->CreateDialog ("Add template");
  dialog->AddRow ();
  dialog->AddLabel ("Template:");
  dialog->AddText ("Template");
  AddAction (list, NEWREF(Action, new NewChildDialogAction (parentsCollectionValue, dialog)));
  AddAction (list, NEWREF(Action, new DeleteChildAction (parentsCollectionValue)));

  DefineHeading ("templateCharacteristicsList", "Name,Value", "Name,Value");
  characteristicsCollectionValue.AttachNew (new CharacteristicsCollectionValue (this));
  Bind (characteristicsCollectionValue, "templateCharacteristicsList");
  list = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  dialog = uiManager->CreateDialog ("Add characteristic property");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("Name");
  dialog->AddRow ();
  dialog->AddLabel ("Value:");
  dialog->AddText ("Value");
  AddAction (list, NEWREF(Action, new NewChildDialogAction (characteristicsCollectionValue, dialog)));
  AddAction (list, NEWREF(Action, new EditChildDialogAction (characteristicsCollectionValue, dialog)));
  AddAction (list, NEWREF(Action, new DeleteChildAction (characteristicsCollectionValue)));

  DefineHeading ("templateClassList", "Class", "Class");
  classesCollectionValue.AttachNew (new ClassesCollectionValue (this));
  Bind (classesCollectionValue, "templateClassList");
  list = XRCCTRL (*this, "templateClassList", wxListCtrl);
  dialog = uiManager->CreateDialog ("Add class");
  dialog->AddRow ();
  dialog->AddLabel ("Class:");
  dialog->AddText ("Class");
  AddAction (list, NEWREF(Action, new NewChildDialogAction (classesCollectionValue, dialog)));
  AddAction (list, NEWREF(Action, new DeleteChildAction (classesCollectionValue)));
}

EntityTemplatePanel::~EntityTemplatePanel ()
{
}

// ----------------------------------------------------------------------

void EntityTemplatePanel::SwitchToTpl (iCelEntityTemplate* tpl)
{
  EntityTemplatePanel::tpl = tpl;

  parentsCollectionValue->SetTemplate (tpl);
  parentsCollectionValue->Refresh ();

  characteristicsCollectionValue->SetTemplate (tpl);
  characteristicsCollectionValue->Refresh ();

  classesCollectionValue->SetTemplate (tpl);
  classesCollectionValue->Refresh ();
}


