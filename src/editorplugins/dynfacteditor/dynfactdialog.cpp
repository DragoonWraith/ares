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

#include <crystalspace.h>
#include "dynfactdialog.h"
#include "editor/iuimanager.h"
#include "editor/iuidialog.h"
#include "editor/iapp.h"
#include "editor/i3dview.h"
#include "editor/imodelrepository.h"
#include "edcommon/listctrltools.h"
#include "edcommon/uitools.h"
#include "meshview.h"
#include "meshfactmodel.h"
#include "edcommon/tools.h"

#include "physicallayer/pl.h"
#include "propclass/dynworld.h"

#include "lightdialog.h"

#include <wx/choicebk.h>

SCF_IMPLEMENT_FACTORY (DynfactDialog)

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(DynfactDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), DynfactDialog :: OnOkButton)
  EVT_CHECKBOX (XRCID("showBodiesCheck"), DynfactDialog :: OnShowBodies)
  EVT_CHECKBOX (XRCID("showOriginCheck"), DynfactDialog :: OnShowOrigin)
  EVT_CHECKBOX (XRCID("showBonesCheck"), DynfactDialog :: OnShowBones)
  EVT_CHECKBOX (XRCID("showJointsCheck"), DynfactDialog :: OnShowJoints)
END_EVENT_TABLE()

csStringID DynfactDialog::ID_Show = csInvalidStringID;

//--------------------------------------------------------------------------

static void CorrectFactoryName (csString& name)
{
  if (name[name.Length ()-1] == '*')
    name = name.Slice (0, name.Length ()-1);
}

//--------------------------------------------------------------------------

DynfactMeshView::DynfactMeshView (DynfactDialog* dialog, iObjectRegistry* object_reg, wxWindow* parent) :
    MeshView (object_reg, parent), dialog (dialog)
{
  normalPen = CreatePen (0.5f, 0.0f, 0.0f, 0.5f);
  hilightPen = CreatePen (1.0f, 0.7f, 0.7f, 1.0f);
  originXPen = CreatePen (1.0f, 0.0f, 0.0f, 1.0f);
  originYPen = CreatePen (0.0f, 1.0f, 0.0f, 1.0f);
  originZPen = CreatePen (0.0f, 0.0f, 1.0f, 1.0f);
  bonePen = CreatePen (0.5f, 0.5f, 0.5f, 1.0f);
  boneActivePen = CreatePen (0.5f, 0.5f, 0.0f, 1.0f);
  boneHiPen = CreatePen (1.0f, 1.0f, 0.0f, 1.0f);
  showBodies = true;
  showJoints = true;
  showBones = true;
  showOrigin = true;
}

void DynfactMeshView::SyncValue (Ares::Value* value)
{
  Refresh ();
}

bool DynfactMeshView::SetMesh (const char* name)
{
  animesh = 0;
  if (!MeshView::SetMesh (name))
    return false;
  animesh = scfQueryInterface<CS::Mesh::iAnimatedMesh> (mesh->GetMeshObject ());

  // If it's an animesh then cancel the animation
  if (animesh
      && animesh->GetSkeleton ()->GetAnimationPacket ())
  {
    CS::Animation::iSkeletonAnimNode* root =
      animesh->GetSkeleton ()->GetAnimationPacket ()->GetAnimationRoot ();
    if (root)
    {
      root->Stop ();
      animesh->GetSkeleton ()->ResetSkeletonState ();
    }
  }

  return true;
}

