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
#include "resourcemover.h"
#include "uimanager.h"
#include "edcommon/listctrltools.h"
#include "edcommon/uitools.h"
#include "edcommon/inspect.h"

#include "celtool/stdparams.h"
#include "physicallayer/entitytpl.h"
#include "tools/questmanager.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ResourceMoverDialog, wxDialog)
  EVT_BUTTON (XRCID("ok_Button"), ResourceMoverDialog :: OnOkButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

class RemoveUnusedAction : public Action
{
private:
  i3DView* view3d;
  ResourceMoverDialog* dialog;

public:
  RemoveUnusedAction (i3DView* view3d, ResourceMoverDialog* dialog) :
    view3d (view3d),
    dialog (dialog) { }
  virtual ~RemoveUnusedAction () { }
  virtual const char* GetName () const { return "Remove Unused Resources..."; }
  virtual bool Do (View* view, wxWindow* component)
  {
    iCelPlLayer* pl = view3d->GetPL ();
    iEngine* engine = view3d->GetEngine ();
    iPcDynamicWorld* dynworld = view3d->GetDynamicWorld ();
    iObjectRegistry* object_reg = view3d->GetApplication ()->GetObjectRegistry ();
    csRef<iQuestManager> questMgr = csQueryRegistry<iQuestManager> (object_reg);
    iAssetManager* assetMgr = view3d->GetApplication ()->GetAssetManager ();

    ResourceCounter counter (object_reg, dynworld);

    counter.CountResources ();

    csSet<csPtrKey<iObject> > resources;

    for (size_t i = 0 ; i < dynworld->GetFactoryCount () ; i++)
    {
      iObject* resource = dynworld->GetFactory (i)->QueryObject ();
      if (dynworld->GetFactory (i)->GetObjectCount () == 0 && assetMgr->IsModifiable (resource)
	  && !assetMgr->IsLocked (resource))
        resources.Add (resource);
    }

    csRef<iCelEntityTemplateIterator> tplIt = pl->GetEntityTemplates ();
    while (tplIt->HasNext ())
    {
      iObject* resource = tplIt->Next ()->QueryObject ();
      if (counter.GetTemplateCounter ().Get (resource->GetName (), 0) == 0 &&
	  assetMgr->IsModifiable (resource) && !assetMgr->IsLocked (resource))
        resources.Add (resource);
    }

    csRef<iQuestFactoryIterator> qIt = questMgr->GetQuestFactories ();
    while (qIt->HasNext ())
    {
      iObject* resource = qIt->Next ()->QueryObject ();
      if (counter.GetQuestCounter ().Get (resource->GetName (), 0) == 0 &&
	  assetMgr->IsModifiable (resource) && !assetMgr->IsLocked (resource))
        resources.Add (resource);
    }

    iLightFactoryList* lf = engine->GetLightFactories ();
    for (size_t i = 0 ; i < (size_t)lf->GetCount () ; i++)
    {
      iObject* resource = lf->Get (i)->QueryObject ();
      if (counter.GetLightCounter ().Get (resource->GetName (), 0) == 0 &&
	  assetMgr->IsModifiable (resource) && !assetMgr->IsLocked (resource))
        resources.Add (resource);
    }


    if (!view3d->GetApplication ()->GetUI ()->Ask ("Are you sure you want to remove %d unused resources (from writable assets)?", (int)resources.GetSize ()))
      return true;

    csSet<csPtrKey<iObject> >::GlobalIterator it = resources.GetIterator ();
    while (it.HasNext ())
    {
      iObject* resource = it.Next ();
      assetMgr->RegisterRemoval (resource);
      engine->RemoveObject (resource);
    }
    dialog->GetResourcesValue ()->Refresh ();
    return true;
  }

  virtual bool IsActive (View* view, wxWindow* component)
  {
    return true;
  }
};

//--------------------------------------------------------------------------

class ShowUsagesAction : public Action
{
private:
  i3DView* view3d;

public:
  ShowUsagesAction (i3DView* view3d) : view3d (view3d) { }
  virtual ~ShowUsagesAction () { }
  virtual const char* GetName () const { return "Show Usages"; }
  virtual bool Do (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () != 1) return true;

    ResourceCounter counter (view3d->GetApplication ()->GetObjectRegistry (),
	view3d->GetDynamicWorld ());

    iModelRepository* repository = view3d->GetModelRepository ();
    iObject* resource = repository->GetResourceFromResources (values[0]);
    counter.SetFilter (resource);
    csReport (view3d->GetApplication ()->GetObjectRegistry (), CS_REPORTER_SEVERITY_NOTIFY,
	"ares.usage", "Usages for object '%s'", resource->GetName ());
    counter.CountResources ();
    return true;
  }
  virtual bool IsActive (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () != 1) return false;
    return true;
  }
};


