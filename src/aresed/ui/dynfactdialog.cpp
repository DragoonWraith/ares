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
#include "../models/model.h"
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
      case BODY_NONE: return "none";
      case BODY_BOX: return "box";
      case BODY_SPHERE: return "sphere";
      case BODY_CYLINDER: return "cylinder";
      case BODY_CONVEXMESH: return "convexmesh";
      case BODY_MESH: return "mesh";
      default: return "?";
    }
  }
  virtual void SetStringValue (const char* str)
  {
    csString sstr = str;
    celBodyType newtype;
    if (sstr == "none") newtype = BODY_NONE;
    else if (sstr == "box") newtype = BODY_BOX;
    else if (sstr == "sphere") newtype = BODY_SPHERE;
    else if (sstr == "cylinder") newtype = BODY_CYLINDER;
    else if (sstr == "convexmesh") newtype = BODY_CONVEXMESH;
    else if (sstr == "mesh") newtype = BODY_MESH;
    else newtype = BODY_NONE;
    if (newtype == *type) return;
    *type = newtype;
    FireValueChanged ();
  }
};

/**
 * A composite value representing a collider for a dynamic factory.
 */
class ColliderValue : public CompositeValue
{
private:
  celBodyInfo info;

public:
  ColliderValue (const celBodyInfo& info) : info (info)
  {
    csRef<Value> typeValue;
    typeValue.AttachNew (new ColliderTypeValue (&ColliderValue::info.type));
    AddChild ("type", typeValue);
  }
  virtual ~ColliderValue () { }
};

/**
 * A value representing the list of colliders for a dynamic factory.
 * Children of this value are of type ColliderValue.
 */
class ColliderCollectionValue : public Value
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;
  size_t idx;
  csRefArray<ColliderValue> colliders;

  void UpdateColliders ()
  {
    colliders.DeleteAll ();
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return;
    for (size_t i = 0 ; i < dynfact->GetBodyCount () ; i++)
    {
      csRef<ColliderValue> value;
      value.AttachNew (new ColliderValue (dynfact->GetBody (i)));
      colliders.Push (value);
    }
  }

public:
  ColliderCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~ColliderCollectionValue () { }

  virtual ValueType GetType () const { return VALUE_COLLECTION; }

  virtual void ResetIterator ()
  {
    UpdateColliders ();
    idx = 0;
  }
  virtual bool HasNext () { return idx < colliders.GetSize (); }
  virtual Value* NextChild ()
  {
    idx++;
    return colliders[idx-1];
  }

  virtual Value* GetChild (size_t idx) { UpdateColliders (); return colliders[idx]; }
  virtual Value* GetChild (const char* name) { return 0; }

  virtual bool DeleteValue (Value* child) { return false; }
  virtual bool AddValue (Value* child) { return false; }
};


class ColliderRowModel : public RowModel
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;
  size_t idx;