void DynfactMeshView::Refresh ()
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
    long idx;

    if (showOrigin)
    {
      AddLine (csVector3 (0), csVector3 (.1, 0, 0), originXPen);
      AddLine (csVector3 (0), csVector3 (0, .1, 0), originYPen);
      AddLine (csVector3 (0), csVector3 (0, 0, .1), originZPen);
    }

    // Render the bodies.
    if (showBodies)
    {
      idx = dialog->GetSelectedCollider ();
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
      using namespace CS::Animation;
      iBodyBone* bone = dialog->GetCurrentBone ();
      if (bone)
      {
	CS::Animation::iSkeleton* skeleton = animesh->GetSkeleton ();

	// Bone to object space transform
        csQuaternion rotation;
        csVector3 position;
        skeleton->GetTransformAbsSpace (bone->GetAnimeshBone (), rotation, position);
	csReversibleTransform o2b (csMatrix3 (rotation.GetConjugate ()), position); 
	csReversibleTransform b2o = o2b.GetInverse ();

        idx = dialog->GetSelectedBoneCollider ();
        for (size_t i = 0 ; i < bone->GetBoneColliderCount () ; i++)
	{
	  size_t pen = i == size_t (idx) ? hilightPen : normalPen;
	  SetLocal2ObjectTransform (pen, b2o);
          iBodyBoneCollider* collider = bone->GetBoneCollider (i);
	  csOrthoTransform trans = collider->GetTransform ();
	  csVector3 offset = trans.GetOrigin ();
	  csColliderGeometryType type = collider->GetGeometryType ();
	  if (type == BOX_COLLIDER_GEOMETRY)
	  {
	    csVector3 size;
	    collider->GetBoxGeometry (size);
	    AddBox (csBox3 (offset - size * .5, offset + size * .5), pen);
	  }
	  else if (type == SPHERE_COLLIDER_GEOMETRY)
	  {
	    float radius;
	    collider->GetSphereGeometry (radius);
	    AddSphere (offset, radius, pen);
	  }
	  else if (type == CYLINDER_COLLIDER_GEOMETRY)
	  {
	    float length, radius;
	    collider->GetCylinderGeometry (length, radius);
	    AddCylinder (offset, radius, length, pen);
	  }
	  else if (type == CAPSULE_COLLIDER_GEOMETRY)
	  {
	    float length, radius;
	    collider->GetCapsuleGeometry (length, radius);
	    AddCapsule (offset, radius, length, pen);
	  }
	  //else if (info.type == TRIMESH_COLLIDER_GEOMETRY || info.type == CONVEXMESH_COLLIDER_GEOMETRY)
	    //AddMesh (info.offset, pen);
	  SetLocal2ObjectTransform (pen, csReversibleTransform ());
	}
      }
    }

    // Render pivot points.
    if (showJoints)
    {
      idx = dialog->GetSelectedPivot ();
      for (size_t i = 0 ; i < fact->GetPivotJointCount () ; i++)
      {
	size_t pen = i == size_t (idx) ? hilightPen : normalPen;
	csVector3 pos = fact->GetPivotJointPosition (i);
	AddSphere (pos, .01, pen);
      }

      // Render joints.
      idx = dialog->GetSelectedJoint ();
      for (size_t i = 0 ; i < fact->GetJointCount () ; i++)
      {
	size_t pen = i == size_t (idx) ? hilightPen : normalPen;
	DynFactJointDefinition& def = fact->GetJoint (i);
	csVector3 pos = def.GetTransform ().GetOrigin ();
	AddSphere (pos, .01, pen);
      }
    }

    // Render skeleton bones.
    if (showBones)
    {
      using namespace CS::Animation;
      CS::Animation::iSkeletonFactory* skelFact = dialog->GetSkeletonFactory (fact->GetName ());
      if (skelFact)
      {
	size_t pen;
	iBodySkeleton* bodySkel = dialog->GetBodyManager ()->FindBodySkeleton (fact->GetName ());
	const csArray<BoneID> bones = skelFact->GetBoneOrderList ();
	csString selBone = dialog->GetSelectedBone ();
	for (size_t i = 0 ; i < bones.GetSize () ; i++)
	{
	  BoneID id = bones[i];
	  if (bodySkel)
	  {
	    if (selBone == skelFact->GetBoneName (id))
	      pen = boneHiPen;
	    else
	      pen = boneActivePen;
	  }
	  else
	  {
	    pen = bonePen;
	  }
	  csQuaternion rot;
	  csVector3 offset;
	  skelFact->GetTransformAbsSpace (id, rot, offset);
	  AddSphere (offset, .02, pen);
	  AddLine (offset, offset + rot.Rotate (csVector3 (0, 0, .3)), pen);
	}
      }
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
    iCelPlLayer* pl = dialog->GetPL ();
    csRef<iAttributeIterator> it = dynfact->GetAttributes ();
    while (it->HasNext ())
    {
      csStringID nameID = it->Next ();
      csString name = pl->FetchString (nameID);
      if (name != "category" && name != "defaultstatic")
        NewChild (nameID, name);
    }
  }
  virtual void ChildChanged (Value* child)
  {
    FireValueChanged ();
    dialog->AddDirtyFactory (dynfact);
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
	dialog->AddDirtyFactory (dynfact);
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
    iCelPlLayer* pl = dialog->GetPL ();
    csStringID nameID = pl->FetchStringID (name);
    dynfact->SetAttribute (nameID, value);
    Value* child = NewChild (nameID, name);
    FireValueChanged ();
    dialog->AddDirtyFactory (dynfact);
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
 * A composite value representing a joint.
 */
class TypedJointValue : public CompositeValue
{
protected:
  csVector3 origin;
  DynFactJointDefinition def;

  void SetupChildren ()
  {
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

  void ClearDef ()
  {
    def.trans.Identity ();
    def.transX = false;
    def.transY = false;
    def.transZ = false;
    def.mindist.Set (0, 0, 0);
    def.maxdist.Set (0, 0, 0);
    def.rotX = false;
    def.rotY = false;
    def.rotZ = false;
    def.minrot.Set (0, 0, 0);
    def.maxrot.Set (0, 0, 0);
    def.bounce.Set (0, 0, 0);
    def.maxforce.Set (0, 0, 0);
  }

public:
  TypedJointValue ()
  {
    ClearDef ();
  }
  virtual ~TypedJointValue () { }
};

/**
 * A composite value representing the joint for a bone.
 */
class BoneJointValue : public TypedJointValue
{
private:
  DynfactDialog* dialog;

protected:
  virtual void ChildChanged (Value* child)
  {
    def.trans.SetOrigin (origin);
    CS::Animation::iBodyBone* bone = dialog->GetCurrentBone ();
    if (bone && bone->GetBoneJoint ())
    {
      CS::Animation::iBodyBoneJoint* joint = bone->GetBoneJoint ();
      joint->SetTransform (def.trans);
      joint->SetTransConstraints (def.transX, def.transY, def.transZ);
      joint->SetMinimumDistance (def.mindist);
      joint->SetMaximumDistance (def.maxdist);
      joint->SetRotConstraints (def.rotX, def.rotY, def.rotZ);
      joint->SetMinimumAngle (def.minrot);
      joint->SetMaximumAngle (def.maxrot);
      joint->SetBounce (def.bounce);
    }
    FireValueChanged ();
    dialog->AddDirtyFactory (dialog->GetCurrentFactory ());
  }

  void SetBone ()
  {
    CS::Animation::iBodyBone* bone = dialog->GetCurrentBone ();
    if (bone && bone->GetBoneJoint ())
    {
      CS::Animation::iBodyBoneJoint* joint = bone->GetBoneJoint ();
      def.trans = joint->GetTransform ();
      def.transX = joint->IsXTransConstrained ();
      def.transY = joint->IsYTransConstrained ();
      def.transZ = joint->IsZTransConstrained ();
      def.mindist = joint->GetMinimumDistance ();
      def.maxdist = joint->GetMaximumDistance ();
      def.rotX = joint->IsXRotConstrained ();
      def.rotY = joint->IsYRotConstrained ();
      def.rotZ = joint->IsZRotConstrained ();
      def.minrot = joint->GetMinimumAngle ();
      def.maxrot = joint->GetMaximumAngle ();
      def.bounce = joint->GetBounce ();
      //def.maxforce = joint->GetMaxForce ();
      def.maxforce.Set (0, 0, 0);
      origin = def.trans.GetOrigin ();
    }
    else
    {
      ClearDef ();
    }
  }

public:
  BoneJointValue (DynfactDialog* dialog) : dialog (dialog)
  {
    SetBone ();
    SetupChildren ();
  }
  virtual ~BoneJointValue () { }

  virtual void FireValueChanged ()
  {
    SetBone ();
    TypedJointValue::FireValueChanged ();
  }
};

/**
 * A composite value representing a joint for a dynamic factory.
 */
class JointValue : public TypedJointValue
{
private:
  size_t idx;
  iDynamicFactory* dynfact;

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
    SetupChildren ();
  }
  virtual ~JointValue () { }
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
    dialog->AddDirtyFactory (dynfact);
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
	dialog->AddDirtyFactory (dynfact);
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
    dialog->AddDirtyFactory (dynfact);
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
    CorrectFactoryName (itemname);
    return dialog->GetSkeletonFactory (itemname);
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
      NewCompositeChild (VALUE_STRING, "name", skelFact->GetBoneName (id), VALUE_NONE);
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
    //i3DView* view3d = dialog->GetApplication ()->Get3DView ();
    //view3d->RefreshFactorySettings (dialog->GetCurrentFactory ());
    dialog->AddDirtyFactory (dynfact);
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
        dialog->AddDirtyFactory (dynfact);
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
    dialog->AddDirtyFactory (dynfact);
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
      csOrthoTransform trans = collider->GetTransform ();
      info.offset = trans.GetOrigin ();
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
    dialog->AddDirtyFactory (dialog->GetCurrentFactory ());
    FireValueChanged ();
    i3DView* view3d = dialog->GetApplication ()->Get3DView ();
    view3d->RefreshFactorySettings (dialog->GetCurrentFactory ());
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
    dialog->AddDirtyFactory (dynfact);
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
    //i3DView* view3d = dialog->GetApplication ()->Get3DView ();
    //view3d->RefreshFactorySettings (dialog->GetCurrentFactory ());
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
    // @@@ Auto-fit here!
    collider->SetBoxGeometry (csVector3 (.02, .02, .02));
    dialog->UpdateRagdoll ();
    idx = bone->GetBoneColliderCount ()-1;
    Value* value = NewChild (idx);
    dialog->AddDirtyFactory (dialog->GetCurrentFactory ());
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
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    if (dynfact)
    {
      dynfact->SetAttribute ("defaultstatic", f ? "true" : "false");
      //i3DView* view3d = dialog->GetApplication ()->Get3DView ();
      //view3d->RefreshFactorySettings (dynfact);
      dialog->AddDirtyFactory (dynfact);
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

/// Value for the collider value of a dynamic factory.
class ColliderValue : public BoolValue
{
private:
  DynfactDialog* dialog;
public:
  ColliderValue (DynfactDialog* dialog) : dialog (dialog) { }
  virtual ~ColliderValue () { }
  virtual void SetBoolValue (bool f)
  {
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    if (dynfact)
    {
      dynfact->SetColliderEnabled (f);
      //i3DView* view3d = dialog->GetApplication ()->Get3DView ();
      //view3d->RefreshFactorySettings (dynfact);
      dialog->AddDirtyFactory (dynfact);
      FireValueChanged ();
    }
  }
  virtual bool GetBoolValue ()
  {
    iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
    if (!dynfact) return false;
    return dynfact->IsColliderEnabled ();
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
    dialog->AddDirtyFactory (dynfact);
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

  CS::Animation::iSkeletonFactory* GetSkeletonFactory ()
  {
    if (!dynfact) return 0;
    csString itemname = dynfact->GetName ();
    CorrectFactoryName (itemname);
    iEngine* engine = dialog->GetEngine ();
    iMeshFactoryWrapper* meshFact = engine->FindMeshFactory (itemname);
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
      NewCompositeChild (VALUE_STRING, "name", skelFact->GetBoneName (id), VALUE_NONE);
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

    Value* value = NewCompositeChild (VALUE_STRING, "name", name.GetData (), VALUE_NONE);
    dialog->AddDirtyFactory (dynfact);
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
  AddChild ("collider", NEWREF(Value,new ColliderValue(dialog)));
}

void DynfactValue::ChildChanged (Value* child)
{
  iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
  if (dynfact)
  {
    i3DView* view3d = dialog->GetApplication ()->Get3DView ();
    view3d->GetModelRepository ()->GetDynfactCollectionValue ()->Refresh ();
    view3d->RefreshFactorySettings (dynfact);
  }
}

//--------------------------------------------------------------------------

BoneValue::BoneValue (DynfactDialog* dialog) : dialog (dialog)
{
  // Setup the composite representing the dynamic factory that is selected.
  AddChild ("boneColliders", NEWREF(Value,new BoneColliderCollectionValue (dialog)));
  AddChild ("boneJoint", NEWREF(Value,new BoneJointValue (dialog)));
}

void BoneValue::ChildChanged (Value* child)
{
  //iDynamicFactory* dynfact = dialog->GetCurrentFactory ();
  //if (dynfact)
  //{
    //i3DView* view3d = dialog->GetApplication ()->Get3DView ();
    //view3d->RefreshFactorySettings (dynfact);
  //}
}

//--------------------------------------------------------------------------

bool CreateJointAction::Do (View* view, wxWindow* component)
{
  iUIManager* uiManager = dialog->GetUIManager ();

  CS::Animation::iBodyBone* bone = dialog->GetCurrentBone ();
  if (!bone)
  {
    return uiManager->Error ("No bone selected!");
  }
  if (bone->GetBoneJoint ())
  {
    return uiManager->Error ("This bone already has a joint!");
  }
  bone->CreateBoneJoint ();
  return true;
}

bool CreateJointAction::IsActive (View* view, wxWindow* component)
{
  CS::Animation::iBodyBone* bone = dialog->GetCurrentBone ();
  if (!bone) return false;
  if (bone->GetBoneJoint ()) return false;
  return true;
}

//--------------------------------------------------------------------------

static Value* FindValueForItem (Value* collectionValue, const char* itemname)
{
  csRef<ValueIterator> it = collectionValue->GetIterator ();
  while (it->HasNext ())
  {
    Value* child = it->NextChild ();
    Value* value = View::FindChild (child, itemname);
    if (value) return value;
  }
  return 0;
}

static Value* GetCategoryForValue (Value* collectionValue, Value* value)
{
  csRef<ValueIterator> it = collectionValue->GetIterator ();
  while (it->HasNext ())
  {
    Value* child = it->NextChild ();
    if (child == value) return value;
    else if (child->IsChild (value)) return child;
  }
  return 0;
}

//--------------------------------------------------------------------------

bool EnableRagdollAction::Do (View* view, wxWindow* component)
{
  iUIManager* uiManager = dialog->GetUIManager ();

  Value* value = view->GetSelectedValue (component);
  if (!value)
  {
    return uiManager->Error ("Please select a valid item!");
  }

  Value* dynfactCollectionValue = dialog->GetApplication ()->Get3DView ()->
    GetModelRepository ()->GetDynfactCollectionValue ();
  Value* categoryValue = GetCategoryForValue (dynfactCollectionValue, value);
  if (!categoryValue || categoryValue == value)
  {
    return uiManager->Error ("Please select a valid item!");
  }

  csString itemname = value->GetStringValue ();
  CorrectFactoryName (itemname);
  iMeshFactoryWrapper* meshFact = dialog->GetEngine ()->FindMeshFactory (itemname);
  CS_ASSERT (meshFact != 0);

  csRef<CS::Mesh::iAnimatedMeshFactory> animFact =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory> (meshFact->GetMeshObjectFactory ());
  if (!animFact)
  {
    return uiManager->Error ("This item is not an animesh!");
  }

  CS::Animation::iSkeletonFactory* skelFact = animFact->GetSkeletonFactory ();
  if (!skelFact)
  {
    return uiManager->Error ("This item has no skeleton!");
  }

  if (dialog->GetBodyManager ()->FindBodySkeleton (itemname))
  {
    return uiManager->Error ("There is already a body skeleton for this item!");
  }

  CS::Animation::iBodySkeleton* bodySkeleton =
    dialog->GetBodyManager ()->CreateBodySkeleton (itemname, skelFact);

#if NEW_PHYSICS
  // @@@
#else
  // Create the ragdoll animation node
  csRef<CS::Animation::iSkeletonRagdollNodeManager> ragdollManager =
    csQueryRegistryOrLoad<CS::Animation::iSkeletonRagdollNodeManager>
    (dialog->GetObjectRegistry (),
     "crystalspace.mesh.animesh.animnode.ragdoll");

  csRef<CS::Animation::iSkeletonRagdollNodeFactory> ragdollNodeFactory =
    ragdollManager->CreateAnimNodeFactory ("ragdoll");
  ragdollNodeFactory->SetBodySkeleton (bodySkeleton);
  i3DView* view3d = dialog->GetApplication ()->Get3DView ();
  ragdollNodeFactory->SetDynamicSystem (view3d->GetDynamicSystem ());

  // Set the ragdoll node as the root of the animation tree
  csRef<CS::Animation::iSkeletonAnimPacketFactory> animPacketFactory =
    animFact->GetSkeletonFactory ()->GetAnimationPacket ();
  animPacketFactory->SetAnimationRoot (ragdollNodeFactory);

  // Populate the skeleton with default colliders and chains
  // TODO: this can probably be made in a separate method, in order to let
  // the user specify the collider type
  bodySkeleton->PopulateDefaultColliders (animFact,
					  CS::Animation::COLLIDER_BOX);
  bodySkeleton->PopulateDefaultBodyChains ();

  // Setup the state of the body chains
  for (csRef<CS::Animation::iBodyChainIterator> it = bodySkeleton->GetBodyChains (); it->HasNext (); )
  {
    CS::Animation::iBodyChain* bodyChain = it->Next ();
    ragdollNodeFactory->AddBodyChain (bodyChain, CS::Animation::STATE_DYNAMIC);
  }
#endif

  dialog->GetMeshView ()->Refresh ();
  dialog->AddDirtyFactory (dialog->GetCurrentFactory ());

  return true;
}

bool EnableRagdollAction::IsActive (View* view, wxWindow* component)
{
  Value* value = view->GetSelectedValue (component);
  if (!value) return false;

  Value* dynfactCollectionValue = dialog->GetApplication ()->Get3DView ()->
    GetModelRepository ()->GetDynfactCollectionValue ();
  Value* categoryValue = GetCategoryForValue (dynfactCollectionValue, value);
  if (!categoryValue || categoryValue == value)
    return false;

  csString itemname = value->GetStringValue ();
  CorrectFactoryName (itemname);
  iMeshFactoryWrapper* meshFact = dialog->GetEngine ()->FindMeshFactory (itemname);
  CS_ASSERT (meshFact != 0);

  csRef<CS::Mesh::iAnimatedMeshFactory> animFact =
    scfQueryInterface<CS::Mesh::iAnimatedMeshFactory> (meshFact->GetMeshObjectFactory ());
  if (!animFact)
    return false;

  CS::Animation::iSkeletonFactory* skelFact = animFact->GetSkeletonFactory ();
  if (!skelFact)
    return false;
  if (dialog->GetBodyManager ()->FindBodySkeleton (itemname))
    return false;
  return true;
}

//--------------------------------------------------------------------------

bool EditCategoryAction::Do (View* view, wxWindow* component)
{
  iUIManager* uiManager = dialog->GetUIManager ();

  Value* value = view->GetSelectedValue (component);
  if (!value)
  {
    return uiManager->Error ("Please select a valid item!");
  }

  Value* dynfactCollectionValue = dialog->GetApplication ()->Get3DView ()->
    GetModelRepository ()->GetDynfactCollectionValue ();
  Value* categoryValue = GetCategoryForValue (dynfactCollectionValue, value);
  if (!categoryValue || categoryValue == value)
  {
    return uiManager->Error ("Please select a valid item!");
  }

  csRef<iUIDialog> dia = uiManager->CreateDialog (dialog, "Edit category");
  dia->AddRow ();
  dia->AddLabel ("Category:");
  dia->AddText ("category");

  csString oldCategory = categoryValue->GetStringValue ();
  dia->SetText ("category", categoryValue->GetStringValue ());
  if (dia->Show (0))
  {
    const DialogResult& rc = dia->GetFieldContents ();
    csString newCategory = rc.Get ("category", oldCategory);
    if (newCategory.IsEmpty ())
    {
      return uiManager->Error ("The category cannot be empty!");
    }
    if (newCategory != oldCategory)
    {
      csString itemname = value->GetStringValue ();
      CorrectFactoryName (itemname);
      i3DView* view3d = dialog->GetApplication ()->Get3DView ();
      view3d->ChangeCategory (newCategory, itemname);
      iPcDynamicWorld* dynworld = view3d->GetDynamicWorld ();
      iDynamicFactory* fact = dynworld->FindFactory (itemname);
      CS_ASSERT (fact != 0);
      fact->SetAttribute ("category", newCategory);
      dynfactCollectionValue->Refresh ();
      Value* itemValue = FindValueForItem (dynfactCollectionValue, itemname);
      if (itemValue)
	view->SetSelectedValue (component, itemValue);
    }
  }

  return true;
}

bool EditCategoryAction::IsActive (View* view, wxWindow* component)
{
  Value* value = view->GetSelectedValue (component);
  if (!value) return false;

  Value* dynfactCollectionValue = dialog->GetApplication ()->Get3DView ()->
    GetModelRepository ()->GetDynfactCollectionValue ();
  Value* categoryValue = GetCategoryForValue (dynfactCollectionValue, value);
  if (!categoryValue || categoryValue == value)
    return false;

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
  bool doBoneColliders;

public:
  BestFitAction (DynfactDialog* dialog, csColliderGeometryType type, bool doBoneColliders = false) :
    dialog (dialog), type (type), doBoneColliders (doBoneColliders) { }
  virtual ~BestFitAction () { }
  virtual const char* GetName () const { return "Fit"; }
  virtual bool Do (View* view, wxWindow* component)
  {
    iDynamicFactory* fact = dialog->GetCurrentFactory ();
    if (!fact) return false;
    if (doBoneColliders)
    {
      CS::Animation::iBodyBone* bone = dialog->GetCurrentBone ();
      if (!bone) return false;
      dialog->FitCollider (bone->GetAnimeshBone (), type);
    }
    else
    {
      dialog->FitCollider (fact, type);
    }
    dialog->AddDirtyFactory (fact);
    return true;
  }

  virtual bool IsActive (View* view, wxWindow* component)
  {
    iDynamicFactory* fact = dialog->GetCurrentFactory ();
    if (!fact) return false;
    if (doBoneColliders)
    {
      CS::Animation::iBodyBone* bone = dialog->GetCurrentBone ();
      if (!bone) return false;
    }
    return true;
  }
};

//--------------------------------------------------------------------------

void MultiNewChildAction::AddFactory (const char* name, const char* category,
    csArray<iObject*>& resources)
{
  i3DView* view3d = dynfact->GetApplication ()->Get3DView ();
  iPcDynamicWorld* dynworld = view3d->GetDynamicWorld ();
  if (!dynworld->FindFactory (name))
  {
    iDynamicFactory* fact = dynworld->AddFactory (name, 1.0f, 1.0f);
    fact->SetAttribute ("category", category);
    view3d->RefreshFactorySettings (fact);
    if (dynworld->IsPhysicsEnabled ())
    {
      const csBox3& bbox = fact->GetBBox ();
      csVector3 c = bbox.GetCenter ();
      csVector3 s = bbox.GetSize ();
      fact->AddRigidBox (c, s, 1.0f);
    }
    else
    {
      fact->SetColliderEnabled (true);
    }
    resources.Push (fact->QueryObject ());
  }
}

bool MultiNewChildAction::IsActive (View* view, wxWindow* component)
{
  Value* value = view->GetSelectedValue (component);
  if (!value) return false;

  Value* dynfactCollectionValue = dynfact->GetApplication ()->Get3DView ()->
    GetModelRepository ()->GetDynfactCollectionValue ();
  Value* categoryValue = GetCategoryForValue (dynfactCollectionValue, value);
  if (!categoryValue || categoryValue != value)
    return false;

  return true;
}

bool MultiNewChildAction::Do (View* view, wxWindow* component)
{
  Value* value = view->GetSelectedValue (component);
  if (!value) return false;

  dialog->Clear ();
  if (!dialog->Show (0))
    return false;

  i3DView* view3d = dynfact->GetApplication ()->Get3DView ();
  Value* dynfactCollectionValue = view3d->GetModelRepository ()->GetDynfactCollectionValue ();
  Value* categoryValue = GetCategoryForValue (dynfactCollectionValue, value);
  csString category = categoryValue->GetStringValue ();

  iPcDynamicWorld* dynworld = view3d->GetDynamicWorld ();
  const DialogValues& values = dialog->GetFieldValues ();

  csArray<iObject*> resources;
  DialogValues::ConstIterator it = values.GetIterator ("name");
  while (it.HasNext ())
  {
    Value* row = it.Next ();
    csString name = row->GetChild (0)->GetStringValue ();
    AddFactory (name, category, resources);
  }
  dynfact->GetApplication ()->RegisterModification (resources);
  categoryValue->Refresh ();
  dynfactCollectionValue->FireValueChanged ();
  return true;
}

//--------------------------------------------------------------------------

bool NewInvisibleChildAction::Do (View* view, wxWindow* component)
{
  csRef<iUIDialog> dia = dialog->GetUIManager ()->CreateDialog (component, "Factory name");
  dia->AddRow ();
  dia->AddLabel ("Name:");
  dia->AddText ("name");
  dia->AddRow ();
  dia->AddLabel ("Min corner:");
  dia->AddText ("minx");
  dia->AddText ("miny");
  dia->AddText ("minz");
  dia->AddRow ();
  dia->AddLabel ("Max corner:");
  dia->AddText ("maxx");
  dia->AddText ("maxy");
  dia->AddText ("maxz");
  bool d = DoDialog (view, component, dia);
  return d;
}

//--------------------------------------------------------------------------

bool NewLightChildAction::Do (View* view, wxWindow* component)
{
  LightDialog* dia = new LightDialog (component, dialog);
  iLightFactory* newFactory = dia->Show (0);
  if (newFactory)
  {
    size_t idx = csArrayItemNotFound;
    DialogResult dialogResult;
    dialogResult.Put ("name", newFactory->QueryObject ()->GetName ());
    dialogResult.Put ("light", "true");
    Value* value = collection->NewValue (idx, view->GetSelectedValue (component),
      dialogResult);
    if (!value)
    {
      dialog->GetEngine ()->RemoveObject (newFactory);
      delete dia;
      return false;
    }
    view->SetSelectedValue (component, value);
    dialog->GetApplication ()->RegisterModification (newFactory->QueryObject ());
  }

  delete dia;
  return true;
}

//--------------------------------------------------------------------------

bool EditLightChildAction::Do (View* view, wxWindow* component)
{
  iDynamicFactory* fact = dialog->GetCurrentFactory ();
  if (!fact) return false;
  iLightFactory* lf = dialog->GetEngine ()->FindLightFactory (fact->GetName ());
  if (!lf) return false;

  LightDialog* dia = new LightDialog (component, dialog);
  if (dia->Show (lf))
  {
    dialog->GetApplication ()->RegisterModification (lf->QueryObject ());
    dialog->AddDirtyFactory (fact);
  }
  delete dia;
  return true;
}

bool EditLightChildAction::IsActive (View* view, wxWindow* component)
{
  Value* value = view->GetSelectedValue (component);
  if (value == 0) return false;
  iDynamicFactory* fact = dialog->GetCurrentFactory ();
  if (!fact) return false;
  iLightFactory* lf = dialog->GetEngine ()->FindLightFactory (fact->GetName ());
  if (!lf) return false;
  return true;
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
    csRef<iString> dimS = dialog->GetUIManager ()->AskDialog ("Thickness of box sides", "Thickness:");
    if (!dimS) return false;
    if (dimS->IsEmpty ()) return false;
    float dim;
    csScanStr (dimS->GetData (), "%f", &dim);
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
    dialog->AddDirtyFactory (fact);
    return true;
  }
};

//--------------------------------------------------------------------------

void DynfactDialog::AddDirtyFactory (iDynamicFactory* fact)
{
  if (fact)
  {
    if (!dirtyFactories.Contains (fact))
    {
      dirtyFactories.Add (fact);
      dirtyFactoriesWeakArray.Push (fact);
      app->RegisterModification (fact->QueryObject ());
    }
  }
}

void DynfactDialog::OnOkButton (wxCommandEvent& event)
{
  i3DView* view3d = GetApplication ()->Get3DView ();
  iPcDynamicWorld* dynworld = view3d->GetDynamicWorld ();
  for (size_t i = 0 ; i < dirtyFactoriesWeakArray.GetSize () ; i++)
  {
    iDynamicFactory* fact = dirtyFactoriesWeakArray[i];
    if (fact)
    {
      printf ("Updating factory '%s'\n", fact->GetName ());
      dynworld->UpdateObjects (fact);
    }
  }
  dirtyFactories.DeleteAll ();
  dirtyFactoriesWeakArray.DeleteAll ();
  view3d->GetModelRepository ()->GetObjectsValue ()->Refresh ();
  view3d->GetModelRepository ()->GetDynfactCollectionValue ()->Refresh ();
  EndModal (TRUE);
}

void DynfactDialog::OnShowBodies (wxCommandEvent& event)
{
  wxCheckBox* check = XRCCTRL (*this, "showBodiesCheck", wxCheckBox);
  meshView->ShowBodies (check->GetValue ());
}

void DynfactDialog::OnShowBones (wxCommandEvent& event)
{
  wxCheckBox* check = XRCCTRL (*this, "showBonesCheck", wxCheckBox);
  meshView->ShowBones (check->GetValue ());
}

void DynfactDialog::OnShowOrigin (wxCommandEvent& event)
{
  wxCheckBox* check = XRCCTRL (*this, "showOriginCheck", wxCheckBox);
  meshView->ShowOrigin (check->GetValue ());
}

void DynfactDialog::OnShowJoints (wxCommandEvent& event)
{
  wxCheckBox* check = XRCCTRL (*this, "showJointsCheck", wxCheckBox);
  meshView->ShowJoints (check->GetValue ());
}

void DynfactDialog::Show ()
{
  app->Get3DView ()->GetModelRepository ()->GetDynfactCollectionValue ()->Refresh ();

  csRef<iEventTimer> timer = csEventTimer::GetStandardTimer (object_reg);
  timer->AddTimerEvent (timerOp, 25);

  ShowModal ();

  timer->RemoveTimerEvent (timerOp);
  meshView->SetMesh (0);
}

void DynfactDialog::Tick ()
{
  meshView->RotateMesh (vc->GetElapsedSeconds ());
}

CS::Animation::iSkeletonFactory* DynfactDialog::GetSkeletonFactory (const char* factName)
{
  iMeshFactoryWrapper* meshFact = engine->FindMeshFactory (factName);
  if (!meshFact) return 0;

  csRef<CS::Mesh::iAnimatedMeshFactory> animFact = scfQueryInterface<CS::Mesh::iAnimatedMeshFactory> (meshFact->GetMeshObjectFactory ());
  if (!animFact) return 0;

  CS::Animation::iSkeletonFactory* skelFact = animFact->GetSkeletonFactory ();
  return skelFact;
}

CS::Animation::iBodyBone* DynfactDialog::GetCurrentBone ()
{
  if (!factorySelectedValue) return 0;
  csString selectedFactory = factorySelectedValue->GetStringValue ();
  if (selectedFactory.IsEmpty ()) return 0;
  CorrectFactoryName (selectedFactory);
  CS::Animation::iBodySkeleton* bodySkel = bodyManager
      ->FindBodySkeleton (selectedFactory);
  if (!bodySkel) return 0;
  Value* nameValue = bonesSelectedValue->GetChildByName ("name");
  csString selectedBone = nameValue->GetStringValue ();
  if (selectedBone.IsEmpty ()) return 0;
  return bodySkel->FindBodyBone (selectedBone);
}

CS::Animation::iBodySkeleton* DynfactDialog::GetCurrentBodySkeleton ()
{
  if (!factorySelectedValue) return 0;
  csString selectedFactory = factorySelectedValue->GetStringValue ();
  if (selectedFactory.IsEmpty ()) return 0;
  CorrectFactoryName (selectedFactory);
  return bodyManager->FindBodySkeleton (selectedFactory);
}

iDynamicFactory* DynfactDialog::GetCurrentFactory ()
{
  if (!factorySelectedValue) return 0;
  csString selectedFactory = factorySelectedValue->GetStringValue ();
  if (selectedFactory.IsEmpty ()) return 0;
  CorrectFactoryName (selectedFactory);
  iPcDynamicWorld* dynworld = app->Get3DView ()->GetDynamicWorld ();
  iDynamicFactory* dynfact = dynworld->FindFactory (selectedFactory);
  return dynfact;
}

long DynfactDialog::GetSelectedCollider ()
{
  wxListCtrl* list = XRCCTRL (*this, "colliders_List", wxListCtrl);
  return ListCtrlTools::GetFirstSelectedRow (list);
}

long DynfactDialog::GetSelectedBoneCollider ()
{
  wxListCtrl* list = XRCCTRL (*this, "boneColliders_List", wxListCtrl);
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

csString DynfactDialog::GetSelectedBone ()
{
  wxListCtrl* list = XRCCTRL (*this, "bones_List", wxListCtrl);
  int row = ListCtrlTools::GetFirstSelectedRow (list);
  if (row < 0) return "";
  return ListCtrlTools::ReadRow (list, row)[0];
}

void DynfactDialog::FitCollider (Value* colSelValue, const csBox3& bbox,
    csColliderGeometryType type)
{
  csVector3 c = bbox.GetCenter ();
  csVector3 s = bbox.GetSize ();
  switch (type)
  {
    case BOX_COLLIDER_GEOMETRY:
      {
	colSelValue->GetChildByName ("offsetX")->SetFloatValue (c.x);
	colSelValue->GetChildByName ("offsetY")->SetFloatValue (c.y);
	colSelValue->GetChildByName ("offsetZ")->SetFloatValue (c.z);
	colSelValue->GetChildByName ("sizeX")->SetFloatValue (s.x);
	colSelValue->GetChildByName ("sizeY")->SetFloatValue (s.y);
	colSelValue->GetChildByName ("sizeZ")->SetFloatValue (s.z);
	break;
      }
    case SPHERE_COLLIDER_GEOMETRY:
      {
	float radius = s.x;
	if (s.y > radius) radius = s.y;
	if (s.z > radius) radius = s.z;
	radius *= 0.5f;
	colSelValue->GetChildByName ("offsetX")->SetFloatValue (c.x);
	colSelValue->GetChildByName ("offsetY")->SetFloatValue (c.y);
	colSelValue->GetChildByName ("offsetZ")->SetFloatValue (c.z);
	colSelValue->GetChildByName ("radius")->SetFloatValue (radius);
	break;
      }
    case CAPSULE_COLLIDER_GEOMETRY:
    case CYLINDER_COLLIDER_GEOMETRY:
      {
	float radius = s.x;
	if (s.z > radius) radius = s.z;
	radius *= 0.5f;
	float length = s.y;
	colSelValue->GetChildByName ("offsetX")->SetFloatValue (c.x);
	colSelValue->GetChildByName ("offsetY")->SetFloatValue (c.y);
	colSelValue->GetChildByName ("offsetZ")->SetFloatValue (c.z);
	colSelValue->GetChildByName ("radius")->SetFloatValue (radius);
	colSelValue->GetChildByName ("length")->SetFloatValue (length);
	break;
      }
    default: return;
  }
}

void DynfactDialog::FitCollider (iDynamicFactory* fact, csColliderGeometryType type)
{
  FitCollider (colliderSelectedValue, fact->GetBBox (), type);
}

void DynfactDialog::FitCollider (CS::Animation::BoneID id, csColliderGeometryType type)
{
  iDynamicFactory* fact = GetCurrentFactory ();
  if (!fact) return;
  csString factName = fact->GetName ();
  iMeshFactoryWrapper* meshFact = engine->FindMeshFactory (factName);
  if (!meshFact) { printf ("Can't find mesh factory '%s'!", factName.GetData ()); return; }
  csRef<CS::Mesh::iAnimatedMeshFactory> animeshFactory = scfQueryInterface<CS::Mesh::iAnimatedMeshFactory>
      (meshFact->GetMeshObjectFactory ());
  FitCollider (bonesColliderSelectedValue, animeshFactory->GetBoneBoundingBox (id), type);
  AddDirtyFactory (fact);
}

void DynfactDialog::SetupDialogs ()
{
  // The dialog for editing new factories.
  factoryDialog = app->GetUI ()->CreateDialog (this, "Factory name");
  factoryDialog->AddRow (1);
  factoryDialog->AddLabel ("Name:");
  factoryDialog->AddList ("name", NEWREF(Value,new MeshCollectionValue (engine,
	  app->Get3DView ()->GetDynamicWorld ())), 0, true, 300,
      "Name", "name");

  // The dialog for editing new attributes.
  attributeDialog = app->GetUI ()->CreateDialog (this, "Attribute");
  attributeDialog->AddRow ();
  attributeDialog->AddLabel ("Name:");
  attributeDialog->AddText ("name");
  attributeDialog->AddRow ();
  attributeDialog->AddLabel ("Value:");
  attributeDialog->AddText ("value");

  // The dialog for selecting a bone.
  selectBoneDialog = app->GetUI ()->CreateDialog (this, "Select Bone");
  selectBoneDialog->AddRow ();
  selectBoneDialog->AddList ("name", NEWREF(Value,new FactoryBoneCollectionValue(this)), 0, false, 200,
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
  view.Bind (colSelValue->GetChildByName ("type"), C3 ("type_", suffix, "ColliderChoice"));
  view.Bind (colSelValue, C3 ("box_", suffix, "ColliderPanel"));
  view.Bind (colSelValue, C3 ("sphere_", suffix, "ColliderPanel"));
  view.Bind (colSelValue, C3 ("cylinder_", suffix, "ColliderPanel"));
  view.Bind (colSelValue, C3 ("capsule_", suffix, "ColliderPanel"));
  view.Bind (colSelValue, C3 ("mesh_", suffix, "ColliderPanel"));
  view.Bind (colSelValue, C3 ("convexMesh_", suffix, "ColliderPanel"));

  // Bind calculated value for the box collider so that there are also min/max
  // controls in addition to offset/size.
  view.Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetX"),
	  colSelValue->GetChildByName ("sizeX"), false)), C3 ("minX_", suffix, "BoxText"));
  view.Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetY"),
	  colSelValue->GetChildByName ("sizeY"), false)), C3 ("minY_", suffix, "BoxText"));
  view.Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetZ"),
	  colSelValue->GetChildByName ("sizeZ"), false)), C3 ("minZ_", suffix, "BoxText"));
  view.Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetX"),
	  colSelValue->GetChildByName ("sizeX"), true)), C3 ("maxX_", suffix, "BoxText"));
  view.Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetY"),
	  colSelValue->GetChildByName ("sizeY"), true)), C3 ("maxY_", suffix, "BoxText"));
  view.Bind (NEWREF(Value, new Offset2MinMaxValue (
	  colSelValue->GetChildByName ("offsetZ"),
	  colSelValue->GetChildByName ("sizeZ"), true)), C3 ("maxZ_", suffix, "BoxText"));
}

