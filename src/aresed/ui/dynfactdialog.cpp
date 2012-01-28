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

#include "../apparesed.h"
#include "dynfactdialog.h"
#include "uimanager.h"
#include "listctrltools.h"
#include "uitools.h"
#include "meshview.h"
#include "treeview.h"
#include "listview.h"
#include "dirtyhelper.h"
#include "../models/dynfactmodel.h"
#include "../models/meshfactmodel.h"
#include "../tools/tools.h"

#include <wx/choicebk.h>

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(DynfactDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), DynfactDialog :: OnOkButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

DynfactMeshView::DynfactMeshView (DynfactDialog* dialog, iObjectRegistry* object_reg, wxWindow* parent) :
    MeshView (object_reg, parent), dialog (dialog)
{
  normalPen = CreatePen (0.5f, 0.0f, 0.0f, 0.5f);
  hilightPen = CreatePen (1.0f, 0.7f, 0.7f, 1.0f);
}

void DynfactMeshView::SyncValue (Ares::Value* value)
{
  iDynamicFactory* fact = dialog->GetCurrentFactory ();
  if (fact && GetMeshName () != fact->GetName ())
  {
    SetMesh (fact->GetName ());
  }
  SetupColliderGeometry ();
}

void DynfactMeshView::SetupColliderGeometry ()
{
  ClearGeometry ();
  long idx = dialog->GetSelectedCollider ();
  iDynamicFactory* fact = dialog->GetCurrentFactory ();
  if (fact)
  {
    for (size_t i = 0 ; i < fact->GetBodyCount () ; i++)
    {
      size_t pen = i == size_t (idx) ? hilightPen : normalPen;
      celBodyInfo info = fact->GetBody (i);
      if (info.type == BODY_BOX)
        AddBox (csBox3 (info.offset - info.size * .5, info.offset + info.size * .5), pen);
      else if (info.type == BODY_SPHERE)
	AddSphere (info.offset, info.radius, pen);
      else if (info.type == BODY_CYLINDER)
	AddCylinder (info.offset, info.radius, info.length, pen);
      else if (info.type == BODY_MESH || info.type == BODY_CONVEXMESH)
	AddMesh (info.offset, pen);
    }
  }
}

//--------------------------------------------------------------------------

using namespace Ares;

/**
 * A value for the type of a collider.
 */
class ColliderTypeValue : public Value
{
private:
  celBodyType* type;

public:
  ColliderTypeValue (celBodyType* t) : type (t) { }
  virtual ~ColliderTypeValue () { }

  virtual ValueType GetType () const { return VALUE_STRING; }
  virtual const char* GetStringValue ()
  {
    switch (*type)
    {
      case BODY_NONE: return "None";
      case BODY_BOX: return "Box";
      case BODY_SPHERE: return "Sphere";
      case BODY_CYLINDER: return "Cylinder";
      case BODY_CONVEXMESH: return "Convex mesh";
      case BODY_MESH: return "Mesh";
      default: return "?";
    }
  }
  virtual void SetStringValue (const char* str)
  {
    csString sstr = str;
    celBodyType newtype;
    if (sstr == "None") newtype = BODY_NONE;
    else if (sstr == "Box") newtype = BODY_BOX;
    else if (sstr == "Sphere") newtype = BODY_SPHERE;
    else if (sstr == "Cylinder") newtype = BODY_CYLINDER;
    else if (sstr == "Convex mesh") newtype = BODY_CONVEXMESH;
    else if (sstr == "Mesh") newtype = BODY_MESH;
    else newtype = BODY_NONE;
    if (newtype == *type) return;
    *type = newtype;
    FireValueChanged ();
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[ColType]";
    dump += Value::Dump (verbose);
    return dump;
  }
};

/**
 * A composite value representing a collider for a dynamic factory.
 */
