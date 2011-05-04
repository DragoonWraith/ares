/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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

#ifndef __ARES_DYNWORLD_IMP_H__
#define __ARES_DYNWORLD_IMP_H__

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "csutil/set.h"
#include "csutil/eventhandlers.h"
#include "csutil/refarr.h"
#include "csutil/parray.h"
#include "iutil/comp.h"
#include "iutil/virtclk.h"

#include "csgeom/aabbtree.h"
#include "csgeom/math3d.h"

#include "ivaria/dynamics.h"
#include "imap/services.h"

#include "iengine/engine.h"
#include "iengine/movable.h"
#include "iengine/imposter.h"

#include "include/idynworld.h"

CS_PLUGIN_NAMESPACE_BEGIN(DynWorld)
{

class DynamicWorld;

class MeshCacheFactory
{
public:
  csArray<iMeshWrapper*> meshes;
};

class MeshCache
{
private:
  csHash<MeshCacheFactory*,csString> meshFactories;

public:
  MeshCache ();
  ~MeshCache ();

  iMeshWrapper* AddMesh (iEngine* engine, iMeshFactoryWrapper* factory,
      iSector* sector, const csReversibleTransform& trans);
  void RemoveMesh (iMeshWrapper* mesh);
  void RemoveMeshes ();
};

class DOCollider
{
protected:
  csVector3 offset;
  float mass;

public:
  DOCollider (const csVector3& offset, float mass) : offset (offset), mass (mass) { }
  virtual ~DOCollider () { }

  float GetMass () const { return mass; }

  virtual csRef<iRigidBody> Create (iDynamicSystem* dynSys, iMeshWrapper* mesh,
      const csReversibleTransform& trans, iRigidBody* sharedBody)
  {
    // Create a body and attach the mesh.
    if (sharedBody)
    {
      return sharedBody;
    }
    else
    {
      csRef<iRigidBody> body = dynSys->CreateBody ();

      csRef<CS::Physics::Bullet::iRigidBody> csBody = scfQueryInterface<CS::Physics::Bullet::iRigidBody> (body);
      csBody->SetLinearDampener (0.0f);
      csBody->SetRollingDampener (0.0f);

      body->AdjustTotalMass (mass);
      body->SetTransform (trans);
      body->AttachMesh (mesh);
      return body;
    }
  }
};

class DOColliderMesh : public DOCollider
{
public:
  DOColliderMesh (const csVector3& offset, float mass) :
    DOCollider (offset, mass) { }
  virtual ~DOColliderMesh () { }
  virtual csRef<iRigidBody> Create (iDynamicSystem* dynSys, iMeshWrapper* mesh,
      const csReversibleTransform& trans, iRigidBody* sharedBody)
  {
    csRef<iRigidBody> body = DOCollider::Create (dynSys, mesh, trans, sharedBody);
    const csMatrix3 tm;
    csOrthoTransform t (tm, offset);
    body->AttachColliderMesh (mesh, t, 10, 1, 0.8f);
    return body;
  }
};

class DOColliderConvexMesh : public DOCollider
{
public:
  DOColliderConvexMesh (const csVector3& offset, float mass) :
    DOCollider (offset, mass) { }
  virtual ~DOColliderConvexMesh () { }
  virtual csRef<iRigidBody> Create (iDynamicSystem* dynSys, iMeshWrapper* mesh,
      const csReversibleTransform& trans, iRigidBody* sharedBody)
  {
    csRef<iRigidBody> body = DOCollider::Create (dynSys, mesh, trans, sharedBody);
    const csMatrix3 tm;
    csOrthoTransform t (tm, offset);
    body->AttachColliderConvexMesh (mesh, t, 10, 1, 0.8f);
    return body;
  }
};

class DOColliderBox : public DOCollider
{
private:
  csVector3 size;

public:
  DOColliderBox (const csVector3& size, const csVector3& offset, float mass) :
    DOCollider (offset, mass), size (size) { }
  virtual ~DOColliderBox () { }
  virtual csRef<iRigidBody> Create (iDynamicSystem* dynSys, iMeshWrapper* mesh,
      const csReversibleTransform& trans, iRigidBody* sharedBody)
  {
    csRef<iRigidBody> body = DOCollider::Create (dynSys, mesh, trans, sharedBody);
    const csMatrix3 tm;
    csOrthoTransform t (tm, offset);
    body->AttachColliderBox (size, t, 10, 1, 0);
    return body;
  }
};

class DOColliderCylinder : public DOCollider
{
private:
  float length, radius;

public:
  DOColliderCylinder (const csVector3& offset, float length, float radius, float mass) :
    DOCollider (offset, mass), length (length), radius (radius) { }
  virtual ~DOColliderCylinder () { }
  virtual csRef<iRigidBody> Create (iDynamicSystem* dynSys, iMeshWrapper* mesh,
      const csReversibleTransform& trans, iRigidBody* sharedBody)
  {
    csRef<iRigidBody> body = DOCollider::Create (dynSys, mesh, trans, sharedBody);
    const csMatrix3 tm;
    csOrthoTransform t (tm, offset);
    t.RotateThis (csVector3 (1.0f, 0.0f, 0.0f), HALF_PI);
    body->AttachColliderCylinder (length, radius, t, 10, 1, 0.8f);
    return body;
  }
};

class DOColliderSphere : public DOCollider
{
private:
  float radius;

public:
  DOColliderSphere (const csVector3& offset, float radius, float mass) :
    DOCollider (offset, mass), radius (radius) { }
  virtual ~DOColliderSphere () { }
  virtual csRef<iRigidBody> Create (iDynamicSystem* dynSys, iMeshWrapper* mesh,
      const csReversibleTransform& trans, iRigidBody* sharedBody)
  {
    csRef<iRigidBody> body = DOCollider::Create (dynSys, mesh, trans, sharedBody);
    body->AttachColliderSphere (radius, offset, 100, .5, 0);
    return body;
  }
};

class DynamicFactory : public scfImplementation1<DynamicFactory,
  iDynamicFactory>
{
private:
  csString name;

  DynamicWorld* world;
  iMeshFactoryWrapper* factory;
  csPDelArray<DOCollider> colliders;
  float bsphereRadius;
  csVector3 bsphereCenter;
  csBox3 bbox;
  csBox3 physBbox;
  float maxradiusRelative;

  csRef<iGeometryGenerator> geometryGenerator;
  csRef<iImposterFactory> imposterFactory;
  float imposterradius;
  csHash<csString,csString> attributes;

public:
  DynamicFactory (DynamicWorld* world, const char* name,
      float maxradiusRelative, float imposterradius);
  virtual ~DynamicFactory () { }
  virtual float GetMaximumRadiusRelative () const { return maxradiusRelative; }
  virtual const csBox3& GetBBox () const { return bbox; }
  virtual const csBox3& GetPhysicsBBox () const { return physBbox; }

  virtual void SetGeometryGenerator (iGeometryGenerator* ggen)
  {
    geometryGenerator = ggen;
  }
  virtual iGeometryGenerator* GetGeometryGenerator () const
  {
    return geometryGenerator;
  }
  virtual void SetAttribute (const char* name, const char* value);
  virtual const char* GetAttribute (const char* name) const;

  virtual void AddRigidBox (const csVector3& offset, const csVector3& size,
      float mass);
  virtual void AddRigidSphere (float radius, const csVector3& offset,
      float mass);
  virtual void AddRigidCylinder (float radius, float length,
      const csVector3& offset, float mass);
  virtual void AddRigidMesh (const csVector3& offset, float mass);
  virtual void AddRigidConvexMesh (const csVector3& offset, float mass);

  const csPDelArray<DOCollider>& GetColliders () const { return colliders; }
  iMeshFactoryWrapper* GetMeshFactory () const { return factory; }
  virtual const char* GetName () const { return name; }
  float GetBSphereRadius () const { return bsphereRadius; }
  const csVector3& GetBSphereCenter () const { return bsphereCenter; }
  DynamicWorld* GetWorld () const { return world; }
  iImposterFactory* GetImposterFactory () const { return imposterFactory; }
};

class DynamicObject : public scfImplementation2<DynamicObject,
  iDynamicObject, iMovableListener>
{
private:
  DynamicFactory* factory;
  csRef<iMeshWrapper> mesh;
  mutable csReversibleTransform trans;
  csRef<iRigidBody> body;
  bool is_static;
  bool is_kinematic;
  bool is_hilight;
  bool hilight_installed;
  float fade;

  void InstallHilight (bool hi);
  void Init ();

  mutable csBox3 bbox;
  mutable bool bboxValid;

public:
  DynamicObject ();
  DynamicObject (DynamicFactory* factory, const csReversibleTransform& trans);
  virtual ~DynamicObject ();

  virtual iDynamicFactory* GetFactory () const { return factory; }
  virtual void MakeStatic ();
  virtual void MakeDynamic ();
  virtual void MakeKinematic ();
  virtual void UndoKinematic ();
  virtual bool IsStatic () const { return is_static; }
  virtual void SetHilight (bool hi);
  virtual bool IsHilight () const { return is_hilight; }
  virtual iMeshWrapper* GetMesh () const { return mesh; }
  virtual iRigidBody* GetBody () const { return body; }
  virtual const csBox3& GetBBox () const;
  virtual void RefreshColliders ();
  virtual const csReversibleTransform& GetTransform ();
  virtual void SetTransform (const csReversibleTransform& trans);

  virtual void MovableChanged (iMovable* movable);
  virtual void MovableDestroyed (iMovable* movable);

  void PrepareMesh (DynamicWorld* world);
  void RemoveMesh (DynamicWorld* world);
  void Save (iDocumentNode* node, iSyntaxService* syn);
  bool Load (iDocumentNode* node, iSyntaxService* syn, DynamicWorld* world);

  void SetFade (float f);
  float GetFade () const { return fade; }

  // Return true if a body is part of this dynamic object.
  bool HasBody (iRigidBody* body);
};

struct DynamicObjectExtraData
{
  void LeafAddObject (DynamicObject* object)
  {
  }
  void LeafUpdateObjects (DynamicObject** objects, uint numObjects)
  {
  }
  void NodeUpdate (const DynamicObjectExtraData& child1,
      const DynamicObjectExtraData& child2)
  {
  }
};

typedef CS::Geometry::AABBTree<DynamicObject, 10, DynamicObjectExtraData> DOTree;

struct DOCollectorInner
{
  csVector3 center;
  float sqradius;

  DOCollectorInner (const csVector3& center, float radius) :
    center (center), sqradius (radius * radius) { }
  bool operator() (const DOTree::Node* node)
  {
    return csIntersect3::BoxSphere (node->GetBBox (), center, sqradius);
  }
};

struct DOCollector
{
  csSet<csPtrKey<DynamicObject> >& prevObjects;
  csSet<csPtrKey<DynamicObject> >& objects;
  csVector3 center;
  float sqradius;

  DOCollector (
      csSet<csPtrKey<DynamicObject> >& prevObjects,
      csSet<csPtrKey<DynamicObject> >& objects, const csVector3& center,
      float radius) :
    prevObjects (prevObjects), objects (objects),
    center (center), sqradius (radius * radius) { }
  bool operator() (const DOTree::Node* node)
  {
    if (!csIntersect3::BoxSphere (node->GetBBox (), center, sqradius))
      return true;
    for (size_t i = 0 ; i < node->GetObjectCount () ; i++)
    {
      DynamicObject* dynobj = node->GetLeafData (i);
      float maxradiusRelative = dynobj->GetFactory ()->GetMaximumRadiusRelative ();
      float sqrad = sqradius * maxradiusRelative * maxradiusRelative;
      if (dynobj->IsStatic ())
        sqrad *= 1.1f;
      if (csIntersect3::BoxSphere (dynobj->GetBBox (), center, sqrad))
      {
        prevObjects.Delete (dynobj);
        objects.Add (dynobj);
      }
    }
    return true;
  }
};

class DynamicWorld : public scfImplementation2<DynamicWorld,
  iDynamicWorld, iComponent>
{
public:
  iObjectRegistry *object_reg;
  csRef<iEngine> engine;
  csRef<iDynamicSystem> dynSys;
  csRef<iVirtualClock> vc;
  csRefArray<DynamicObject> objects;
  csRefArray<DynamicFactory> factories;
  //csRefArray<CurvedMeshDynamicObjectCreator> cmdocs;
  csHash<DynamicFactory*,csString> factory_hash;
  iSector* sector;
  MeshCache meshCache;
  float radius;
  csSet<csPtrKey<DynamicObject> > fadingIn;
  csSet<csPtrKey<DynamicObject> > fadingOut;
  DOTree tree;

  csSet<csPtrKey<DynamicObject> > visibleObjects;

public:
  DynamicWorld (iBase *iParent);
  virtual ~DynamicWorld ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  virtual iDynamicFactory* AddFactory (const char* factory, float maxradius,
      float imposterradius);
  virtual void RemoveFactory (iDynamicFactory* factory);
  virtual void DynamicWorld::DeleteFactories ();
  virtual size_t GetFactoryCount () const { return factories.GetSize () ; }
  virtual iDynamicFactory* GetFactory (size_t index) const { return factories[index]; }

  virtual iDynamicObject* AddObject (const char* factory,
      const csReversibleTransform& trans);
  virtual void DeleteObject (iDynamicObject* dynobj);
  virtual void DeleteObjects ();
  virtual void Setup (iSector* sector, iDynamicSystem* dynSys);
  virtual void SetRadius (float radius);
  virtual float GetRadius () const { return radius; }
  virtual void PrepareView (iCamera* camera, csTicks elapsed_ticks);
  virtual iDynamicObject* FindObject (iRigidBody* body);
  virtual iDynamicObject* FindObject (iMeshWrapper* mesh);
  virtual void Save (iDocumentNode* node);
  virtual csRef<iString> Load (iDocumentNode* node);
};

}
CS_PLUGIN_NAMESPACE_END(DynWorld)

#endif // __ARES_DYNWORLD_IMP_H__
