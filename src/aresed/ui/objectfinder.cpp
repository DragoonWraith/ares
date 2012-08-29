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
#include "objectfinder.h"
#include "uimanager.h"
#include "edcommon/listctrltools.h"
#include "edcommon/uitools.h"

#include "celtool/stdparams.h"
#include "physicallayer/entitytpl.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ObjectFinderDialog, wxDialog)
  EVT_BUTTON (XRCID("ok_Button"), ObjectFinderDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancel_Button"), ObjectFinderDialog :: OnCancelButton)
  EVT_BUTTON (XRCID("searchTemplate_Button"), ObjectFinderDialog :: OnSearchTemplateButton)
  EVT_BUTTON (XRCID("searchFactory_Button"), ObjectFinderDialog :: OnSearchDynfactButton)
  EVT_BUTTON (XRCID("resetTemplate_Button"), ObjectFinderDialog :: OnResetTemplateButton)
  EVT_BUTTON (XRCID("resetFactory_Button"), ObjectFinderDialog :: OnResetDynfactButton)
  EVT_BUTTON (XRCID("resetRadius_Button"), ObjectFinderDialog :: OnResetRadiusButton)
  EVT_TEXT_ENTER (XRCID("radiusFilter_Text"), ObjectFinderDialog :: OnChangeRadius)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

static void CorrectName (csString& name)
{
  if (name[name.Length ()-1] == '*')
    name = name.Slice (0, name.Length ()-1);
}

class ObjectFinderFilteredCollectionValue : public FilteredCollectionValue
{
private:
  i3DView* view3d;
  csString templateName;
  csString factoryName;
  float maxradius;

protected:
  virtual bool Filter (Value* child)
  {
    iDynamicObject* dynobj = view3d->GetDynamicObjectFromObjects (child);
    if (!templateName.IsEmpty ())
    {
      iCelEntityTemplate* tpl = dynobj->GetEntityTemplate ();
      if (!tpl) return false;
      if (templateName != tpl->GetName ()) return false;
    }
    if (!factoryName.IsEmpty ())
    {
      if (factoryName != dynobj->GetFactory ()->GetName ()) return false;
    }
    if (maxradius >= 0.0f)
    {
      const csVector3& origin = view3d->GetCsCamera ()->GetTransform ().GetOrigin ();
      const csReversibleTransform& trans = dynobj->GetTransform ();
      float dist = sqrt (csSquaredDist::PointPoint (trans.GetOrigin (), origin));
      if (dist > maxradius) return false;
    }
    return true;
  }

public:
  ObjectFinderFilteredCollectionValue (i3DView* view3d) : FilteredCollectionValue (0),
    view3d (view3d)
  {
    maxradius = -1.0f;
  }
  virtual ~ObjectFinderFilteredCollectionValue () { }

  void SetFilterTemplate (const char* templateName)
  {
    ObjectFinderFilteredCollectionValue::templateName = templateName;
    FireValueChanged ();
  }
  void SetFilterFactory (const char* factoryName)
  {
    ObjectFinderFilteredCollectionValue::factoryName = factoryName;
    FireValueChanged ();
  }
  void SetFilterRadius (float maxradius)
  {
    ObjectFinderFilteredCollectionValue::maxradius = maxradius;
    FireValueChanged ();
  }
};

//--------------------------------------------------------------------------

void ObjectFinderDialog::OnChangeRadius (wxCommandEvent& event)
{
  float radius;
  csString radiusstr = UITools::GetValue (this, "radiusFilter_Text");
  csScanStr (radiusstr, "%f", &radius);
  filteredCollection->SetFilterRadius (radius);
}

void ObjectFinderDialog::OnResetTemplateButton (wxCommandEvent& event)
{
  UITools::SetValue (this, "templateFilter_Text", "");
  filteredCollection->SetFilterTemplate ("");
}

void ObjectFinderDialog::OnResetDynfactButton (wxCommandEvent& event)
{
  UITools::SetValue (this, "factoryFilter_Text", "");
  filteredCollection->SetFilterFactory ("");
}

void ObjectFinderDialog::OnResetRadiusButton (wxCommandEvent& event)
{
  UITools::SetValue (this, "radiusFilter_Text", "");
  filteredCollection->SetFilterRadius (-1.0f);
}

void ObjectFinderDialog::OnSearchTemplateButton (wxCommandEvent& event)
{
  csRef<iUIDialog> dialog = uiManager->CreateDialog (this, "Select a template");
  dialog->AddRow ();
  dialog->AddListIndexed ("template", uiManager->GetApp ()->Get3DView ()->GetTemplatesValue (),
      TEMPLATE_COL_NAME, 300, "Template", TEMPLATE_COL_NAME);

  if (dialog->Show (0) == 1)
  {
    const DialogResult& result = dialog->GetFieldContents ();
    csString name = result.Get ("template", (const char*)0);
    CorrectName (name);
    UITools::SetValue (this, "templateFilter_Text", name);
    filteredCollection->SetFilterTemplate (name);
  }
}

void ObjectFinderDialog::OnSearchDynfactButton (wxCommandEvent& event)
{
  csRef<iUIDialog> dialog = uiManager->CreateDialog (this, "Select a dynamic factory");
  dialog->AddRow ();
  dialog->AddListIndexed ("factory", uiManager->GetApp ()->Get3DView ()->GetFactoriesValue (),
      FACTORY_COL_NAME, 300, "Factory,Usage", FACTORY_COL_NAME, FACTORY_COL_USAGE);

  if (dialog->Show (0) == 1)
  {
    const DialogResult& result = dialog->GetFieldContents ();
    csString name = result.Get ("factory", (const char*)0);
    CorrectName (name);
    UITools::SetValue (this, "factoryFilter_Text", name);
    filteredCollection->SetFilterFactory (name);
  }
}

void ObjectFinderDialog::OnOkButton (wxCommandEvent& event)
{
  wxListCtrl* list = XRCCTRL (*this, "object_List", wxListCtrl);
  Value* value = GetSelectedValue (list);
  if (value)
  {
    i3DView* view3d = uiManager->GetApp ()->Get3DView ();
    iDynamicObject* dynobj = view3d->GetDynamicObjectFromObjects (value);
    view3d->GetSelection ()->SetCurrentObject (dynobj);
  }

  filteredCollection->SetCollection (0);
  EndModal (TRUE);
}

void ObjectFinderDialog::OnCancelButton (wxCommandEvent& event)
{
  filteredCollection->SetCollection (0);
  EndModal (TRUE);
}

void ObjectFinderDialog::Show ()
{
  Value* objectsValue = uiManager->GetApp ()->Get3DView ()->GetObjectsValue ();
  filteredCollection->SetCollection (objectsValue);
  ShowModal ();
}

ObjectFinderDialog::ObjectFinderDialog (wxWindow* parent, UIManager* uiManager) :
  View (this),
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("ObjectFinderDialog"));

  filteredCollection.AttachNew (new ObjectFinderFilteredCollectionValue (uiManager->GetApp ()->Get3DView ()));
  DefineHeadingIndexed ("object_List",
      "Factory,Template,Entity,ID,X,Y,Z,Distance",
      DYNOBJ_COL_FACTORY,
      DYNOBJ_COL_TEMPLATE,
      DYNOBJ_COL_ENTITY,
      DYNOBJ_COL_ID,
      DYNOBJ_COL_X,
      DYNOBJ_COL_Y,
      DYNOBJ_COL_Z,
      DYNOBJ_COL_DISTANCE);
  Bind (filteredCollection, "object_List");
}

ObjectFinderDialog::~ObjectFinderDialog ()
{
}