class ColliderValue : public CompositeValue
{
private:
  size_t idx;
  iDynamicFactory* dynfact;
  celBodyInfo info;

protected:
  virtual void ChildChanged (Value* child)
  {
    switch (info.type)
    {
      case BODY_NONE:
	break;
      case BODY_BOX:
	dynfact->AddRigidBox (info.offset, info.size, info.mass, idx);
	break;
      case BODY_SPHERE:
	dynfact->AddRigidSphere (info.radius, info.offset, info.mass, idx);
	break;
      case BODY_CYLINDER:
	dynfact->AddRigidCylinder (info.radius, info.length, info.offset, info.mass, idx);
	break;
      case BODY_CONVEXMESH:
	dynfact->AddRigidConvexMesh (info.offset, info.mass, idx);
	break;
      case BODY_MESH:
	dynfact->AddRigidMesh (info.offset, info.mass, idx);
	break;
      default:
	printf ("Something is wrong: unknown collider type %d\n", info.type);
	break;
    }
    FireValueChanged ();
  }

public:
  ColliderValue (size_t idx, iDynamicFactory* dynfact) : idx (idx), dynfact (dynfact)
  {
    if (dynfact) info = dynfact->GetBody (idx);
    csRef<Value> v;
    v.AttachNew (new ColliderTypeValue (&ColliderValue::info.type)); AddChild ("type", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.offset.x)); AddChild ("offsetX", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.offset.y)); AddChild ("offsetY", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.offset.z)); AddChild ("offsetZ", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.mass)); AddChild ("mass", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.radius)); AddChild ("radius", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.length)); AddChild ("length", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.size.x)); AddChild ("sizeX", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.size.y)); AddChild ("sizeY", v);
    v.AttachNew (new FloatPointerValue (&ColliderValue::info.size.z)); AddChild ("sizeZ", v);
  }
  virtual ~ColliderValue () { }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Col]";
    dump += CompositeValue::Dump (verbose);
    return dump;
  }
};

/**
 * A value representing the list of colliders for a dynamic factory.
 * Children of this value are of type ColliderValue.
 */
class ColliderCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;

  // Create a new child and add to the array.
  Value* NewChild (size_t i)
  {
    csRef<ColliderValue> value;
    value.AttachNew (new ColliderValue (i, dynfact));
    children.Push (value);
    value->SetParent (this);
    return value;
  }

protected:
  virtual void UpdateChildren ()
  {
    if (dynfact != dialog->GetCurrentFactory ()) dirty = true;
    if (!dirty) return;
    ReleaseChildren ();
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return;
    dirty = false;
    for (size_t i = 0 ; i < dynfact->GetBodyCount () ; i++)
      NewChild (i);
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
    UIManager* uiManager = dialog->GetUIManager ();
    AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
    ares3d->SetupFactorySettings (dialog->GetCurrentFactory ());
  }

public:
  ColliderCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~ColliderCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return false;
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      if (children[i] == child)
      {
	dynfact->DeleteBody (i);
	child->SetParent (0);
	children.DeleteIndex (i);
	FireValueChanged ();
	return true;
      }
    return false;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return 0;
    dynfact->AddRigidBox (csVector3 (0, 0, 0), csVector3 (.2, .2, .2), 1.0f);
    idx = dynfact->GetBodyCount ()-1;
    Value* value = NewChild (idx);
    FireValueChanged ();
    return value;
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Col*]";
    dump += StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

//--------------------------------------------------------------------------

