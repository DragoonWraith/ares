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

#include "physicallayer/pl.h"

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
  originXPen = CreatePen (1.0f, 0.0f, 0.0f, 1.0f);
  originYPen = CreatePen (0.0f, 1.0f, 0.0f, 1.0f);
  originZPen = CreatePen (0.0f, 0.0f, 1.0f, 1.0f);
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
  iDynamicFactory* fact = dialog->GetCurrentFactory ();
  if (fact)
  {
    AddLine (csVector3 (0), csVector3 (.1, 0, 0), originXPen);
    AddLine (csVector3 (0), csVector3 (0, .1, 0), originYPen);
    AddLine (csVector3 (0), csVector3 (0, 0, .1), originZPen);

    long idx = dialog->GetSelectedCollider ();
    for (size_t i = 0 ; i < fact->GetBodyCount () ; i++)
    {
      size_t pen = i == size_t (idx) ? hilightPen : normalPen;
      celBodyInfo info = fact->GetBody (i);
      if (info.type == BOX_COLLIDER_GEOMETRY)
        AddBox (csBox3 (info.offset - info.size * .5, info.offset + info.size * .5), pen);
      else if (info.type == SPHERE_COLLIDER_GEOMETRY)
	AddSphere (info.offset, info.radius, pen);
      else if (info.type == CYLINDER_COLLIDER_GEOMETRY)
	AddCylinder (info.offset, info.radius, info.length, pen);
      else if (info.type == CAPSULE_COLLIDER_GEOMETRY)
	AddCapsule (info.offset, info.radius, info.length, pen);
      else if (info.type == TRIMESH_COLLIDER_GEOMETRY || info.type == CONVEXMESH_COLLIDER_GEOMETRY)
	AddMesh (info.offset, pen);
    }
    idx = dialog->GetSelectedPivot ();
    for (size_t i = 0 ; i < fact->GetPivotJointCount () ; i++)
    {
      size_t pen = i == size_t (idx) ? hilightPen : normalPen;
      csVector3 pos = fact->GetPivotJointPosition (i);
      AddSphere (pos, .01, pen);
    }
    idx = dialog->GetSelectedJoint ();
    for (size_t i = 0 ; i < fact->GetJointCount () ; i++)
    {
      size_t pen = i == size_t (idx) ? hilightPen : normalPen;
      DynFactJointDefinition& def = fact->GetJoint (i);
      csVector3 pos = def.GetTransform ().GetOrigin ();
      AddSphere (pos, .01, pen);
    }
  }
}

//--------------------------------------------------------------------------

using namespace Ares;

/**
 * A composite value representing an attribute for a dynamic factory.
 */
class AttributeValue : public CompositeValue
{
private:
  csStringID nameID;
  iDynamicFactory* dynfact;
  csRef<Value> valueValue;

protected:
  virtual void ChildChanged (Value* child)
  {
    dynfact->SetAttribute (nameID, valueValue->GetStringValue ());
    FireValueChanged ();
  }

public:
  AttributeValue (csStringID nameID, const char* name, iDynamicFactory* dynfact) : nameID (nameID), dynfact (dynfact)
  {
    csString value;
    if (dynfact)
    {
      value = dynfact->GetAttribute (nameID);
    }
    valueValue.AttachNew (new StringValue (value));
    AddChild ("attrName", NEWREF(Value,new StringValue (name)));
    AddChild ("attrValue", valueValue);
  }
  virtual ~AttributeValue () { }

  csStringID GetName () const { return nameID; }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Att]";
    dump += CompositeValue::Dump (verbose);
    return dump;
  }
};

/**
 * A value representing the list of attributes for a dynamic factory.
 * Children of this value are of type AttributeValue.
 */
class AttributeCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;

  // Create a new child and add to the array.
  Value* NewChild (csStringID nameID, const char* name)
  {
    csRef<AttributeValue> value;
    value.AttachNew (new AttributeValue (nameID, name, dynfact));
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
    AresEdit3DView* ares3d = dialog->GetUIManager ()->GetApp ()->GetAresView ();
    csRef<iAttributeIterator> it = dynfact->GetAttributes ();
    while (it->HasNext ())
    {
      csStringID nameID = it->Next ();
      csString name = ares3d->GetPL ()->FetchString (nameID);
      if (name != "category" && name != "defaultstatic")
        NewChild (nameID, name);
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  AttributeCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~AttributeCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return false;
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      if (children[i] == child)
      {
	AttributeValue* attValue = static_cast<AttributeValue*> (child);
	dynfact->ClearAttribute (attValue->GetName ());
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
    csString name = suggestion.Get ("name", (const char*)0);
    csString value = suggestion.Get ("value", (const char*)0);
    AresEdit3DView* ares3d = dialog->GetUIManager ()->GetApp ()->GetAresView ();
    csStringID nameID = ares3d->GetPL ()->FetchStringID (name);
    dynfact->SetAttribute (nameID, value);
    Value* child = NewChild (nameID, name);
    FireValueChanged ();
    return child;
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Att*]";
    dump += StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

//--------------------------------------------------------------------------

/**
 * A composite value representing a joint for a dynamic factory.
 */
class JointValue : public CompositeValue
{
private:
  size_t idx;
  iDynamicFactory* dynfact;
  csVector3 origin;
  DynFactJointDefinition def;

protected:
  virtual void ChildChanged (Value* child)
  {
    def.trans.SetOrigin (origin);
    dynfact->SetJoint (idx, def);
    FireValueChanged ();
  }

public:
  JointValue (size_t idx, iDynamicFactory* dynfact) : idx (idx), dynfact (dynfact)
  {
    if (dynfact)
    {
      def = dynfact->GetJoint (idx);
      origin = def.trans.GetOrigin ();
    }
    else
    {
      origin.Set (0, 0, 0);
    }
    AddChild ("jointPosX", NEWREF(Value,new FloatPointerValue (&origin.x)));
    AddChild ("jointPosY", NEWREF(Value,new FloatPointerValue (&origin.y)));
    AddChild ("jointPosZ", NEWREF(Value,new FloatPointerValue (&origin.z)));
    AddChild ("bounceX", NEWREF(Value,new FloatPointerValue (&def.bounce.x)));
    AddChild ("bounceY", NEWREF(Value,new FloatPointerValue (&def.bounce.y)));
    AddChild ("bounceZ", NEWREF(Value,new FloatPointerValue (&def.bounce.z)));
    AddChild ("maxforceX", NEWREF(Value,new FloatPointerValue (&def.maxforce.x)));
    AddChild ("maxforceY", NEWREF(Value,new FloatPointerValue (&def.maxforce.y)));
    AddChild ("maxforceZ", NEWREF(Value,new FloatPointerValue (&def.maxforce.z)));
    AddChild ("xLockTrans", NEWREF(Value,new BoolPointerValue (&def.transX)));
    AddChild ("yLockTrans", NEWREF(Value,new BoolPointerValue (&def.transY)));
    AddChild ("zLockTrans", NEWREF(Value,new BoolPointerValue (&def.transZ)));
    AddChild ("xMinTrans", NEWREF(Value,new FloatPointerValue (&def.mindist.x)));
    AddChild ("yMinTrans", NEWREF(Value,new FloatPointerValue (&def.mindist.y)));
    AddChild ("zMinTrans", NEWREF(Value,new FloatPointerValue (&def.mindist.z)));
    AddChild ("xMaxTrans", NEWREF(Value,new FloatPointerValue (&def.maxdist.x)));
    AddChild ("yMaxTrans", NEWREF(Value,new FloatPointerValue (&def.maxdist.y)));
    AddChild ("zMaxTrans", NEWREF(Value,new FloatPointerValue (&def.maxdist.z)));
    AddChild ("xLockRot", NEWREF(Value,new BoolPointerValue (&def.rotX)));
    AddChild ("yLockRot", NEWREF(Value,new BoolPointerValue (&def.rotY)));
    AddChild ("zLockRot", NEWREF(Value,new BoolPointerValue (&def.rotZ)));
    AddChild ("xMinRot", NEWREF(Value,new FloatPointerValue (&def.minrot.x)));
    AddChild ("yMinRot", NEWREF(Value,new FloatPointerValue (&def.minrot.y)));
    AddChild ("zMinRot", NEWREF(Value,new FloatPointerValue (&def.minrot.z)));
    AddChild ("xMaxRot", NEWREF(Value,new FloatPointerValue (&def.maxrot.x)));
    AddChild ("yMaxRot", NEWREF(Value,new FloatPointerValue (&def.maxrot.y)));
    AddChild ("zMaxRot", NEWREF(Value,new FloatPointerValue (&def.maxrot.z)));
  }
  virtual ~JointValue () { }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Jnt]";
    dump += CompositeValue::Dump (verbose);
    return dump;
  }
};

/**
 * A value representing the list of joints for a dynamic factory.
 * Children of this value are of type JointValue.
 */
class JointCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;

  // Create a new child and add to the array.
  Value* NewChild (size_t i)
  {
    csRef<JointValue> value;
    value.AttachNew (new JointValue (i, dynfact));
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
    for (size_t i = 0 ; i < dynfact->GetJointCount () ; i++)
      NewChild (i);
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  JointCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~JointCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return false;
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      if (children[i] == child)
      {
	dynfact->RemovePivotJoint (i);
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
    dynfact->CreateJoint ();
    idx = dynfact->GetJointCount ()-1;
    Value* value = NewChild (idx);
    FireValueChanged ();
    return value;
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Jnt*]";
    dump += StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

//--------------------------------------------------------------------------

/**
 * A value representing the list of bones for a skeleton factory.
 */
class FactoryBoneCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;

  CS::Animation::iSkeletonFactory* GetSkeletonFactory ()
  {
    if (!dynfact) return 0;
    csString itemname = dynfact->GetName ();
    UIManager* uiManager = dialog->GetUIManager ();
    AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
    iMeshFactoryWrapper* meshFact = ares3d->GetEngine ()->FindMeshFactory (itemname);
    CS_ASSERT (meshFact != 0);

    csRef<CS::Mesh::iAnimatedMeshFactory> animFact = scfQueryInterface<CS::Mesh::iAnimatedMeshFactory> (meshFact->GetMeshObjectFactory ());
    if (!animFact) return 0;

    CS::Animation::iSkeletonFactory* skelFact = animFact->GetSkeletonFactory ();
    return skelFact;
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
    using namespace CS::Animation;
    CS::Animation::iSkeletonFactory* skelFact = GetSkeletonFactory ();
    if (!skelFact) return;
    const csArray<BoneID>& bones = skelFact->GetBoneOrderList ();
    for (size_t i = 0 ; i < bones.GetSize () ; i++)
    {
      BoneID id = bones[i];
      csRef<CompositeValue> value;
      value.AttachNew (new CompositeValue ());
      value->AddChild ("name", NEWREF(Value,new StringValue(skelFact->GetBoneName (id))));
      children.Push (value);
      value->SetParent (this);
    }
  }

public:
  FactoryBoneCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~FactoryBoneCollectionValue () { }
};

//--------------------------------------------------------------------------

/**
 * A composite value representing a pivot joint for a dynamic factory.
 */
class PivotValue : public CompositeValue
{
private:
  size_t idx;
  iDynamicFactory* dynfact;
  csVector3 pos;

protected:
  virtual void ChildChanged (Value* child)
  {
    dynfact->SetPivotJointPosition (idx, pos);
    FireValueChanged ();
  }

public:
  PivotValue (size_t idx, iDynamicFactory* dynfact) : idx (idx), dynfact (dynfact)
  {
    if (dynfact) pos = dynfact->GetPivotJointPosition (idx);
    AddChild ("pivotX", NEWREF(Value,new FloatPointerValue (&pos.x)));
    AddChild ("pivotY", NEWREF(Value,new FloatPointerValue (&pos.y)));
    AddChild ("pivotZ", NEWREF(Value,new FloatPointerValue (&pos.z)));
  }
  virtual ~PivotValue () { }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Piv]";
    dump += CompositeValue::Dump (verbose);
    return dump;
  }
};

/**
 * A value representing the list of pivot joints for a dynamic factory.
 * Children of this value are of type PivotValue.
 */
class PivotCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;

  // Create a new child and add to the array.
  Value* NewChild (size_t i)
  {
    csRef<PivotValue> value;
    value.AttachNew (new PivotValue (i, dynfact));
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
    for (size_t i = 0 ; i < dynfact->GetPivotJointCount () ; i++)
      NewChild (i);
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
    //UIManager* uiManager = dialog->GetUIManager ();
    //AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
    //ares3d->SetupFactorySettings (dialog->GetCurrentFactory ());
  }

public:
  PivotCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~PivotCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return false;
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      if (children[i] == child)
      {
	dynfact->RemovePivotJoint (i);
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
    dynfact->CreatePivotJoint (csVector3 (0, 0, 0));
    idx = dynfact->GetPivotJointCount ()-1;
    Value* value = NewChild (idx);
    FireValueChanged ();
    return value;
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[Piv*]";
    dump += StandardCollectionValue::Dump (verbose);
    return dump;
  }
};

//--------------------------------------------------------------------------

/**
 * A value for the type of a collider.
 */
class ColliderTypeValue : public Value
{
private:
  csColliderGeometryType* type;

public:
  ColliderTypeValue (csColliderGeometryType* t) : type (t) { }
  virtual ~ColliderTypeValue () { }

  virtual ValueType GetType () const { return VALUE_STRING; }
  virtual const char* GetStringValue ()
  {
    switch (*type)
    {
      case NO_GEOMETRY: return "None";
      case BOX_COLLIDER_GEOMETRY: return "Box";
      case SPHERE_COLLIDER_GEOMETRY: return "Sphere";
      case CYLINDER_COLLIDER_GEOMETRY: return "Cylinder";
      case CAPSULE_COLLIDER_GEOMETRY: return "Capsule";
      case CONVEXMESH_COLLIDER_GEOMETRY: return "Convex mesh";
      case TRIMESH_COLLIDER_GEOMETRY: return "Mesh";
      default: return "?";
    }
  }
  virtual void SetStringValue (const char* str)
  {
    csString sstr = str;
    csColliderGeometryType newtype;
    if (sstr == "None") newtype = NO_GEOMETRY;
    else if (sstr == "Box") newtype = BOX_COLLIDER_GEOMETRY;
    else if (sstr == "Sphere") newtype = SPHERE_COLLIDER_GEOMETRY;
    else if (sstr == "Cylinder") newtype = CYLINDER_COLLIDER_GEOMETRY;
    else if (sstr == "Capsule") newtype = CAPSULE_COLLIDER_GEOMETRY;
    else if (sstr == "Convex mesh") newtype = CONVEXMESH_COLLIDER_GEOMETRY;
    else if (sstr == "Mesh") newtype = TRIMESH_COLLIDER_GEOMETRY;
    else newtype = NO_GEOMETRY;
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
 * A composite value representing a collider.
 */
template <class T>
class TypedColliderValue : public CompositeValue
{
protected:
  size_t idx;
  T* parent;
  celBodyInfo info;

public:
  TypedColliderValue (size_t idx, T* parent) : idx (idx), parent (parent) { }

  void Setup ()
  {
    AddChild ("type", NEWREF(Value,new ColliderTypeValue (&TypedColliderValue::info.type)));
    AddChild ("offsetX", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.offset.x)));
    AddChild ("offsetY", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.offset.y)));
    AddChild ("offsetZ", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.offset.z)));
    AddChild ("mass", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.mass)));
    AddChild ("radius", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.radius)));
    AddChild ("length", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.length)));
    AddChild ("sizeX", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.size.x)));
    AddChild ("sizeY", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.size.y)));
    AddChild ("sizeZ", NEWREF(Value,new FloatPointerValue (&TypedColliderValue::info.size.z)));
  }
  virtual ~TypedColliderValue () { }
};

/**
 * A composite value representing a collider for a dynamic factory.
 */
class DynfactColliderValue : public TypedColliderValue<iDynamicFactory>
{
protected:
  virtual void ChildChanged (Value* child)
  {
    switch (info.type)
    {
      case NO_GEOMETRY:
	break;
      case BOX_COLLIDER_GEOMETRY:
	parent->AddRigidBox (info.offset, info.size, info.mass, idx);
	break;
      case SPHERE_COLLIDER_GEOMETRY:
	parent->AddRigidSphere (info.radius, info.offset, info.mass, idx);
	break;
      case CYLINDER_COLLIDER_GEOMETRY:
	parent->AddRigidCylinder (info.radius, info.length, info.offset, info.mass, idx);
	break;
      case CAPSULE_COLLIDER_GEOMETRY:
	parent->AddRigidCapsule (info.radius, info.length, info.offset, info.mass, idx);
	break;
      case CONVEXMESH_COLLIDER_GEOMETRY:
	parent->AddRigidConvexMesh (info.offset, info.mass, idx);
	break;
      case TRIMESH_COLLIDER_GEOMETRY:
	parent->AddRigidMesh (info.offset, info.mass, idx);
	break;
      default:
	printf ("Something is wrong: unknown collider type %d\n", info.type);
	break;
    }
    FireValueChanged ();
  }

public:
  DynfactColliderValue (size_t idx, iDynamicFactory* dynfact)
    : TypedColliderValue<iDynamicFactory> (idx, dynfact)
  {
    if (dynfact) info = dynfact->GetBody (idx);
    Setup ();
  }
  virtual ~DynfactColliderValue () { }
};

/**
 * A composite value representing a collider for a bone.
 */