//--------------------------------------------------------------------------

class MoveToAssetAction : public Action
{
private:
  i3DView* view3d;

public:
  MoveToAssetAction (i3DView* view3d) : view3d (view3d) { }
  virtual ~MoveToAssetAction () { }
  virtual const char* GetName () const { return "Move to asset..."; }
  virtual bool Do (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () == 0) return true;

    iModelRepository* repository = view3d->GetModelRepository ();

    csString title;
    if (values.GetSize () == 1)
      title = "Select an asset for this resource";
    else
      title = "Select an asset for these resources";
    csRef<iUIDialog> dialog = view3d->GetApplication ()->GetUI ()->CreateDialog (title, 500);
    dialog->AddRow ();
    csRef<Ares::Value> assets = repository->GetAssetsValue ();
    dialog->AddListIndexed ("asset", assets, ASSET_COL_FILE, false, 300, "Writable,Path,File,Mount",
	ASSET_COL_WRITABLE, ASSET_COL_PATH, ASSET_COL_FILE, ASSET_COL_MOUNT);
    if (dialog->Show (0))
    {
      const DialogValues& result = dialog->GetFieldValues ();
      Ares::Value* row = result.Get ("asset", (Ares::Value*)0);
      iAsset* asset = 0;
      if (row)
	asset = repository->GetAssetFromAssets (row);
      iAssetManager* assetManager = view3d->GetApplication ()->GetAssetManager ();
      for (size_t i = 0 ; i < values.GetSize () ; i++)
      {
	iObject* resource = repository->GetResourceFromResources (values[i]);
        assetManager->PlaceResource (resource, asset);
      }
      Value* list = view->GetValue (component);
      FilteredCollectionValue* filteredList = static_cast<FilteredCollectionValue*> (list);
      filteredList->GetCollection ()->Refresh ();
    }

    return true;
  }
  virtual bool IsActive (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () == 0) return false;
    return true;
  }
};

//--------------------------------------------------------------------------

class LockAction : public Action
{
private:
  i3DView* view3d;
  ResourceMoverDialog* dialog;

public:
  LockAction (i3DView* view3d, ResourceMoverDialog* dialog) : view3d (view3d), dialog (dialog) { }
  virtual ~LockAction () { }
  virtual const char* GetName () const { return "Lock Resources"; }
  virtual bool Do (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () == 0) return true;

    iModelRepository* repository = view3d->GetModelRepository ();
    iAssetManager* assetManager = view3d->GetApplication ()->GetAssetManager ();
    for (size_t i = 0 ; i < values.GetSize () ; i++)
    {
      iObject* resource = repository->GetResourceFromResources (values[i]);
      assetManager->Lock (resource);
    }

    dialog->GetResourcesValue ()->Refresh ();

    return true;
  }
  virtual bool IsActive (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () == 0) return false;
    return true;
  }
};

//--------------------------------------------------------------------------

class UnlockAction : public Action
{
private:
  i3DView* view3d;
  ResourceMoverDialog* dialog;

public:
  UnlockAction (i3DView* view3d, ResourceMoverDialog* dialog) : view3d (view3d), dialog (dialog) { }
  virtual ~UnlockAction () { }
  virtual const char* GetName () const { return "Unlock Resources"; }
  virtual bool Do (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () == 0) return true;

    iModelRepository* repository = view3d->GetModelRepository ();
    iAssetManager* assetManager = view3d->GetApplication ()->GetAssetManager ();
    for (size_t i = 0 ; i < values.GetSize () ; i++)
    {
      iObject* resource = repository->GetResourceFromResources (values[i]);
      assetManager->Unlock (resource);
    }