/// Value for the maximum radius of a dynamic factory.
class MaxRadiusValue : public FloatValue
{
private:
  DynfactDialog* dialog;
public:
  MaxRadiusValue (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~MaxRadiusValue () { }
  virtual void SetFloatValue (float f)
  {
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    if (dynfact) dynfact->SetMaximumRadiusRelative (f);
    FireValueChanged ();
  }
  virtual float GetFloatValue ()
  {
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    return dynfact ? dynfact->GetMaximumRadiusRelative () : 0.0f;
  }
};

/// Value for the imposter radius of a dynamic factory.
class ImposterRadiusValue : public FloatValue
{
private:
  DynfactDialog* dialog;
public:
  ImposterRadiusValue (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~ImposterRadiusValue () { }
  virtual void SetFloatValue (float f)
  {
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    if (dynfact) dynfact->SetImposterRadius (f);
    FireValueChanged ();
  }
  virtual float GetFloatValue ()
  {
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    return dynfact ? dynfact->GetImposterRadius () : 0.0f;
  }
};

//--------------------------------------------------------------------------

class RotMeshTimer : public scfImplementation1<RotMeshTimer, iTimerEvent>
{
private:
  DynfactDialog* df;

public:
  RotMeshTimer (DynfactDialog* df) : scfImplementationType (this), df (df) { }
  virtual ~RotMeshTimer () { }
  virtual bool Perform (iTimerEvent* ev) { df->Tick (); return true; }
};

//--------------------------------------------------------------------------

DynfactValue::DynfactValue (DynfactDialog* dialog) : dialog (dialog)
{
  // Setup the composite representing the dynamic factory that is selected.
  AddChild ("colliders", NEWREF(Value,new ColliderCollectionValue (dialog)));
  AddChild ("maxRadius", NEWREF(Value,new MaxRadiusValue(dialog)));
  AddChild ("imposterRadius", NEWREF(Value,new ImposterRadiusValue(dialog)));
}

//--------------------------------------------------------------------------

bool EditCategoryAction::Do (View* view, wxWindow* component)
{
  UIManager* uiManager = dialog->GetUIManager ();

  Value* value = view->GetSelectedValue (component);
  if (!value)
  {
    uiManager->Error ("Please select a valid item!");
    return false;
  }

  AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
  DynfactCollectionValue* dynfactCollectionValue = ares3d->GetDynfactCollectionValue ();
  Value* categoryValue = dynfactCollectionValue->GetCategoryForValue (value);
  if (!categoryValue || categoryValue == value)
  {
    uiManager->Error ("Please select a valid item!");
    return false;
  }

  UIDialog dia (dialog, "Edit category");
  dia.AddRow ();
  dia.AddLabel ("Category:");
  dia.AddText ("category");

  csString oldCategory = categoryValue->GetStringValue ();
  dia.SetText ("category", categoryValue->GetStringValue ());
  if (dia.Show (0))
  {
    const DialogResult& rc = dia.GetFieldContents ();
    csString newCategory = rc.Get ("category", oldCategory);
    if (newCategory.IsEmpty ())
    {
      uiManager->Error ("The category cannot be empty!");
      return false;
    }
    if (newCategory != oldCategory)
    {
      csString itemname = value->GetStringValue ();
      ares3d->ChangeCategory (newCategory, itemname);
      iPcDynamicWorld* dynworld = ares3d->GetDynamicWorld ();
      iDynamicFactory* fact = dynworld->FindFactory (itemname);
      CS_ASSERT (fact != 0);
      fact->SetAttribute ("category", newCategory);
      dynfactCollectionValue->Refresh ();
      Value* itemValue = dynfactCollectionValue->FindValueForItem (itemname);
      if (itemValue)
	view->SetSelectedValue (component, itemValue);
    }
  }

  return true;
}

//--------------------------------------------------------------------------

/**
 * This action calculates the best fit for a given body type for the given
 * mesh. Works for BOX, CYLINDER, and SPHERE body types.
 */
class BestFitAction : public Action
{
private:
  DynfactDialog* dialog;
  celBodyType type;

public:
  BestFitAction (DynfactDialog* dialog, celBodyType type) :
    dialog (dialog), type (type) { }
  virtual ~BestFitAction () { }
  virtual const char* GetName () const { return "Fit"; }
  virtual bool Do (View* view, wxWindow* component)
  {
    iDynamicFactory* fact = dialog->GetCurrentFactory ();
    if (!fact) return false;
    const csBox3& bbox = fact->GetBBox ();
    csVector3 c = bbox.GetCenter ();
    csVector3 s = bbox.GetSize ();
    Value* colliderSelectedValue = dialog->GetColliderSelectedValue ();
    switch (type)
    {
      case BODY_BOX:
	{
	  colliderSelectedValue->GetChildByName ("offsetX")->SetFloatValue (c.x);
	  colliderSelectedValue->GetChildByName ("offsetY")->SetFloatValue (c.y);
	  colliderSelectedValue->GetChildByName ("offsetZ")->SetFloatValue (c.z);
	  colliderSelectedValue->GetChildByName ("sizeX")->SetFloatValue (s.x);
	  colliderSelectedValue->GetChildByName ("sizeY")->SetFloatValue (s.y);
	  colliderSelectedValue->GetChildByName ("sizeZ")->SetFloatValue (s.z);
	  break;
	}
      case BODY_SPHERE:
	{
	  float radius = s.x;
	  if (s.y > radius) radius = s.y;
	  if (s.z > radius) radius = s.z;
	  radius /= 2.0f;
	  colliderSelectedValue->GetChildByName ("offsetX")->SetFloatValue (c.x);
	  colliderSelectedValue->GetChildByName ("offsetY")->SetFloatValue (c.y);
	  colliderSelectedValue->GetChildByName ("offsetZ")->SetFloatValue (c.z);
	  colliderSelectedValue->GetChildByName ("radius")->SetFloatValue (radius);
	  break;
	}
      case BODY_CYLINDER:
	{
	  float radius = s.x;
	  if (s.z > radius) radius = s.z;
	  radius /= 2.0f;
	  float length = s.y;
	  colliderSelectedValue->GetChildByName ("offsetX")->SetFloatValue (c.x);
	  colliderSelectedValue->GetChildByName ("offsetY")->SetFloatValue (c.y);
	  colliderSelectedValue->GetChildByName ("offsetZ")->SetFloatValue (c.z);
	  colliderSelectedValue->GetChildByName ("radius")->SetFloatValue (radius);
	  colliderSelectedValue->GetChildByName ("length")->SetFloatValue (length);
	  break;
	}
      default: CS_ASSERT (false);
    }
    return true;
  }
};

//--------------------------------------------------------------------------

void DynfactDialog::OnOkButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void DynfactDialog::Show ()
{
  uiManager->GetApp ()->GetAresView ()->GetDynfactCollectionValue ()->Refresh ();

  csRef<iEventTimer> timer = csEventTimer::GetStandardTimer (uiManager->GetApp ()->GetObjectRegistry ());
  timer->AddTimerEvent (timerOp, 25);

  ShowModal ();

  timer->RemoveTimerEvent (timerOp);
  meshView->SetMesh (0);
}

void DynfactDialog::Tick ()
{
  iVirtualClock* vc = uiManager->GetApp ()->GetVC ();
  meshView->RotateMesh (vc->GetElapsedSeconds ());
}

iDynamicFactory* DynfactDialog::GetCurrentFactory ()
{
  csString selectedFactory = factorySelectedValue->GetStringValue ();
  if (selectedFactory.IsEmpty ()) return 0;
  iPcDynamicWorld* dynworld = uiManager->GetApp ()->GetAresView ()->GetDynamicWorld ();
  iDynamicFactory* dynfact = dynworld->FindFactory (selectedFactory);
  return dynfact;
}

long DynfactDialog::GetSelectedCollider ()
{
  wxListCtrl* list = XRCCTRL (*this, "colliders_List", wxListCtrl);
  return ListCtrlTools::GetFirstSelectedRow (list);
}

DynfactDialog::DynfactDialog (wxWindow* parent, UIManager* uiManager) :
  View (this), uiManager (uiManager)
{
  AppAresEditWX* app = uiManager->GetApp ();
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("DynfactDialog"));