void DynfactDialog::SetupJointsEditor (Value* jntValue, const char* suffix)
{
  view.BindEnabled (view.Not (jntValue->GetChildByName ("xLockTrans")), C3 ("xMinTrans_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("xLockTrans")), C3 ("xMaxTrans_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("yLockTrans")), C3 ("yMinTrans_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("yLockTrans")), C3 ("yMaxTrans_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("zLockTrans")), C3 ("zMinTrans_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("zLockTrans")), C3 ("zMaxTrans_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("xLockRot")), C3 ("xMinRot_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("xLockRot")), C3 ("xMaxRot_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("yLockRot")), C3 ("yMinRot_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("yLockRot")), C3 ("yMaxRot_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("zLockRot")), C3 ("zMinRot_", suffix, "Text"));
  view.BindEnabled (view.Not (jntValue->GetChildByName ("zLockRot")), C3 ("zMaxRot_", suffix, "Text"));
}

void DynfactDialog::SetupActions ()
{
  Value* colliders = dynfactValue->GetChildByName ("colliders");
  Value* pivots = dynfactValue->GetChildByName ("pivots");
  Value* joints = dynfactValue->GetChildByName ("joints");
  Value* bones = dynfactValue->GetChildByName ("bones");
  Value* boneColliders = boneValue->GetChildByName ("boneColliders");
  Value* dynfactCollectionValue = app->Get3DView ()->GetModelRepository ()->GetDynfactCollectionValue ();

  wxListCtrl* bonesList = XRCCTRL (*this, "bones_List", wxListCtrl);
  wxListCtrl* jointsList = XRCCTRL (*this, "joints_List", wxListCtrl);
  wxListCtrl* pivotsList = XRCCTRL (*this, "pivots_List", wxListCtrl);
  wxListCtrl* colliderList = XRCCTRL (*this, "colliders_List", wxListCtrl);
  wxListCtrl* bonesColliderList = XRCCTRL (*this, "boneColliders_List", wxListCtrl);
  wxTreeCtrl* factoryTree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);

  // The actions.
  view.AddAction (colliderList, NEWREF(Action, new NewChildAction (colliders)));
  view.AddAction (colliderList, NEWREF(Action, new ContainerBoxAction (this, colliders)));
  view.AddAction (colliderList, NEWREF(Action, new DeleteChildAction (colliders)));
  view.AddAction (bonesColliderList, NEWREF(Action, new NewChildAction (boneColliders)));
  view.AddAction (pivotsList, NEWREF(Action, new NewChildAction (pivots)));
  view.AddAction (pivotsList, NEWREF(Action, new DeleteChildAction (pivots)));
  view.AddAction (jointsList, NEWREF(Action, new NewChildAction (joints)));
  view.AddAction (jointsList, NEWREF(Action, new DeleteChildAction (joints)));
  view.AddAction (bonesList, NEWREF(Action, new NewChildDialogAction (bones, selectBoneDialog)));
  view.AddAction (bonesList, NEWREF(Action, new CreateJointAction (this)));
  view.AddAction (factoryTree, NEWREF(Action, new MultiNewChildAction (this, factoryDialog)));
  view.AddAction (factoryTree, NEWREF(Action, new NewInvisibleChildAction (this, dynfactCollectionValue)));
  view.AddAction (factoryTree, NEWREF(Action, new NewLightChildAction (this, dynfactCollectionValue)));
  view.AddAction (factoryTree, NEWREF(Action, new EditLightChildAction (this, dynfactCollectionValue)));
  view.AddAction (factoryTree, NEWREF(Action, new DeleteChildAction (dynfactCollectionValue)));
  view.AddAction (factoryTree, NEWREF(Action, new EditCategoryAction (this)));
  view.AddAction (factoryTree, NEWREF(Action, new EnableRagdollAction (this)));
  Value* attributes = dynfactValue->GetChildByName ("attributes");
  view.AddAction ("attributes_List", NEWREF(Action, new NewChildDialogAction (attributes, attributeDialog)));
  view.AddAction ("attributes_List", NEWREF(Action, new DeleteChildAction (attributes)));

  view.AddAction ("boxFitOffsetButton", NEWREF(Action, new BestFitAction(this, BOX_COLLIDER_GEOMETRY)));
  view.AddAction ("sphereFitOffsetButton", NEWREF(Action, new BestFitAction(this, SPHERE_COLLIDER_GEOMETRY)));
  view.AddAction ("cylinderFitOffsetButton", NEWREF(Action, new BestFitAction(this, CYLINDER_COLLIDER_GEOMETRY)));
  view.AddAction ("capsuleFitOffsetButton", NEWREF(Action, new BestFitAction(this, CAPSULE_COLLIDER_GEOMETRY)));

  view.AddAction ("boneBoxFitOffsetButton", NEWREF(Action, new BestFitAction(this, BOX_COLLIDER_GEOMETRY, true)));
  view.AddAction ("boneSphereFitOffsetButton", NEWREF(Action, new BestFitAction(this, SPHERE_COLLIDER_GEOMETRY, true)));
  view.AddAction ("boneCylinderFitOffsetButton", NEWREF(Action, new BestFitAction(this, CYLINDER_COLLIDER_GEOMETRY, true)));
  view.AddAction ("boneCapsuleFitOffsetButton", NEWREF(Action, new BestFitAction(this, CAPSULE_COLLIDER_GEOMETRY, true)));
}