    dialog->GetResourcesValue ()->Refresh ();

    return true;
  }
  virtual bool IsActive (View* view, wxWindow* component)
  {
    csArray<Value*> values = view->GetSelectedValues (component);
    if (values.GetSize () == 0) return false;
    return true;
  }
};

//--------------------------------------------------------------------------


class ResourceFilterValue : public FilteredCollectionValue
{
private:
  i3DView* view3d;
  csRef<ListSelectedValue> selection;

protected:
  virtual bool Filter (Value* child)
  {
    Value* selected = selection->GetMirrorValue ();
    iAsset* asset = 0;
    if (selected)
      asset = view3d->GetModelRepository ()->GetAssetFromAssets (selected);

    if (!asset) return true;
    iObject* resource = view3d->GetModelRepository ()->GetResourceFromResources (child);
    return asset->GetCollection ()->IsParentOf (resource);
  }

public:
  ResourceFilterValue (i3DView* view3d, ListSelectedValue* selection)
    : view3d (view3d), selection (selection)
  {
  }
  virtual ~ResourceFilterValue () { }
};

//--------------------------------------------------------------------------

void ResourceMoverDialog::OnOkButton (wxCommandEvent& event)
{
  wxListCtrl* assetList = XRCCTRL (*this, "asset_List", wxListCtrl);
  wxListCtrl* resourceList = XRCCTRL (*this, "resource_List", wxListCtrl);
  RemoveBinding (assetList);
  RemoveBinding (resourceList);

  EndModal (TRUE);
}

void ResourceMoverDialog::Show ()
{
  wxListCtrl* assetList = XRCCTRL (*this, "asset_List", wxListCtrl);
  csRef<Value> assetsValue = uiManager->GetApp ()->Get3DView ()->GetModelRepository ()->GetAssetsValue ();
  Bind (assetsValue, "asset_List");

  csRef<ListSelectedValue> selValue;
  selValue.AttachNew (new ListSelectedValue (assetList, assetsValue, VALUE_STRINGARRAY));

  csRef<Value> rv = uiManager->GetApp ()->Get3DView ()->GetModelRepository ()->GetResourcesValue ();
  resourcesValue = rv;

  csRef<ResourceFilterValue> resourceFilterValue;
  resourceFilterValue.AttachNew (new ResourceFilterValue (uiManager->GetApp ()->Get3DView (),
	selValue));
  resourceFilterValue->SetCollection (resourcesValue);

  Signal (selValue, resourceFilterValue);

  Bind (resourceFilterValue, "resource_List");

  ShowModal ();
}

ResourceMoverDialog::ResourceMoverDialog (wxWindow* parent, UIManager* uiManager) :
  View (this),
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("ResourceMoverDialog"));

  DefineHeadingIndexed ("asset_List",
      "RW,Path,File,Mount",
      ASSET_COL_WRITABLE,
      ASSET_COL_PATH,
      ASSET_COL_FILE,
      ASSET_COL_MOUNT);
  DefineHeadingIndexed ("resource_List",
      "Name,Type,Usage,File,Path,Mount",
      RESOURCE_COL_NAME,
      RESOURCE_COL_TYPE,
      RESOURCE_COL_USAGE,
      RESOURCE_COL_ASSET_FILE,
      RESOURCE_COL_ASSET_PATH,
      RESOURCE_COL_ASSET_MOUNT);

  wxListCtrl* rl = XRCCTRL (*this, "resource_List", wxListCtrl);
  AddAction (rl, NEWREF (Action, new MoveToAssetAction (uiManager->GetApp ()->Get3DView ())));
  AddAction (rl, NEWREF (Action, new ShowUsagesAction (uiManager->GetApp ()->Get3DView ())));
  AddAction (rl, NEWREF (Action, new RemoveUnusedAction (uiManager->GetApp ()->Get3DView (), this)));
  AddAction (rl, NEWREF (Action, new LockAction (uiManager->GetApp ()->Get3DView (), this)));
  AddAction (rl, NEWREF (Action, new UnlockAction (uiManager->GetApp ()->Get3DView (), this)));
}

ResourceMoverDialog::~ResourceMoverDialog ()
{
}


