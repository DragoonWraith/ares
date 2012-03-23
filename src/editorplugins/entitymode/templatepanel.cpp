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
#include "edcommon/listview.h"
#include "edcommon/rowmodel.h"
#include "edcommon/tools.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"

#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "propclass/chars.h"

//--------------------------------------------------------------------------

class EntParRowModel : public RowModel
{
protected:
  EntityTemplatePanel* entPanel;
  iCelEntityTemplate* tpl;

public:
  EntParRowModel (EntityTemplatePanel* entPanel) : entPanel (entPanel), tpl (0) { }
  virtual ~EntParRowModel () { }

  void SetTemplate (iCelEntityTemplate* tpl)
  {
    EntParRowModel::tpl = tpl;
  }
};

using namespace Ares;

class ParentsCollectionValue : public StandardCollectionValue
{
private:
  EntityTemplatePanel* entPanel;
  iCelEntityTemplate* tpl;

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

  Value* NewChild (const char* name)
  {
    csRef<CompositeValue> composite = NEWREF(CompositeValue,new CompositeValue());
    composite->AddChild ("Template", NEWREF(StringValue,new StringValue(name)));
    children.Push (composite);
    composite->SetParent (this);
    return composite;
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
      NewChild (name);
    }
  }

public:
  ParentsCollectionValue (EntityTemplatePanel* entPanel) : entPanel (entPanel), tpl (0) { }
  virtual ~ParentsCollectionValue () { }

  void SetTemplate (iCelEntityTemplate* tpl)
  {
    ParentsCollectionValue::tpl = tpl;
    dirty = true;
  }

  virtual bool DeleteValue (Value* child)
  {
    if (!tpl) return false;
    Value* nameValue = child->GetChildByName ("Template");
    csString name = nameValue->GetStringValue ();
    iCelEntityTemplate* t = FindTemplate (name);
    if (!t) return false;
    tpl->RemoveParent (t);
    return true;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    if (!tpl) return 0;
    csString name = suggestion.Get ("Template", (const char*)0);
    iCelEntityTemplate* t = FindTemplate (name);
    if (!t) return 0;
    tpl->AddParent (t);
    Value* value = NewChild (name);
    FireValueChanged ();
    return value;
  }
};

#if 0
class ParentsRowModel : public EntParRowModel
{
private:
  csRef<iCelEntityTemplateIterator> it;

  iCelEntityTemplate* FindTemplate (const csStringArray& row)
  {
    iCelPlLayer* pl = entPanel->GetPL ();
    csString name = row[0];
    iCelEntityTemplate* t = pl->FindEntityTemplate (name);
    if (!t)
    {
      entPanel->GetUIManager ()->Error ("Can't find template '%s'!", name.GetData ());
      return 0;
    }
    return t;
  }

public:
  ParentsRowModel (EntityTemplatePanel* entPanel) : EntParRowModel (entPanel) { }
  virtual ~ParentsRowModel () { }

  virtual void ResetIterator ()
  {
    it = tpl->GetParents ();
  }
  virtual bool HasRows () { return it->HasNext (); }
  virtual csStringArray NextRow ()
  {
    iCelEntityTemplate* parent = it->Next ();
    return Tools::MakeArray (parent->GetName (), (const char*)0);
  }

  virtual bool DeleteRow (const csStringArray& row)
  {
    iCelEntityTemplate* t = FindTemplate (row);
    if (!t) return false;
    tpl->RemoveParent (t);
    return true;
  }
  virtual bool AddRow (const csStringArray& row)
  {
    iCelEntityTemplate* t = FindTemplate (row);
    if (!t) return false;
    tpl->AddParent (t);
    return true;
  }

  virtual const char* GetColumns () { return "Template"; }
  virtual bool IsEditAllowed () const { return false; }
};
#endif

//--------------------------------------------------------------------------

class CharacteristicsRowModel : public EntParRowModel
{
private:
  csRef<iCharacteristicsIterator> it;

public:
  CharacteristicsRowModel (EntityTemplatePanel* entPanel) : EntParRowModel (entPanel) { }
  virtual ~CharacteristicsRowModel () { }