  // The mesh panel.
  wxPanel* panel = XRCCTRL (*this, "meshPanel", wxPanel);
  meshView = new DynfactMeshView (this, app->GetObjectRegistry (), panel);

  // The dialog for editing new factories.
  factoryDialog = new UIDialog (this, "Factory name");
  factoryDialog->AddRow ();
  factoryDialog->AddLabel ("Name:");
  factoryDialog->AddList ("name", NEWREF(Value,new MeshCollectionValue(app)), 0,
      "Name", "name");

  // Setup the dynamic factory tree.
  Value* dynfactCollectionValue = uiManager->GetApp ()->GetAresView ()->GetDynfactCollectionValue ();
  Bind (dynfactCollectionValue, "factoryTree");
  wxTreeCtrl* factoryTree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);
  factorySelectedValue.AttachNew (new TreeSelectedValue (factoryTree, dynfactCollectionValue, VALUE_COLLECTION));

  // Setup the collider list.
  DefineHeading ("colliders_List", "Type,Mass,x,y,z", "type,mass,offsetX,offsetY,offsetZ");

  // Setup the composite representing the dynamic factory that is selected.
  dynfactValue.AttachNew (new DynfactValue (this));
  Bind (dynfactValue, this);
  Value* colliders = dynfactValue->GetChildByName ("colliders");