void DynfactDialog::SetupListHeadings ()
{
  // Setup the lists.
  view.DefineHeading ("colliders_List", "Type,Mass,x,y,z", "type,mass,offsetX,offsetY,offsetZ");
  view.DefineHeading ("boneColliders_List", "Type,Mass,x,y,z", "type,mass,offsetX,offsetY,offsetZ");
  view.DefineHeading ("pivots_List", "x,y,z", "pivotX,pivotY,pivotZ");
  view.DefineHeading ("joints_List", "x,y,z,tx,ty,tz,rx,ry,rz", "jointPosX,jointPosY,jointPosZ,xLockTrans,yLockTrans,zLockTrans,xLockRot,yLockRot,zLockRot");
  view.DefineHeading ("attributes_List", "Name,Value", "attrName,attrValue");
  view.DefineHeading ("bones_List", "Name", "name");
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

void DynfactDialog::UpdateRagdoll ()
{
  // Find the ragdoll factory
  CS::Animation::iBodySkeleton* bodySkeleton = GetCurrentBodySkeleton ();
  csRef<CS::Animation::iSkeletonRagdollNodeFactory> ragdollNodeFactory =
    scfQueryInterfaceSafe<CS::Animation::iSkeletonRagdollNodeFactory>
    (bodySkeleton->GetSkeletonFactory ()->GetAnimationPacket ()
     ->GetAnimationRoot ()->FindNode ("ragdoll"));
  if (!ragdollNodeFactory) return;

  // Clear the state of all chains
  for (csRef<CS::Animation::iBodyChainIterator> it = bodySkeleton->GetBodyChains (); it->HasNext (); )
  {
    CS::Animation::iBodyChain* bodyChain = it->Next ();
    ragdollNodeFactory->RemoveBodyChain (bodyChain);
  }

  // Populate with new body chains
  bodySkeleton->ClearBodyChains ();
  bodySkeleton->PopulateDefaultBodyChains ();

  // Reset the state of all chains
  for (csRef<CS::Animation::iBodyChainIterator> it = bodySkeleton->GetBodyChains (); it->HasNext (); )
  {
    CS::Animation::iBodyChain* bodyChain = it->Next ();
    ragdollNodeFactory->AddBodyChain (bodyChain, CS::Animation::STATE_DYNAMIC);
  }

  // TODO: all the instances of animeshes currently in use of the body skeleton should be updated too,
  // otherwise their ragdoll node will be in an inconsistent state.
}

DynfactDialog::DynfactDialog (iBase* parent) :
  scfImplementationType (this, parent), view (this)
{
}

bool DynfactDialog::Initialize (iObjectRegistry* object_reg)
{
  DynfactDialog::object_reg = object_reg;

  bodyManager = csQueryRegistryOrLoad<CS::Animation::iBodyManager> (object_reg,
      "crystalspace.mesh.animesh.body");
  if (!bodyManager)
  {
    printf ("Can't find body manager!\n");
    fflush (stdout);
    return false;
  }
  pl = csQueryRegistry<iCelPlLayer> (object_reg);
  if (!pl)
  {
    printf ("Can't find physical layer!\n");
    fflush (stdout);
    return false;
  }
  engine = csQueryRegistry<iEngine> (object_reg);
  vc = csQueryRegistry<iVirtualClock> (object_reg);

  ID_Show = pl->FetchStringID ("Show");

  return true;
}

void DynfactDialog::SetApplication (iAresEditor* app)
{
  DynfactDialog::app = app;
}

void DynfactDialog::SetParent (wxWindow* parent)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("DynfactDialog"));

  // The mesh panel.
  wxPanel* panel = XRCCTRL (*this, "meshPanel", wxPanel);
  meshView = new DynfactMeshView (this, object_reg, panel);

  wxCheckBox* check;
  check = XRCCTRL (*this, "showJointsCheck", wxCheckBox);
  check->SetValue (true);
  check = XRCCTRL (*this, "showBonesCheck", wxCheckBox);
  check->SetValue (true);
  check = XRCCTRL (*this, "showBodiesCheck", wxCheckBox);
  check->SetValue (true);
  check = XRCCTRL (*this, "showOriginCheck", wxCheckBox);
  check->SetValue (true);

  SetupDialogs ();

  // Setup the dynamic factory tree.
  Value* dynfactCollectionValue = app->Get3DView ()->GetModelRepository ()->GetDynfactCollectionValue ();
  view.Bind (dynfactCollectionValue, "factoryTree");
  wxTreeCtrl* factoryTree = XRCCTRL (*this, "factoryTree", wxTreeCtrl);
  factorySelectedValue.AttachNew (new TreeSelectedValue (factoryTree, dynfactCollectionValue, VALUE_COLLECTION));

  SetupListHeadings ();

  // Setup the composite representing the dynamic factory that is selected.
  dynfactValue.AttachNew (new DynfactValue (this));
  view.Bind (dynfactValue, this);

  // Setup the composite representing the bone that is selected.
  boneValue.AttachNew (new BoneValue (this));
  view.Bind (boneValue, "bonesPanel");

  SetupSelectedValues ();

  // Bind the selected collider value to the mesh view. This value is not actually
  // used by the mesh view but this binding only serves as a signal for the mesh
  // view to update itself.
  view.Bind (colliderSelectedValue, meshView);
  // Also do this for the selected pivot value.
  view.Bind (pivotsSelectedValue, meshView);
  // Also do this for the selected joint value.
  view.Bind (jointsSelectedValue, meshView);
  // Also do this for the selected bone value.
  view.Bind (bonesSelectedValue, meshView);
  // Also do this for the selected bone collider value.
  view.Bind (bonesColliderSelectedValue, meshView);

  // Connect the selected value from the category tree to the dynamic
  // factory value so that the two radius values and the collider list
  // gets refreshed in case the current dynfact changes. We connect
  // with 'dochildren' equal to true to make sure the children get notified
  // as well (i.e. the list for example).
  view.Signal (factorySelectedValue, dynfactValue, true);
  // Also connect it to the selected values so that a new mesh is rendered
  // when the current dynfact changes.
  view.Signal (factorySelectedValue, colliderSelectedValue);
  view.Signal (factorySelectedValue, pivotsSelectedValue);
  view.Signal (factorySelectedValue, jointsSelectedValue);
  view.Signal (factorySelectedValue, bonesSelectedValue);

  // When another bone is selected we want to update the selection of the
  // bone collider too and we also want to update the joint.
  view.Signal (bonesSelectedValue, boneValue, true);
  view.Signal (bonesSelectedValue, bonesColliderSelectedValue);

  // Setup the collider editors.
  SetupColliderEditor (colliderSelectedValue, "");
  SetupColliderEditor (bonesColliderSelectedValue, "Bone");

  view.Bind (pivotsSelectedValue, "pivotPosition_Panel");
  view.Bind (jointsSelectedValue, "joints_Panel");

  // Bind some values to the enabled/disabled state of several components.
  view.BindEnabled (pivotsSelectedValue->GetSelectedState (), "pivotPosition_Panel");
  view.BindEnabled (jointsSelectedValue->GetSelectedState (), "joints_Panel");
  view.BindEnabled (bonesSelectedValue->GetSelectedState (), "boneJoint_Panel");
  SetupJointsEditor (jointsSelectedValue, "");
  // @@@ Bug: doesn't work for the first time when a bone is selected.
  SetupJointsEditor (boneValue->GetChildByName ("boneJoint"), "Bone");

  SetupActions ();

  timerOp.AttachNew (new RotMeshTimer (this));

  Layout ();
  Fit ();
}

DynfactDialog::~DynfactDialog ()
{
  delete meshView;
}