class BoneColliderValue : public TypedColliderValue<CS::Animation::iBodyBone>
{
protected:
  virtual void ChildChanged (Value* child)
  {
    CS::Animation::iBodyBoneCollider* collider = parent->GetBoneCollider (idx);
    collider->SetTransform (csOrthoTransform (csMatrix3 (), info.offset));
    switch (info.type)
    {
      case NO_GEOMETRY:
	break;
      case BOX_COLLIDER_GEOMETRY:
	collider->SetBoxGeometry (info.size);
	break;
      case SPHERE_COLLIDER_GEOMETRY:
	collider->SetSphereGeometry (info.radius);
	break;
      case CYLINDER_COLLIDER_GEOMETRY:
	collider->SetCylinderGeometry (info.length, info.radius);
	break;
      case CAPSULE_COLLIDER_GEOMETRY:
	collider->SetCapsuleGeometry (info.length, info.radius);
	break;
      case CONVEXMESH_COLLIDER_GEOMETRY:
	// @@@ TODO
	break;
      case TRIMESH_COLLIDER_GEOMETRY:
	// @@@ TODO
	break;
      default:
	printf ("Something is wrong: unknown collider type %d\n", info.type);
	break;
    }
    FireValueChanged ();
  }

public:
  BoneColliderValue (size_t idx, CS::Animation::iBodyBone* bone)
    : TypedColliderValue<CS::Animation::iBodyBone> (idx, bone)
  {
    if (bone)
    {
      CS::Animation::iBodyBoneCollider* collider = bone->GetBoneCollider (idx);
      info.type = collider->GetGeometryType ();
      info.offset.Set (0, 0, 0);
      info.size.Set (0, 0, 0);
      info.mass = 0.0f;
      info.radius = 0.0f;
      info.length = 0.0f;
      switch (info.type)
      {
	case BOX_COLLIDER_GEOMETRY:
	  collider->GetBoxGeometry (info.size);
	  break;
	case SPHERE_COLLIDER_GEOMETRY:
	  collider->GetSphereGeometry (info.radius);
	  break;
	case CYLINDER_COLLIDER_GEOMETRY:
	  collider->GetCylinderGeometry (info.length, info.radius);
	  break;
	case CAPSULE_COLLIDER_GEOMETRY:
	  collider->GetCapsuleGeometry (info.length, info.radius);
	  break;
	default:
	  break;
      }
    }
    Setup ();
  }
  virtual ~BoneColliderValue () { }
};

/**
 * A value representing the list of colliders for a dynamic factory.
 * Children of this value are of type DynfactColliderValue.
 */
class ColliderCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;

  // Create a new child and add to the array.
  Value* NewChild (size_t i)
  {
    csRef<DynfactColliderValue> value;
    value.AttachNew (new DynfactColliderValue (i, dynfact));
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
    const csBox3& bbox = dynfact->GetBBox ();
    csVector3 c = bbox.GetCenter ();
    csVector3 s = bbox.GetSize ();
    dynfact->AddRigidBox (c, s, 1.0f);
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

/**
 * A value representing the list of colliders for a bone.
 * Children of this value are of type BoneColliderValue.
 */
class BoneColliderCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  CS::Animation::iBodyBone* bone;

  // Create a new child and add to the array.
  Value* NewChild (size_t i)
  {
    csRef<BoneColliderValue> value;
    value.AttachNew (new BoneColliderValue (i, bone));
    children.Push (value);
    value->SetParent (this);
    return value;
  }

protected:
  virtual void UpdateChildren ()
  {
    if (bone != dialog->GetCurrentBone ()) dirty = true;
    if (!dirty) return;
    ReleaseChildren ();
    bone = dialog->GetCurrentBone ();
    if (!bone) return;
    dirty = false;
    for (size_t i = 0 ; i < bone->GetBoneColliderCount () ; i++)
      NewChild (i);
  }
  //virtual void ChildChanged (Value* child)
  //{
    //FireValueChanged ();
    //UIManager* uiManager = dialog->GetUIManager ();
    //AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
    //ares3d->SetupFactorySettings (dialog->GetCurrentFactory ());
  //}

public:
  BoneColliderCollectionValue (DynfactDialog* dialog) : dialog (dialog), bone (0) { }
  virtual ~BoneColliderCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
#if 0
    bone = dialog->GetCurrentBone ();
    if (!bone) return false;
    for (size_t i = 0 ; i < children.GetSize () ; i++)
      if (children[i] == child)
      {
	bone->DeleteBody (i);
	child->SetParent (0);
	children.DeleteIndex (i);
	FireValueChanged ();
	return true;
      }
#endif
    return false;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    bone = dialog->GetCurrentBone ();
    if (!bone) return 0;
    CS::Animation::iBodyBoneCollider* collider = bone->CreateBoneCollider ();
    collider->SetBoxGeometry (csVector3 (.02, .02, .02));
    idx = bone->GetBoneColliderCount ()-1;
    Value* value = NewChild (idx);
    FireValueChanged ();
    return value;
  }

  virtual csString Dump (bool verbose = false)
  {
    csString dump = "[BCol*]";
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

/// Value for the static value of a dynamic factory.
class StaticValue : public BoolValue
{
private:
  DynfactDialog* dialog;
public:
  StaticValue (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~StaticValue () { }
  virtual void SetBoolValue (bool f)
  {
    printf ("StaticValue::SetBoolValue %d\n", f); fflush (stdout);
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    if (dynfact)
    {
      dynfact->SetAttribute ("defaultstatic", f ? "true" : "false");
      //UIManager* uiManager = dialog->GetUIManager ();
      //AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
      //ares3d->SetupFactorySettings (dynfact);
      FireValueChanged ();
    }
  }
  virtual bool GetBoolValue ()
  {
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return false;
    const char* st = dynfact->GetAttribute ("defaultstatic");
    return (st && *st == 't');
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

/**
 * A value representing the list of bones for a dynamic factory.
 * Children of this value are of type BoneValue.
 */
class BoneCollectionValue : public StandardCollectionValue
{
private:
  DynfactDialog* dialog;
  iDynamicFactory* dynfact;

  // Create a new child and add to the array.
  Value* NewChild (const char* boneName)
  {
    csRef<CompositeValue> value;
    value.AttachNew (new CompositeValue ());
    value->AddChild ("name", NEWREF(Value,new StringValue (boneName)));
    children.Push (value);
    value->SetParent (this);
    return value;
  }

  CS::Animation::iSkeletonFactory* GetSkeletonFactory ()
  {
    if (!dynfact) return 0;
    csString itemname = dynfact->GetName ();
    UIManager* uiManager = dialog->GetUIManager ();
    AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
    iMeshFactoryWrapper* meshFact = ares3d->GetEngine ()->FindMeshFactory (itemname);
    CS_ASSERT (meshFact != 0);

    csRef<CS::Mesh::iAnimatedMeshFactory> animFact = scfQueryInterface<CS::Mesh::iAnimatedMeshFactory> (meshFact->GetMeshObjectFactory ());
    if (!animFact) return 0;

    CS::Animation::iSkeletonFactory* skelFact = animFact->GetSkeletonFactory ();
    return skelFact;
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
    using namespace CS::Animation;
    iBodySkeleton* bodySkel = dialog->GetBodyManager ()
      ->FindBodySkeleton (dynfact->GetName ());
    if (!bodySkel) return;
    CS::Animation::iSkeletonFactory* skelFact = GetSkeletonFactory ();
    csRef<iBoneIDIterator> it = bodySkel->GetBodyBones ();
    while (it->HasNext ())
    {
      BoneID id = it->Next ();
      NewChild (skelFact->GetBoneName (id));
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
  }

public:
  BoneCollectionValue (DynfactDialog* dialog) : dialog (dialog), dynfact (0) { }
  virtual ~BoneCollectionValue () { }

  virtual bool DeleteValue (Value* child)
  {
    // @@@ Not implemented yet.
    return false;
  }
  virtual Value* NewValue (size_t idx, Value* selectedValue, const DialogResult& suggestion)
  {
    dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return 0;
    using namespace CS::Animation;
    csString name = suggestion.Get ("name", (const char*)0);

    iBodySkeleton* bodySkel = dialog->GetBodyManager ()
      ->FindBodySkeleton (dynfact->GetName ());
    if (!bodySkel) return 0;

    CS::Animation::iSkeletonFactory* skelFact = GetSkeletonFactory ();
    BoneID id = skelFact->FindBone (name);
    if (id == InvalidBoneID) return 0;

    bodySkel->CreateBodyBone (id);

    Value* value = NewChild (name);
    FireValueChanged ();
    return value;
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
  AddChild ("pivots", NEWREF(Value,new PivotCollectionValue (dialog)));
  AddChild ("joints", NEWREF(Value,new JointCollectionValue (dialog)));
  AddChild ("attributes", NEWREF(Value,new AttributeCollectionValue (dialog)));
  AddChild ("bones", NEWREF(Value,new BoneCollectionValue (dialog)));
  AddChild ("maxRadius", NEWREF(Value,new MaxRadiusValue(dialog)));
  AddChild ("imposterRadius", NEWREF(Value,new ImposterRadiusValue(dialog)));
  AddChild ("static", NEWREF(Value,new StaticValue(dialog)));
}

void DynfactValue::ChildChanged (Value* child)
{
  iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
  if (dynfact)
  {
    UIManager* uiManager = dialog->GetUIManager ();
    AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
    ares3d->SetupFactorySettings (dynfact);
  }
}

//--------------------------------------------------------------------------

BoneValue::BoneValue (DynfactDialog* dialog) : dialog (dialog)
{
  // Setup the composite representing the dynamic factory that is selected.
  AddChild ("boneColliders", NEWREF(Value,new BoneColliderCollectionValue (dialog)));
}

void BoneValue::ChildChanged (Value* child)
{
  //iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
  //if (dynfact)
  //{
    //UIManager* uiManager = dialog->GetUIManager ();
    //AresEdit3DView* ares3d = uiManager->GetApp ()->GetAresView ();
    //ares3d->SetupFactorySettings (dynfact);
  //}
}

//--------------------------------------------------------------------------

bool CreateBodySkeletonAction::Do (View* view, wxWindow* component)
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

  csString itemname = value->GetStringValue ();
  iMeshFactoryWrapper* meshFact = ares3d->GetEngine ()->FindMeshFactory (itemname);
  CS_ASSERT (meshFact != 0);

  csRef<CS::Mesh::iAnimatedMeshFactory> animFact = scfQueryInterface<CS::Mesh::iAnimatedMeshFactory> (meshFact->GetMeshObjectFactory ());
  if (!animFact)
  {
    uiManager->Error ("This item is not an animesh!");
    return false;
  }

  CS::Animation::iSkeletonFactory* skelFact = animFact->GetSkeletonFactory ();
  if (!skelFact)
  {
    uiManager->Error ("This item has no skeleton!");
    return false;
  }

  if (dialog->GetBodyManager ()->FindBodySkeleton (itemname))
  {
    uiManager->Error ("There is already a body skeleton for this item!");
    return false;
  }

  //iBodySkeleton* bodySkel = bodyManager->CreateBodySkeleton (itemname, skelFact);
  dialog->GetBodyManager ()->CreateBodySkeleton (itemname, skelFact);

  return true;
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
  csColliderGeometryType type;

public:
  BestFitAction (DynfactDialog* dialog, csColliderGeometryType type) :
    dialog (dialog), type (type) { }
  virtual ~BestFitAction () { }
  virtual const char* GetName () const { return "Fit"; }
  virtual bool Do (View* view, wxWindow* component)
  {
    iDynamicFactory* fact = dialog->GetCurrentFactory ();
    if (!fact) return false;
    dialog->FitCollider (fact, type);
    return true;
  }
};

//--------------------------------------------------------------------------

bool NewInvisibleChildAction::Do (View* view, wxWindow* component)
{
  UIDialog* dialog = new UIDialog (component, "Factory name");
  dialog->AddRow ();
  dialog->AddLabel ("Name:");
  dialog->AddText ("name");
  dialog->AddRow ();
  dialog->AddLabel ("Min corner:");
  dialog->AddText ("minx");
  dialog->AddText ("miny");
  dialog->AddText ("minz");
  dialog->AddRow ();
  dialog->AddLabel ("Max corner:");
  dialog->AddText ("maxx");
  dialog->AddText ("maxy");
  dialog->AddText ("maxz");
  bool d = DoDialog (view, component, dialog);
  delete dialog;
  return d;
}

//--------------------------------------------------------------------------

Offset2MinMaxValue::Offset2MinMaxValue (Value* offset, Value* size, bool operatorPlus)
  : offset (offset), size (size), operatorPlus (operatorPlus)
{
  listener.AttachNew (new StandardChangeListener (this));
  offset->AddValueChangeListener (listener);
  size->AddValueChangeListener (listener);
}

Offset2MinMaxValue::~Offset2MinMaxValue ()
{
  offset->RemoveValueChangeListener (listener);
  size->RemoveValueChangeListener (listener);
}

//--------------------------------------------------------------------------

/**
 * This standard action creates a new default child for a collection.
 * It assumes the collection supports the NewValue() method. It will
 * call the NewValue() method with an empty suggestion array.
 */
class ContainerBoxAction : public Action
{
private:
  DynfactDialog* dialog;
  Value* collection;

public:
  ContainerBoxAction (DynfactDialog* dialog, Value* collection)
    : dialog (dialog), collection (collection) { }
  virtual ~ContainerBoxAction () { }
  virtual const char* GetName () const { return "Container Box"; }
  virtual bool Do (View* view, wxWindow* component)
  {
    iDynamicFactory* fact = dialog->GetCurrentFactory ();
    if (!fact) return false;
    csString dimS = dialog->GetUIManager ()->AskDialog ("Thickness of box sides", "Thickness:");
    if (dimS.IsEmpty ()) return false;
    float dim;
    csScanStr (dimS, "%f", &dim);
    const csBox3& bbox = fact->GetBBox ();
    csBox3 bottom = bbox;
    bottom.SetMax (1, bottom.GetMin (1)+dim);
    fact->AddRigidBox (bottom.GetCenter (), bottom.GetSize (), 1.0);
    csBox3 left = bbox;
    left.SetMin (1, bottom.GetMax (1));
    left.SetMax (0, left.GetMin (0)+dim);
    fact->AddRigidBox (left.GetCenter (), left.GetSize (), 1.0);
    csBox3 right = bbox;
    right.SetMin (1, bottom.GetMax (1));
    right.SetMin (0, right.GetMax (0)-dim);
    fact->AddRigidBox (right.GetCenter (), right.GetSize (), 1.0);
    csBox3 up = bbox;
    up.SetMin (1, bottom.GetMax (1));
    up.SetMin (0, left.GetMax (0));
    up.SetMax (0, right.GetMin (0));
    up.SetMax (2, right.GetMin (2)+dim);
    fact->AddRigidBox (up.GetCenter (), up.GetSize (), 1.0);
    csBox3 down = bbox;
    down.SetMin (1, bottom.GetMax (1));
    down.SetMin (0, left.GetMax (0));
    down.SetMax (0, right.GetMin (0));
    down.SetMin (2, right.GetMax (2)-dim);
    fact->AddRigidBox (down.GetCenter (), down.GetSize (), 1.0);
    collection->Refresh ();
    dialog->GetColliderSelectedValue ()->Refresh ();
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

CS::Animation::iBodyBone* DynfactDialog::GetCurrentBone ()
{
  if (!factorySelectedValue) return 0;
  csString selectedFactory = factorySelectedValue->GetStringValue ();
  if (selectedFactory.IsEmpty ()) return 0;
  CS::Animation::iBodySkeleton* bodySkel = bodyManager
      ->FindBodySkeleton (selectedFactory);
  if (!bodySkel) return 0;
  Value* nameValue = bonesSelectedValue->GetChildByName ("name");
  csString selectedBone = nameValue->GetStringValue ();
  return bodySkel->FindBodyBone (selectedBone);
}

iDynamicFactory* DynfactDialog::GetCurrentFactory ()
{
  if (!factorySelectedValue) return 0;
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

long DynfactDialog::GetSelectedPivot ()
{
  wxListCtrl* list = XRCCTRL (*this, "pivots_List", wxListCtrl);
  return ListCtrlTools::GetFirstSelectedRow (list);
}

long DynfactDialog::GetSelectedJoint ()
{
  wxListCtrl* list = XRCCTRL (*this, "joints_List", wxListCtrl);
  return ListCtrlTools::GetFirstSelectedRow (list);
}

void DynfactDialog::FitCollider (iDynamicFactory* fact, csColliderGeometryType type)
{
  const csBox3& bbox = fact->GetBBox ();
  csVector3 c = bbox.GetCenter ();
  csVector3 s = bbox.GetSize ();
  switch (type)
  {
    case BOX_COLLIDER_GEOMETRY:
      {
	colliderSelectedValue->GetChildByName ("offsetX")->SetFloatValue (c.x);
	colliderSelectedValue->GetChildByName ("offsetY")->SetFloatValue (c.y);
	colliderSelectedValue->GetChildByName ("offsetZ")->SetFloatValue (c.z);
	colliderSelectedValue->GetChildByName ("sizeX")->SetFloatValue (s.x);
	colliderSelectedValue->GetChildByName ("sizeY")->SetFloatValue (s.y);
	colliderSelectedValue->GetChildByName ("sizeZ")->SetFloatValue (s.z);
	break;
      }
    case SPHERE_COLLIDER_GEOMETRY:
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
    case CAPSULE_COLLIDER_GEOMETRY:
    case CYLINDER_COLLIDER_GEOMETRY:
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
    default: return;
  }
}

void DynfactDialog::SetupDialogs ()
{
  AppAresEditWX* app = uiManager->GetApp ();

  // The dialog for editing new factories.
  factoryDialog = new UIDialog (this, "Factory name");
  factoryDialog->AddRow (1);
  factoryDialog->AddLabel ("Name:");
  factoryDialog->AddList ("name", NEWREF(Value,new MeshCollectionValue(app)), 0,
      "Name", "name");

  // The dialog for editing new attributes.
  attributeDialog = new UIDialog (this, "Attribute");
  attributeDialog->AddRow ();
  attributeDialog->AddLabel ("Name:");
  attributeDialog->AddText ("name");
  attributeDialog->AddRow ();
  attributeDialog->AddLabel ("Value:");
  attributeDialog->AddText ("value");

  // The dialog for selecting a bone.
  selectBoneDialog = new UIDialog (this, "Select Bone");
  selectBoneDialog->AddRow ();
  selectBoneDialog->AddList ("name", NEWREF(Value,new FactoryBoneCollectionValue(this)), 0,
      "Name", "name");
}

static csString C3 (const char* s1, const char* s2, const char* s3)
{
  csString s (s1);
  s += s2;
  s += s3;
  return s;
}

void DynfactDialog::SetupColliderEditor (Value* colSelValue, const char* suffix)
{
  // Bind the selection value to the different panels that describe the different
  // types of colliders.
  Bind (colSelValue->GetChildByName ("type"), C3 ("type_", suffix, "ColliderChoice"));
  Bind (colSelValue, C3 ("box_", suffix, "ColliderPanel"));
  Bind (colSelValue, C3 ("sphere_", suffix, "ColliderPanel"));
  Bind (colSelValue, C3 ("cylinder_", suffix, "ColliderPanel"));
  Bind (colSelValue, C3 ("capsule_", suffix, "ColliderPanel"));
  Bind (colSelValue, C3 ("mesh_", suffix, "ColliderPanel"));
  Bind (colSelValue, C3 ("convexMesh_", suffix, "ColliderPanel"));

  // Bind calculated value for the box collider so that there are also min/max
  // controls in addition to offset/size.
  Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetX"),
	  colSelValue->GetChildByName ("sizeX"), false)), C3 ("minX_", suffix, "BoxText"));
  Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetY"),
	  colSelValue->GetChildByName ("sizeY"), false)), C3 ("minY_", suffix, "BoxText"));
  Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetZ"),
	  colSelValue->GetChildByName ("sizeZ"), false)), C3 ("minZ_", suffix, "BoxText"));
  Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetX"),
	  colSelValue->GetChildByName ("sizeX"), true)), C3 ("maxX_", suffix, "BoxText"));
  Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetY"),
	  colSelValue->GetChildByName ("sizeY"), true)), C3 ("maxY_", suffix, "BoxText"));
  Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetZ"),
	  colSelValue->GetChildByName ("sizeZ"), true)), C3 ("maxZ_", suffix, "BoxText"));
}