  // Create a selection value that will follow the selection on the collider list.
  wxListCtrl* colliderList = XRCCTRL (*this, "colliders_List", wxListCtrl);
  colliderSelectedValue.AttachNew (new ListSelectedValue (colliderList, colliders, VALUE_COMPOSITE));
  colliderSelectedValue->SetupComposite (NEWREF(Value,new ColliderValue(0,0)));

  // Bind the selected collider value to the mesh view. This value is not actually
  // used by the mesh view but this binding only serves as a signal for the mesh
  // view to update itself.
  Bind (colliderSelectedValue, meshView);

  // Connect the selected value from the catetory tree to the dynamic
  // factory value so that the two radius values and the collider list
  // gets refreshed in case the current dynfact changes. We connect
  // with 'dochildren' equal to true to make sure the children get notified
  // as well (i.e. the list for example).
  Signal (factorySelectedValue, dynfactValue, true);
  // Also connect it to the selected value so that a new mesh is rendered
  // when the current dynfact changes.
  Signal (factorySelectedValue, colliderSelectedValue);

  // Bind the selection value to the different panels that describe the different types of colliders.
  Bind (colliderSelectedValue->GetChildByName ("type"), "type_colliderChoice");
  Bind (colliderSelectedValue, "box_ColliderPanel");
  Bind (colliderSelectedValue, "sphere_ColliderPanel");
  Bind (colliderSelectedValue, "cylinder_ColliderPanel");
  Bind (colliderSelectedValue, "mesh_ColliderPanel");
  Bind (colliderSelectedValue, "convexMesh_ColliderPanel");

  // The actions.
  AddAction (colliderList, NEWREF(Action, new NewChildAction (colliders)));
  AddAction (colliderList, NEWREF(Action, new DeleteChildAction (colliders)));
  AddAction (factoryTree, NEWREF(Action, new NewChildDialogAction (dynfactCollectionValue, factoryDialog)));
  AddAction (factoryTree, NEWREF(Action, new DeleteChildAction (dynfactCollectionValue)));
  AddAction (factoryTree, NEWREF(Action, new EditCategoryAction (this)));
  AddAction ("boxFitOffsetButton", NEWREF(Action, new BestFitAction(this, BODY_BOX)));
  AddAction ("sphereFitOffsetButton", NEWREF(Action, new BestFitAction(this, BODY_SPHERE)));
  AddAction ("cylinderFitOffsetButton", NEWREF(Action, new BestFitAction(this, BODY_CYLINDER)));

  timerOp.AttachNew (new RotMeshTimer (this));
}

DynfactDialog::~DynfactDialog ()
{
  delete meshView;
  delete factoryDialog;
}


