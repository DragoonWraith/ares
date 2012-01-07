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
  wxListCtrl* list = XRCCTRL (*this, "colliderList", wxListCtrl);
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
  factoryDialog->AddList ("name", NEWREF(MeshCollectionValue,new MeshCollectionValue(app->GetEngine ())), 0,
      "Name", "name");

  // Setup the dynamic factory tree.
  Value* dynfactCollectionValue = uiManager->GetApp ()->GetAresView ()->GetDynfactCollectionValue ();
  Bind (dynfactCollectionValue, "factoryTree");
  wxTreeCtrl* factoryTree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);
  factorySelectedValue.AttachNew (new TreeSelectedValue (factoryTree, dynfactCollectionValue, VALUE_COLLECTION));

  // Define the collider list and value.
  DefineHeading ("colliderList", "Type,Mass,x,y,z", "type,mass,offsetX,offsetY,offsetZ");
  colliderCollectionValue.AttachNew (new ColliderCollectionValue (this));
  Bind (colliderCollectionValue, "colliderList");

  // Create a selection value that will follow the selection on the collider list.
  wxListCtrl* colliderList = XRCCTRL (*this, "colliderList", wxListCtrl);
  colliderSelectedValue.AttachNew (new ListSelectedValue (colliderList, colliderCollectionValue, VALUE_COMPOSITE));
  colliderSelectedValue->SetupComposite (NEWREF(ColliderValue,new ColliderValue(0,0)));

  // Bind the selected collider value to the mesh view. This value is not actually
  // used by the mesh view but this binding only serves as a signal for the mesh
  // view to update itself.
  Bind (colliderSelectedValue, meshView);

  // Connect the selected value from the category tree to the collection
  // of colliders so that it gets refreshed if the current category changes.
  // Also connect it to the mesh view so that a new mesh is rendered.
  Signal (factorySelectedValue, colliderCollectionValue);
  Signal (factorySelectedValue, colliderSelectedValue);

  // Bind the selection value to the different panels that describe the different types of colliders.
  Bind (colliderSelectedValue->GetChild ("type"), "type_colliderChoice");
  Bind (colliderSelectedValue, "box_ColliderPanel");
  Bind (colliderSelectedValue, "sphere_ColliderPanel");
  Bind (colliderSelectedValue, "cylinder_ColliderPanel");
  Bind (colliderSelectedValue, "mesh_ColliderPanel");
  Bind (colliderSelectedValue, "convexMesh_ColliderPanel");

  // The actions.
  AddAction (colliderList, NEWREF(Action, new NewChildAction (colliderCollectionValue)));
  AddAction (colliderList, NEWREF(Action, new DeleteChildAction (colliderCollectionValue)));
  AddAction (factoryTree, NEWREF(Action, new NewChildDialogAction (dynfactCollectionValue, factoryDialog)));
  AddAction (factoryTree, NEWREF(Action, new DeleteChildAction (dynfactCollectionValue)));

  timerOp.AttachNew (new RotMeshTimer (this));
}

DynfactDialog::~DynfactDialog ()
{
  delete meshView;
  delete factoryDialog;
}