void DynfactDialog::SetupJointsEditor (Value* jointsSelectedValue)
{
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("xLockTrans")), "xMinTrans");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("xLockTrans")), "xMaxTrans");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("yLockTrans")), "yMinTrans");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("yLockTrans")), "yMaxTrans");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("zLockTrans")), "zMinTrans");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("zLockTrans")), "zMaxTrans");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("xLockRot")), "xMinRot");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("xLockRot")), "xMaxRot");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("yLockRot")), "yMinRot");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("yLockRot")), "yMaxRot");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("zLockRot")), "zMinRot");
  BindEnabled (Not (jointsSelectedValue->GetChildByName ("zLockRot")), "zMaxRot");
}

void DynfactDialog::SetupActions ()
{
  Value* colliders = dynfactValue->GetChildByName ("colliders");
  Value* pivots = dynfactValue->GetChildByName ("pivots");
  Value* joints = dynfactValue->GetChildByName ("joints");
  Value* bones = dynfactValue->GetChildByName ("bones");
  Value* boneColliders = boneValue->GetChildByName ("boneColliders");
  Value* dynfactCollectionValue = uiManager->GetApp ()->GetAresView ()->GetDynfactCollectionValue ();

  wxListCtrl* bonesList = XRCCTRL (*this, "bones_List", wxListCtrl);
  wxListCtrl* jointsList = XRCCTRL (*this, "joints_List", wxListCtrl);
  wxListCtrl* pivotsList = XRCCTRL (*this, "pivots_List", wxListCtrl);
  wxListCtrl* colliderList = XRCCTRL (*this, "colliders_List", wxListCtrl);
  wxListCtrl* bonesColliderList = XRCCTRL (*this, "boneColliders_List", wxListCtrl);
  wxTreeCtrl* factoryTree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);

  // The actions.
  AddAction (colliderList, NEWREF(Action, new NewChildAction (colliders)));
  AddAction (colliderList, NEWREF(Action, new ContainerBoxAction (this, colliders)));
  AddAction (colliderList, NEWREF(Action, new DeleteChildAction (colliders)));
  AddAction (bonesColliderList, NEWREF(Action, new NewChildAction (boneColliders)));
  AddAction (pivotsList, NEWREF(Action, new NewChildAction (pivots)));
  AddAction (pivotsList, NEWREF(Action, new DeleteChildAction (pivots)));
  AddAction (jointsList, NEWREF(Action, new NewChildAction (joints)));
  AddAction (jointsList, NEWREF(Action, new DeleteChildAction (joints)));
  AddAction (bonesList, NEWREF(Action, new NewChildDialogAction (bones, selectBoneDialog)));
  AddAction (factoryTree, NEWREF(Action, new NewChildDialogAction (dynfactCollectionValue, factoryDialog)));
  AddAction (factoryTree, NEWREF(Action, new NewInvisibleChildAction (dynfactCollectionValue)));
  AddAction (factoryTree, NEWREF(Action, new DeleteChildAction (dynfactCollectionValue)));
  AddAction (factoryTree, NEWREF(Action, new EditCategoryAction (this)));
  AddAction (factoryTree, NEWREF(Action, new CreateBodySkeletonAction (this)));
  Value* attributes = dynfactValue->GetChildByName ("attributes");
  AddAction ("attributes_List", NEWREF(Action, new NewChildDialogAction (attributes, attributeDialog)));
  AddAction ("attributes_List", NEWREF(Action, new DeleteChildAction (attributes)));
  AddAction ("boxFitOffsetButton", NEWREF(Action, new BestFitAction(this, BOX_COLLIDER_GEOMETRY)));
  AddAction ("sphereFitOffsetButton", NEWREF(Action, new BestFitAction(this, SPHERE_COLLIDER_GEOMETRY)));
  AddAction ("cylinderFitOffsetButton", NEWREF(Action, new BestFitAction(this, CYLINDER_COLLIDER_GEOMETRY)));
  AddAction ("capsuleFitOffsetButton", NEWREF(Action, new BestFitAction(this, CAPSULE_COLLIDER_GEOMETRY)));
}