public:
  ColliderRowModel (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~ColliderRowModel () { }

  virtual void ResetIterator ()
  {
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return;
    idx = 0;
  }
  virtual bool HasRows () { return dynfact && idx < dynfact->GetBodyCount (); }
  virtual csStringArray NextRow ()
  {
    const char* type;
    celBodyInfo info = dynfact->GetBody (idx++);
    switch (info.type)
    {
      case BODY_BOX: type = "box"; break;
      case BODY_SPHERE: type = "sphere"; break;
      case BODY_CYLINDER: type = "cylinder"; break;
      case BODY_MESH: type = "mesh"; break;
      case BODY_CONVEXMESH: type = "convexmesh"; break;
      default: type = "?";
    }
    return Tools::MakeArray (type, (const char*)0);
  }

  virtual bool DeleteRow (const csStringArray& row) { return false; }
  virtual bool AddRow (const csStringArray& row) { return false; }

  virtual const char* GetColumns () { return "Type"; }
  virtual bool IsEditAllowed () const { return false; }
  virtual csStringArray EditRow (const csStringArray& origRow) { return origRow; }
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

class ColliderEditorModel : public EditorModel
{
private:
  DynfactDialog* dialog;
  DirtyHelper helper;

public:
  ColliderEditorModel (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~ColliderEditorModel () { }

  DirtyHelper& GetHelper () { return helper; }

  virtual void Update (const csStringArray& row)
  {
    if (row.GetSize () > 0)
      dialog->EditCollider (row[0]);
    else
      dialog->EditCollider (0);
  }
  virtual csStringArray Read ()
  {
    // @@@ Not yet implemented.
    return csStringArray ();
  }
  virtual bool IsDirty () { return helper.IsDirty (); }
  virtual void SetDirty (bool dirty) { helper.SetDirty (dirty); }
  virtual void AddDirtyListener (DirtyListener* listener) { helper.AddDirtyListener (listener); }
  virtual void RemoveDirtyListener (DirtyListener* listener) { helper.RemoveDirtyListener (listener); }
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

void DynfactDialog::EditCollider (const char* typeName)
{
  SetupColliderGeometry ();
}

void DynfactDialog::EditFactory (const char* meshName)
{
  meshView->SetMesh (meshName);
  colliderView->Refresh ();
  SetupColliderGeometry ();
}

void DynfactDialog::ClearColliderPanel ()
{
  wxChoicebook* book = XRCCTRL (*this, "colliderChoice", wxChoicebook);
  book->SetSelection (0);
  UITools::ClearControls (this,
      "boxMassText",
      "boxOffsetXText", "boxOffsetYText", "boxOffsetZText",
      "boxSizeXText", "boxSizeYText", "boxSizeZText",
      "sphereMassText",
      "sphereOffsetXText", "sphereOffsetYText", "sphereOffsetZText",
      "sphereRadiusText",
      "cylinderMassText",
      "cylinderOffsetXText", "cylinderOffsetYText", "cylinderOffsetZText",
      "cylinderRadiusText", "cylinderLengthText",
      "meshMassText",
      "meshOffsetXText", "meshOffsetYText", "meshOffsetZText",
      "cmeshMassText",
      "cmeshOffsetXText", "cmeshOffsetYText", "cmeshOffsetZText",
      (const char*)0);
}

void DynfactDialog::UpdateColliderPanel (const celBodyInfo& info)
{
  // @@@ mass
  switch (info.type)
  {
    case BODY_BOX:
      UITools::SwitchPage (this, "colliderChoice", "Box");
      UITools::SetValue (this, "boxMassText", info.mass);
      UITools::SetValue (this, "boxOffsetXText", info.offset.x);
      UITools::SetValue (this, "boxOffsetYText", info.offset.y);
      UITools::SetValue (this, "boxOffsetZText", info.offset.z);
      UITools::SetValue (this, "boxSizeXText", info.size.x);
      UITools::SetValue (this, "boxSizeYText", info.size.y);
      UITools::SetValue (this, "boxSizeZText", info.size.z);
      break;
    case BODY_SPHERE:
      UITools::SwitchPage (this, "colliderChoice", "Sphere");
      UITools::SetValue (this, "sphereMassText", info.mass);
      UITools::SetValue (this, "sphereOffsetXText", info.offset.x);
      UITools::SetValue (this, "sphereOffsetYText", info.offset.y);
      UITools::SetValue (this, "sphereOffsetZText", info.offset.z);
      UITools::SetValue (this, "sphereRadiusText", info.radius);
      break;
    case BODY_CYLINDER:
      UITools::SwitchPage (this, "colliderChoice", "Cylinder");
      UITools::SetValue (this, "cylinderMassText", info.mass);
      UITools::SetValue (this, "cylinderOffsetXText", info.offset.x);
      UITools::SetValue (this, "cylinderOffsetYText", info.offset.y);
      UITools::SetValue (this, "cylinderOffsetZText", info.offset.z);
      UITools::SetValue (this, "cylinderRadiusText", info.radius);
      UITools::SetValue (this, "cylinderLengthText", info.length);
      break;
    case BODY_MESH:
      UITools::SwitchPage (this, "colliderChoice", "Mesh");
      UITools::SetValue (this, "meshMassText", info.mass);
      UITools::SetValue (this, "meshOffsetXText", info.offset.x);
      UITools::SetValue (this, "meshOffsetYText", info.offset.y);
      UITools::SetValue (this, "meshOffsetZText", info.offset.z);
      break;
    case BODY_CONVEXMESH:
      UITools::SwitchPage (this, "colliderChoice", "Convex mesh");
      UITools::SetValue (this, "cmeshMassText", info.mass);
      UITools::SetValue (this, "cmeshOffsetXText", info.offset.x);
      UITools::SetValue (this, "cmeshOffsetYText", info.offset.y);
      UITools::SetValue (this, "cmeshOffsetZText", info.offset.z);
      break;
    default:
      UITools::SwitchPage (this, "colliderChoice", "None");
      break;
  }
}

void DynfactDialog::SetupColliderGeometry ()
{
  meshView->ClearGeometry ();
  wxListCtrl* list = XRCCTRL (*this, "colliderList", wxListCtrl);
  long idx = ListCtrlTools::GetFirstSelectedRow (list);

  ClearColliderPanel ();

  iDynamicFactory* dynfact = GetCurrentFactory ();
  if (dynfact)
  {
    for (size_t i = 0 ; i < dynfact->GetBodyCount () ; i++)
    {
      size_t pen = i == size_t (idx) ? hilightPen : normalPen;
      celBodyInfo info = dynfact->GetBody (i);
      if (i == size_t (idx))
      {
	UpdateColliderPanel (info);
      }
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

  wxListCtrl* list = XRCCTRL (*this, "colliderList", wxListCtrl);
  colliderModel.AttachNew (new ColliderRowModel (this));
  colliderView = new ListCtrlView (list, colliderModel);
  colliderEditorModel.AttachNew (new ColliderEditorModel (this));
  colliderEditorModel->GetHelper ().RegisterComponents (this,
      "colliderChoice",
      "boxMassText",
      "boxOffsetXText", "boxOffsetYText", "boxOffsetZText",
      "boxSizeXText", "boxSizeYText", "boxSizeZText",
      "sphereMassText",
      "sphereOffsetXText", "sphereOffsetYText", "sphereOffsetZText",
      "sphereRadiusText",
      "cylinderMassText",
      "cylinderOffsetXText", "cylinderOffsetYText", "cylinderOffsetZText",
      "cylinderRadiusText", "cylinderLengthText",
      "meshMassText",
      "meshOffsetXText", "meshOffsetYText", "meshOffsetZText",
      "cmeshMassText",
      "cmeshOffsetXText", "cmeshOffsetYText", "cmeshOffsetZText",
      (const char*)0);
  colliderView->SetEditorModel (colliderEditorModel);
  colliderView->SetApplyButton (this, "applyColliderButton");

  timerOp.AttachNew (new RotMeshTimer (this));
}

DynfactDialog::~DynfactDialog ()
{
  delete meshView;
  delete meshTreeView;
  delete colliderView;
}


