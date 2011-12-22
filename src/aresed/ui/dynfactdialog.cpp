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
#include "../tools/tools.h"

#include <wx/choicebk.h>

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(DynfactDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), DynfactDialog :: OnOkButton)
END_EVENT_TABLE()

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

protected:
  virtual void UpdateChildren ()
  {
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      children[i]->SetParent (0);
    children.DeleteAll ();
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return;
    for (size_t i = 0 ; i < dynfact->GetBodyCount () ; i++)
    {
      csRef<ColliderValue> value;
      value.AttachNew (new ColliderValue (i, dynfact));
      children.Push (value);
      value->SetParent (this);
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  ColliderCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~ColliderCollectionValue () { }

  /**
   * Call this when you want to refresh this value because external data (i.e.
   * the current factory) changes.
   */
  void Refresh () { FireValueChanged (); }

  virtual bool DeleteValue (Value* child) { return false; }
  virtual bool AddValue (Value* child) { return false; }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Col*]";
    dump += StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

//--------------------------------------------------------------------------

class FactoryEditorModel : public EditorModel
{
private:
  DynfactDialog* dialog;

public:
  FactoryEditorModel (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~FactoryEditorModel () { }

  virtual void Update (const csStringArray& row)
  {
    if (row.GetSize () > 1)
      dialog->EditFactory (row[1]);
    else
      dialog->EditFactory (0);
  }
  virtual csStringArray Read ()
  {
    // @@@ Not yet implemented.
    return csStringArray ();
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
  selIndex = -1;
  meshTreeView->Refresh ();

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
  csStringArray row = meshTreeView->GetSelectedRow ();
  if (row.GetSize () <= 1) return 0;
  iPcDynamicWorld* dynworld = uiManager->GetApp ()->GetAresView ()->GetDynamicWorld ();
  iDynamicFactory* dynfact = dynworld->FindFactory (row[1]);
  return dynfact;
}

void DynfactDialog::EditFactory (const char* meshName)
{
  meshView->SetMesh (meshName);

  wxListCtrl* colliderList = XRCCTRL (*this, "colliderList", wxListCtrl);
  ListCtrlTools::ClearSelection (colliderList, true);

  colliderCollectionValue->Refresh ();
  SetupColliderGeometry ();
}

void DynfactDialog::SetupColliderGeometry ()
{
  meshView->ClearGeometry ();
  wxListCtrl* list = XRCCTRL (*this, "colliderList", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);

  iDynamicFactory* dynfact = GetCurrentFactory ();
  if (dynfact)
  {
    for (size_t i = 0 ; i < dynfact->GetBodyCount () ; i++)
    {
      size_t pen = i == size_t (idx) ? hilightPen : normalPen;
      celBodyInfo info = dynfact->GetBody (i);
      if (info.type == BODY_BOX)
      {
        csBox3 b;
        b.SetSize (info.size);
	b.SetCenter (info.offset);
        meshView->AddBox (b, pen);
      }
      else if (info.type == BODY_SPHERE)
      {
	meshView->AddSphere (info.offset, info.radius, pen);
      }
      else if (info.type == BODY_CYLINDER)
      {
	meshView->AddCylinder (info.offset, info.radius, info.length, pen);
      }
      else if (info.type == BODY_MESH || info.type == BODY_CONVEXMESH)
      {
	meshView->AddMesh (info.offset, pen);
      }
    }
  }
}

DynfactDialog::DynfactDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("DynfactDialog"));
  wxPanel* panel = XRCCTRL (*this, "meshPanel", wxPanel);
  meshView = new MeshView (uiManager->GetApp ()->GetObjectRegistry (), panel);
  normalPen = meshView->CreatePen (0.5f, 0.0f, 0.0f, 0.5f);
  hilightPen = meshView->CreatePen (1.0f, 0.7f, 0.7f, 1.0f);

  wxTreeCtrl* tree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);
  meshTreeView = new TreeCtrlView (tree, uiManager->GetApp ()->GetAresView ()
      ->GetDynfactRowModel ());
  meshTreeView->SetRootName ("Categories");
  factoryEditorModel.AttachNew (new FactoryEditorModel (this));
  meshTreeView->SetEditorModel (factoryEditorModel);

  // Setup the view representing this dialog.
  colliderView.AttachNew (new View (this));

  // Define the collider list and value.
  colliderView->DefineHeading ("colliderList", "Type,Mass", "type,mass");
  colliderCollectionValue.AttachNew (new ColliderCollectionValue (this));
  colliderView->Bind (colliderCollectionValue, "colliderList");
  csRef<CollidersValueChangeListener> colListener;
  colListener.AttachNew (new CollidersValueChangeListener (this));
  colliderCollectionValue->AddValueChangeListener (colListener);

  // Create a selection value that will follow the selection on the collider list.
  wxListCtrl* colliderList = XRCCTRL (*this, "colliderList", wxListCtrl);
  colliderSelectedValue.AttachNew (new SelectedValue (colliderList, colliderCollectionValue, VALUE_COMPOSITE));
  csRef<ColliderValue> colliderValue;
  colliderValue.AttachNew (new ColliderValue (0, 0));
  colliderSelectedValue->SetupComposite (colliderValue);

  // Bind the selection value to the different panels that describe the different types of colliders.
  colliderView->Bind (colliderSelectedValue->GetChild ("type"), "type_colliderChoice");
  colliderView->Bind (colliderSelectedValue, "box_ColliderPanel");
  colliderView->Bind (colliderSelectedValue, "sphere_ColliderPanel");
  colliderView->Bind (colliderSelectedValue, "cylinder_ColliderPanel");
  colliderView->Bind (colliderSelectedValue, "mesh_ColliderPanel");
  colliderView->Bind (colliderSelectedValue, "convexMesh_ColliderPanel");

  timerOp.AttachNew (new RotMeshTimer (this));
}

DynfactDialog::~DynfactDialog ()
{
  delete meshView;
  delete meshTreeView;
  //delete colliderView;
}