void DynfactDialog::SetupListHeadings ()
{
  // Setup the lists.
  DefineHeading ("colliders_List", "Type,Mass,x,y,z", "type,mass,offsetX,offsetY,offsetZ");
  DefineHeading ("boneColliders_List", "Type,Mass,x,y,z", "type,mass,offsetX,offsetY,offsetZ");
  DefineHeading ("pivots_List", "x,y,z", "pivotX,pivotY,pivotZ");
  DefineHeading ("joints_List", "x,y,z,tx,ty,tz,rx,ry,rz", "jointPosX,jointPosY,jointPosZ,xLockTrans,yLockTrans,zLockTrans,xLockRot,yLockRot,zLockRot");
  DefineHeading ("attributes_List", "Name,Value", "attrName,attrValue");
  DefineHeading ("bones_List", "Name", "name");
}

void DynfactDialog::SetupSelectedValues ()
{
  // Create a selection value that will follow the selection on the collider list.
  Value* colliders = dynfactValue->GetChildByName ("colliders");
  wxListCtrl* colliderList = XRCCTRL (*this, "colliders_List", wxListCtrl);
  colliderSelectedValue.AttachNew (new ListSelectedValue (colliderList, colliders, VALUE_COMPOSITE));
  colliderSelectedValue->SetupComposite (NEWREF(Value,new DynfactColliderValue(0,0)));

  // Create a selection value that will follow the selection on the pivot list.
  Value* pivots = dynfactValue->GetChildByName ("pivots");
  wxListCtrl* pivotsList = XRCCTRL (*this, "pivots_List", wxListCtrl);
  pivotsSelectedValue.AttachNew (new ListSelectedValue (pivotsList, pivots, VALUE_COMPOSITE));
  pivotsSelectedValue->SetupComposite (NEWREF(Value,new PivotValue(0,0)));

  // Create a selection value that will follow the selection on the joint list.
  Value* joints = dynfactValue->GetChildByName ("joints");
  wxListCtrl* jointsList = XRCCTRL (*this, "joints_List", wxListCtrl);
  jointsSelectedValue.AttachNew (new ListSelectedValue (jointsList, joints, VALUE_COMPOSITE));
  jointsSelectedValue->SetupComposite (NEWREF(Value,new JointValue(0,0)));

  // Create a selection value that will follow the selection on the bones list.
  Value* bones = dynfactValue->GetChildByName ("bones");
  wxListCtrl* bonesList = XRCCTRL (*this, "bones_List", wxListCtrl);
  bonesSelectedValue.AttachNew (new ListSelectedValue (bonesList, bones, VALUE_COMPOSITE));
  bonesSelectedValue->AddChild ("name", NEWREF(MirrorValue,new MirrorValue(VALUE_STRING)));

  // Create a selection value that will follow the selection on the collider list.
  Value* boneColliders = boneValue->GetChildByName ("boneColliders");
  wxListCtrl* bonesColliderList = XRCCTRL (*this, "boneColliders_List", wxListCtrl);
  bonesColliderSelectedValue.AttachNew (new ListSelectedValue (bonesColliderList, boneColliders, VALUE_COMPOSITE));
  bonesColliderSelectedValue->SetupComposite (NEWREF(Value,new BoneColliderValue(0,0)));
}