  virtual void ResetIterator ()
  {
    it = tpl->GetCharacteristics ()->GetAllCharacteristics ();
  }
  virtual bool HasRows () { return it->HasNext (); }
  virtual csStringArray NextRow ()
  {
    float f;
    csString name = it->Next (f);
    csString value; value.Format ("%g", f);
    return Tools::MakeArray (name.GetData (), value.GetData (), (const char*)0);
  }

  virtual bool DeleteRow (const csStringArray& row)
  {
    iTemplateCharacteristics* chars = tpl->GetCharacteristics ();
    csString name = row[0];
    chars->ClearCharacteristic (name);
    return true;
  }
  virtual bool AddRow (const csStringArray& row)
  {
    iTemplateCharacteristics* chars = tpl->GetCharacteristics ();
    csString name = row[0];
    csString value = row[1];
    float f;
    csScanStr (value, "%f", &f);
    chars->SetCharacteristic (name, f);
    return true;
  }

  virtual const char* GetColumns () { return "Name,Value"; }
};

//--------------------------------------------------------------------------

class ClassesRowModel : public EntParRowModel
{
private:
  csSet<csStringID>::GlobalIterator it;

public:
  ClassesRowModel (EntityTemplatePanel* entPanel) : EntParRowModel (entPanel) { }
  virtual ~ClassesRowModel () { }

  virtual void ResetIterator ()
  {
    const csSet<csStringID>& classes = tpl->GetClasses ();
    it = classes.GetIterator ();
  }
  virtual bool HasRows () { return it.HasNext (); }
  virtual csStringArray NextRow ()
  {
    iCelPlLayer* pl = entPanel->GetPL ();
    csStringID classID = it.Next ();
    csString className = pl->FetchString (classID);
    return Tools::MakeArray (className.GetData (), (const char*)0);
  }

  virtual bool DeleteRow (const csStringArray& row)
  {
    iCelPlLayer* pl = entPanel->GetPL ();
    csString name = row[0];
    csStringID id = pl->FetchStringID (name);
    tpl->RemoveClass (id);
    return true;
  }
  virtual bool AddRow (const csStringArray& row)
  {
    iCelPlLayer* pl = entPanel->GetPL ();
    csString name = row[0];
    csStringID id = pl->FetchStringID (name);
    tpl->AddClass (id);
    return true;
  }

  virtual const char* GetColumns () { return "Class"; }
  virtual bool IsEditAllowed () const { return false; }
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
  //parentsModel.AttachNew (new ParentsRowModel (this));
  //parentsView = new ListCtrlView (list, parentsModel);
  dialog = uiManager->CreateDialog ("Add template");
  dialog->AddRow ();
  dialog->AddLabel ("Template:");
  dialog->AddText ("Template");
  //parentsView->SetEditorDialog (dialog);
  AddAction (list, NEWREF(Action, new NewChildDialogAction (parentsCollectionValue, dialog)));
  AddAction (list, NEWREF(Action, new DeleteChildAction (parentsCollectionValue)));

  list = XRCCTRL (*this, "templateCharacteristicsList", wxListCtrl);
  characteristicsModel.AttachNew (new CharacteristicsRowModel (this));
  characteristicsView = new ListCtrlView (list, characteristicsModel);
  dialog = uiManager->CreateDialog ("Add characteristic property");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("Name");
  dialog->AddRow ();
  dialog->AddLabel ("Value:");
  dialog->AddText ("Value");
  characteristicsView->SetEditorDialog (dialog);

  list = XRCCTRL (*this, "templateClassList", wxListCtrl);
  classesModel.AttachNew (new ClassesRowModel (this));
  classesView = new ListCtrlView (list, classesModel);
  dialog = uiManager->CreateDialog ("Add class");
  dialog->AddRow ();
  dialog->AddLabel ("Class:");
  dialog->AddText ("Class");
  classesView->SetEditorDialog (dialog);
}

EntityTemplatePanel::~EntityTemplatePanel ()
{
  //delete parentsView;
  delete characteristicsView;
  delete classesView;
}

// ----------------------------------------------------------------------

void EntityTemplatePanel::SwitchToTpl (iCelEntityTemplate* tpl)
{
  EntityTemplatePanel::tpl = tpl;

  parentsCollectionValue->SetTemplate (tpl);
  //parentsView->Refresh ();

  characteristicsModel->SetTemplate (tpl);
  characteristicsView->Refresh ();

  classesModel->SetTemplate (tpl);
  classesView->Refresh ();
}