DynfactDialog::DynfactDialog (wxWindow* parent, UIManager* uiManager) :
  View (this), uiManager (uiManager)
{
  AppAresEditWX* app = uiManager->GetApp ();
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("DynfactDialog"));

  bodyManager = csQueryRegistry<CS::Animation::iBodyManager> (app->GetObjectRegistry ());
  if (!bodyManager)
  {
    printf ("Can't find body manager!\n");
    fflush (stdout);
    return;
  }

  // The mesh panel.
  wxPanel* panel = XRCCTRL (*this, "meshPanel", wxPanel);
  meshView = new DynfactMeshView (this, app->GetObjectRegistry (), panel);

  SetupDialogs ();

  // Setup the dynamic factory tree.
  Value* dynfactCollectionValue = uiManager->GetApp ()->GetAresView ()->GetDynfactCollectionValue ();
  Bind (dynfactCollectionValue, "factoryTree");
  wxTreeCtrl* factoryTree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);
  factorySelectedValue.AttachNew (new TreeSelectedValue (factoryTree, dynfactCollectionValue, VALUE_COLLECTION));

  SetupListHeadings ();

  // Setup the composite representing the dynamic factory that is selected.
  dynfactValue.AttachNew (new DynfactValue (this));
  Bind (dynfactValue, this);

  // Setup the composite representing the bone that is selected.
  boneValue.AttachNew (new BoneValue (this));
  Bind (boneValue, "bonesPanel");

  SetupSelectedValues ();

  // Bind the selected collider value to the mesh view. This value is not actually
  // used by the mesh view but this binding only serves as a signal for the mesh
  // view to update itself.
  Bind (colliderSelectedValue, meshView);
  // Also do this for the selected pivot value.
  Bind (pivotsSelectedValue, meshView);
  // Also do this for the selected joint value.
  Bind (jointsSelectedValue, meshView);
  // Also do this for the selected bone value.
  Bind (bonesSelectedValue, meshView);

  // Connect the selected value from the category tree to the dynamic
  // factory value so that the two radius values and the collider list
  // gets refreshed in case the current dynfact changes. We connect
  // with 'dochildren' equal to true to make sure the children get notified
  // as well (i.e. the list for example).
  Signal (factorySelectedValue, dynfactValue, true);
  // Also connect it to the selected values so that a new mesh is rendered
  // when the current dynfact changes.
  Signal (factorySelectedValue, colliderSelectedValue);
  Signal (factorySelectedValue, pivotsSelectedValue);
  Signal (factorySelectedValue, jointsSelectedValue);
  Signal (factorySelectedValue, bonesSelectedValue);

  // When another bone is selected we want to update the selection of the
  // bone collider too.
  Signal (bonesSelectedValue, boneValue, true);
  Signal (bonesSelectedValue, bonesColliderSelectedValue);

  // Setup the collider editors.
  SetupColliderEditor (colliderSelectedValue, "");
  SetupColliderEditor (bonesColliderSelectedValue, "Bone");

  Bind (pivotsSelectedValue, "pivotPosition_Panel");
  Bind (jointsSelectedValue, "joints_Panel");

  // Bind some values to the enabled/disabled state of several components.
  BindEnabled (pivotsSelectedValue->GetSelectedState (), "pivotPosition_Panel");
  BindEnabled (jointsSelectedValue->GetSelectedState (), "joints_Panel");
  SetupJointsEditor (jointsSelectedValue);

  SetupActions ();

  timerOp.AttachNew (new RotMeshTimer (this));
}

DynfactDialog::~DynfactDialog ()
{
  delete meshView;
  delete factoryDialog;
  delete attributeDialog;
  delete selectBoneDialog;
}


