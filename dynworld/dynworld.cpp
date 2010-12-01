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
#include "cssysdef.h"

#include "csutil/event.h"
#include "csutil/eventnames.h"
#include "csutil/eventhandlers.h"
#include "csutil/stringarray.h"
#include "csutil/csstring.h"
#include "csutil/xmltiny.h"
#include "csutil/scfstr.h"
#include "csutil/scanstr.h"
#include "cstool/csview.h"
#include "cstool/simplestaticlighter.h"
#include "iutil/objreg.h"
#include "iutil/object.h"
#include "iutil/eventh.h"
#include "iutil/plugin.h"
#include "iengine/sector.h"
#include "iengine/camera.h"
#include "iengine/movable.h"
#include "iengine/imposter.h"
#include "csgeom/math3d.h"
#include "ivaria/bullet.h"
#include "imesh/objmodel.h"

#include "dynworld.h"



CS_PLUGIN_NAMESPACE_BEGIN(DynWorld)
{

//---------------------------------------------------------------------------------------

MeshCache::MeshCache ()
{
}

MeshCache::~MeshCache ()
{
  csHash<MeshCacheFactory*,csString>::GlobalIterator it = meshFactories.GetIterator ();
  while (it.HasNext ())
  {
    csString key;
    MeshCacheFactory* mf = it.Next (key);
    delete mf;
  }
}

iMeshWrapper* MeshCache::AddMesh (iEngine* engine,
    iMeshFactoryWrapper* factory, iSector* sector,
    const csReversibleTransform& trans)
{
  const char* factname = factory->QueryObject ()->GetName ();
  csRef<iMeshWrapper> mesh;
  MeshCacheFactory* mf = meshFactories.Get (factname, 0);
  if (mf && mf->meshes.GetSize () > 0)
  {
    mesh = mf->meshes.Pop ();
    mesh->GetMovable ()->SetSector (sector);
  }
  else
  {
    mesh = engine->CreateMeshWrapper (factory, "", sector);
  }

  iMovable* movable = mesh->GetMovable ();
  movable->SetTransform (trans);
  movable->UpdateMove ();
  return mesh;
}

void MeshCache::RemoveMesh (iMeshWrapper* mesh)
{
  const char* factname = mesh->GetFactory ()->QueryObject ()->GetName ();
  MeshCacheFactory* mf = meshFactories.Get (factname, 0);
  if (!mf)
  {
    mf = new MeshCacheFactory ();
    meshFactories.Put (factname, mf);
  }
  mf->meshes.Push (mesh);
  mesh->GetMovable ()->ClearSectors ();
  mesh->GetMovable ()->UpdateMove ();
}

//---------------------------------------------------------------------------------------

DynamicFactory::DynamicFactory (DynamicWorld* world, const char* name,
    float maxradiusRelative, float imposterradius) :
  scfImplementationType (this), name (name), world (world),
  maxradiusRelative (maxradiusRelative), imposterradius (imposterradius)
{
  bbox.StartBoundingBox ();
  physBbox.StartBoundingBox ();

  factory = world->engine->FindMeshFactory (name);
  if (factory == 0)
  {
    printf ("Could not find factory '%s'!\n", name);
    fflush (stdout);
    return;
  }

  if (imposterradius > 0)
  {
    imposterFactory = scfQueryInterface<iImposterFactory> (factory);
    imposterFactory->SetMinDistance (imposterradius * world->radius);
    //imposterFactory->SetRotationTolerance (45.0);
    //imposterFactory->SetCameraRotationTolerance(45.0);
    imposterFactory->SetShader("base", "lighting_imposter");
    imposterFactory->SetRenderReal(true);
  }

  iObjectModel* model = factory->GetMeshObjectFactory ()->GetObjectModel ();
  if (model)
  {
    model->GetRadius (bsphereRadius, bsphereCenter);
    bbox = model->GetObjectBoundingBox ();
  }
  else
  {
    printf ("WARNING! No object model for %s!\n", factory->QueryObject ()->GetName ());
    fflush (stdout);
  }
}

void DynamicFactory::SetAttribute (const char* name, const char* value)
{
  attributes.Put (name, value);
}

const char* DynamicFactory::GetAttribute (const char* name) const
{
  return attributes.Get (name, (const char*)0);
}

void DynamicFactory::AddRigidBox (const csVector3& offset, const csVector3& size,
    float mass)
{
  csBox3 b (-size / 2 + offset, size / 2 + offset);
  for (size_t i = 0 ; i < 8 ; i++)
    physBbox.AddBoundingVertex (b.GetCorner (i));
  colliders.Push (new DOColliderBox (size, offset, mass));
}

void DynamicFactory::AddRigidSphere (float radius, const csVector3& offset,
    float mass)
{
  csBox3 b (offset - csVector3 (radius), offset + csVector3 (radius));
  for (size_t i = 0 ; i < 8 ; i++)
    physBbox.AddBoundingVertex (b.GetCorner (i));
  colliders.Push (new DOColliderSphere (offset, radius, mass));
}

void DynamicFactory::AddRigidCylinder (float radius, float length,
      const csVector3& offset, float mass)
{
  csBox3 b (offset - csVector3 (radius, length/2, radius),
      offset + csVector3 (radius, length/2, radius));
  for (size_t i = 0 ; i < 8 ; i++)
    physBbox.AddBoundingVertex (b.GetCorner (i));
  colliders.Push (new DOColliderCylinder (offset, length, radius, mass));
}

void DynamicFactory::AddRigidMesh (const csVector3& offset, float mass)
{
  for (size_t i = 0 ; i < 8 ; i++)
    physBbox.AddBoundingVertex (bbox.GetCorner (i));
  colliders.Push (new DOColliderMesh (offset, mass));
}

void DynamicFactory::AddRigidConvexMesh (const csVector3& offset, float mass)
{
  for (size_t i = 0 ; i < 8 ; i++)
    physBbox.AddBoundingVertex (bbox.GetCorner (i));
  colliders.Push (new DOColliderConvexMesh (offset, mass));
}

//---------------------------------------------------------------------------------------

void DynamicObject::Init ()
{
  factory = 0;
  is_static = false;
  is_kinematic = false;
  is_hilight = false;
  hilight_installed = false;
  fade = 0;
  bboxValid = false;
}

DynamicObject::DynamicObject () : scfImplementationType (this)
{
  Init ();
}

DynamicObject::DynamicObject (DynamicFactory* factory, const csReversibleTransform& trans)
  : scfImplementationType (this)
{
  Init ();
  DynamicObject::factory = factory;
  DynamicObject::trans = trans;
}

DynamicObject::~DynamicObject ()
{
}

void DynamicObject::InstallHilight (bool hi)
{
  csRef<iShaderVarStringSet> strings =
    csQueryRegistryTagInterface<iShaderVarStringSet>
    (factory->GetWorld ()->object_reg, "crystalspace.shader.variablenameset");
  CS::ShaderVarStringID ambient = strings->Request ("light ambient");
  csRef<iShaderVariableContext> svc =
    scfQueryInterfaceSafe<iShaderVariableContext> (mesh);
  if (svc)
  {
    if (hi)
    {
      if (!hilight_installed)
      {
        csRef<csShaderVariable> var = svc->GetVariableAdd(ambient);
        var->SetValue(csVector3(1,1,1));
	hilight_installed = true;
      }
    }
    else if (hilight_installed)
    {
      svc->RemoveVariable(ambient);
      hilight_installed = false;
    }
  }
}

void DynamicObject::SetHilight (bool hi)
{
  if (hi == is_hilight) return;
  is_hilight = hi;
  if (!mesh) return;
  InstallHilight (hi);
}

void DynamicObject::MakeStatic ()
{
  is_static = true;
  if (!is_kinematic && body)
    body->MakeStatic ();
}

void DynamicObject::MakeDynamic ()
{
  is_static = false;
  if (!is_kinematic && body)
    body->MakeDynamic ();
}

void DynamicObject::MakeKinematic ()
{
  if (is_kinematic) return;
  is_kinematic = true;
  if (!body) return;
  csRef<CS::Physics::Bullet::iRigidBody> bulletBody = scfQueryInterface<CS::Physics::Bullet::iRigidBody> (body);
  bulletBody->MakeKinematic ();
}

void DynamicObject::UndoKinematic ()
{
  if (!is_kinematic) return;
  is_kinematic = false;
  if (!body) return;
  if (is_static)
    body->MakeStatic ();
  else
    body->MakeDynamic ();
}

void DynamicObject::SetFade (float f)
{
  fade = f;
  if (mesh)
  {
    mesh->GetMeshObject ()->SetMixMode (CS_FX_SETALPHA(fade));
  }
}

void DynamicObject::RemoveMesh (DynamicWorld* world)
{
  if (!mesh) return;
  InstallHilight (false);
  CS_ASSERT (hilight_installed == false);
  world->dynSys->RemoveBody (body);
  body = 0;
  trans = mesh->GetMovable ()->GetTransform ();
  iImposterFactory* imposterFactory = factory->GetImposterFactory ();
  if (imposterFactory)
    imposterFactory->RemoveImposter (mesh);
  mesh->GetMovable ()->RemoveListener (this);
  world->meshCache.RemoveMesh (mesh);
  mesh = 0;
}

void DynamicObject::PrepareMesh (DynamicWorld* world)
{
  if (mesh) return;
  mesh = world->meshCache.AddMesh (world->engine, factory->GetMeshFactory (),
      world->sector, trans);
  mesh->GetMovable ()->AddListener (this);
  InstallHilight (is_hilight);
  CS_ASSERT (hilight_installed == is_hilight);
  SetFade (fade);
  const csPDelArray<DOCollider>& colliders = factory->GetColliders ();
  for (size_t i = 0 ; i < colliders.GetSize () ; i++)
  {
    body = colliders[i]->Create (world->dynSys, mesh, trans, body);
  }
  if (is_static)
    body->MakeStatic ();
}

void DynamicObject::RefreshColliders ()
{
  if (!body) return;
  body->DestroyColliders ();
  const csPDelArray<DOCollider>& colliders = factory->GetColliders ();
  for (size_t i = 0 ; i < colliders.GetSize () ; i++)
  {
    body = colliders[i]->Create (factory->GetWorld ()->dynSys, mesh, trans, body);
  }
  if (is_static)
    body->MakeStatic ();
}

bool DynamicObject::HasBody (iRigidBody* body)
{
  return body == DynamicObject::body;
}

const csReversibleTransform& DynamicObject::GetTransform ()
{
  if (mesh)
    trans = mesh->GetMovable ()->GetTransform ();
  return trans;
}

void DynamicObject::SetTransform (const csReversibleTransform& trans)
{
  if (mesh)
  {
    mesh->GetMovable ()->SetTransform (trans);
    mesh->GetMovable ()->UpdateMove ();
  }
  DynamicObject::trans = trans;
}

void DynamicObject::Save (iDocumentNode* node, iSyntaxService* syn)
{
  if (mesh)
    trans = mesh->GetMovable ()->GetTransform ();

  node->SetAttribute ("fact", factory->GetName ());
  if (is_static)
    node->SetAttribute ("static", "true");

  csString matrix;
  const csMatrix3& m = trans.GetO2T ();
  matrix.Format ("%g %g %g %g %g %g %g %g %g",
      m.m11, m.m12, m.m13,
      m.m21, m.m22, m.m23,
      m.m31, m.m32, m.m33);
  node->SetAttribute ("m", (const char*)matrix);

  const csVector3& v = trans.GetO2TTranslation ();
  csString vector;
  vector.Format ("%g %g %g", v.x, v.y, v.z);
  node->SetAttribute ("v", (const char*)vector);
}

bool DynamicObject::Load (iDocumentNode* node, iSyntaxService* syn,
    DynamicWorld* world)
{
  csString factname = node->GetAttributeValue ("fact");
  factory = world->factory_hash.Get (factname, 0);
  if (!factory) return false;
  if (!syn->ParseBoolAttribute (node, "static", is_static, false, false))
    return false;

  csMatrix3 m;
  csVector3 v (0);
  csString matrix = node->GetAttributeValue ("m");
  if (matrix.Length () > 0)
  {
    csScanStr ((const char*)matrix, "%f %f %f %f %f %f %f %f %f",
      &m.m11, &m.m12, &m.m13,
      &m.m21, &m.m22, &m.m23,
      &m.m31, &m.m32, &m.m33);
  }
  csString vector = node->GetAttributeValue ("v");
  if (vector.Length () > 0)
  {
    csScanStr ((const char*)vector, "%f %f %f", &v.x, &v.y, &v.z);
  }
  trans.SetO2T (m);
  trans.SetO2TTranslation (v);

  return true;
}

void DynamicObject::MovableChanged (iMovable*)
{
  if (bboxValid)
  {
    csBox3 oldBbox = bbox;
    bboxValid = false;
    factory->GetWorld ()->tree.MoveObject (this, oldBbox);
  }
  else
  {
    bboxValid = false;
    factory->GetWorld ()->tree.RemoveObject (this);
    factory->GetWorld ()->tree.AddObject (this);
  }
}

void DynamicObject::MovableDestroyed (iMovable*)
{
}

const csBox3& DynamicObject::GetBBox () const
{
  if (bboxValid) return bbox;
  if (mesh)
    trans = mesh->GetMovable ()->GetTransform ();

  const csBox3& factBBox = factory->GetBBox ();
  bbox.StartBoundingBox (trans.This2Other (factBBox.GetCorner (0)));
  bbox.AddBoundingVertexSmart (trans.This2Other (factBBox.GetCorner (1)));
  bbox.AddBoundingVertexSmart (trans.This2Other (factBBox.GetCorner (2)));
  bbox.AddBoundingVertexSmart (trans.This2Other (factBBox.GetCorner (3)));
  bbox.AddBoundingVertexSmart (trans.This2Other (factBBox.GetCorner (4)));
  bbox.AddBoundingVertexSmart (trans.This2Other (factBBox.GetCorner (5)));
  bbox.AddBoundingVertexSmart (trans.This2Other (factBBox.GetCorner (6)));
  bbox.AddBoundingVertexSmart (trans.This2Other (factBBox.GetCorner (7)));
  bboxValid = true;

  return bbox;
}

//---------------------------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (DynamicWorld)

DynamicWorld::DynamicWorld (iBase *iParent)
  : scfImplementationType (this, iParent)
{  
  object_reg = 0;
  radius = 80;
}

DynamicWorld::~DynamicWorld ()
{
  DeleteObjects ();
}

bool DynamicWorld::Initialize (iObjectRegistry *object_reg)
{
  DynamicWorld::object_reg = object_reg;
  engine = csQueryRegistry<iEngine> (object_reg);
  vc = csQueryRegistry<iVirtualClock> (object_reg);
  return true;
}

iDynamicFactory* DynamicWorld::AddFactory (const char* factory, float maxradius,
    float imposterradius)
{
  csRef<DynamicFactory> obj;
  obj.AttachNew (new DynamicFactory (this, factory, maxradius, imposterradius));
  factories.Push (obj);
  factory_hash.Put (factory, obj);
  return obj;
}

void DynamicWorld::RemoveFactory (iDynamicFactory* factory)
{
  factory_hash.DeleteAll (factory->GetName ());
  factories.Delete (static_cast<DynamicFactory*> (factory));
}

iDynamicObject* DynamicWorld::AddObject (const char* factory,
    const csReversibleTransform& trans)
{
  csRef<DynamicObject> obj;
  DynamicFactory* fact = factory_hash.Get (factory, 0);
  if (!fact) return 0;
  obj.AttachNew (new DynamicObject (fact, trans));
  objects.Push (obj);
  tree.AddObject (obj);
  return obj;
}

void DynamicWorld::DeleteObjects ()
{
  while (objects.GetSize () > 0)
  {
    DeleteObject (objects[0]);
  }
}

void DynamicWorld::DeleteObject (iDynamicObject* dynobj)
{
  DynamicObject* dyn = static_cast<DynamicObject*> (dynobj);
  visibleObjects.Delete (dyn);
  fadingOut.Delete (dyn);
  fadingIn.Delete (dyn);
  size_t idx = objects.Find (dyn);
  if (idx != csArrayItemNotFound)
  {
    tree.RemoveObject (dyn);
    dyn->RemoveMesh (this);
    objects.DeleteIndex (idx);
  }
}

void DynamicWorld::PrepareView (iCamera* camera, csTicks elapsed_ticks)
{
  float fade_speed = float (elapsed_ticks) / 3000.0;

  {
    csSet<csPtrKey<DynamicObject> > newset;
    csSet<csPtrKey<DynamicObject> >::GlobalIterator it = fadingIn.GetIterator ();
    while (it.HasNext ())
    {
      DynamicObject* dyn = it.Next ();
      float f = dyn->GetFade ();
      f -= fade_speed;
      if (f <= 0)
      {
        f = 0;
      }
      else
      {
        newset.Add (dyn);
      }
      dyn->SetFade (f);
    }
    fadingIn = newset;
  }

  {
    csSet<csPtrKey<DynamicObject> > newset;
    csSet<csPtrKey<DynamicObject> >::GlobalIterator it = fadingOut.GetIterator ();
    while (it.HasNext ())
    {
      DynamicObject* dyn = it.Next ();
      float f = dyn->GetFade ();
      f += fade_speed;
      if (f >= 1)
      {
        f = 1;
        dyn->RemoveMesh (this);
      }
      else
      {
        newset.Add (dyn);
      }
      dyn->SetFade (f);
    }
    fadingOut = newset;
  }

  const csVector3& campos = camera->GetTransform ().GetOrigin ();

  csSet<csPtrKey<DynamicObject> > prevVisible = visibleObjects;
  visibleObjects.Empty ();
  DOCollector collector (prevVisible, visibleObjects, campos, radius);
  DOCollectorInner inner (campos, radius);
  tree.Traverse (inner, collector);

  csSet<csPtrKey<DynamicObject> >::GlobalIterator it = prevVisible.GetIterator ();
  while (it.HasNext ())
  {
    DynamicObject* dyn = it.Next ();
    if (dyn->GetMesh ())
    {
      fadingIn.Delete (dyn);
      fadingOut.Add (dyn);
    }
  }

  csSet<csPtrKey<DynamicObject> >::GlobalIterator it2 = visibleObjects.GetIterator ();
  while (it2.HasNext ())
  {
    DynamicObject* dyn = it2.Next ();
    if (!dyn->GetMesh ())
    {
      dyn->PrepareMesh (this);
      fadingOut.Delete (dyn);
      fadingIn.Add (dyn);
    }
  }
}

iDynamicObject* DynamicWorld::FindObject (iRigidBody* body)
{
  size_t i;
  for (i = 0 ; i < objects.GetSize () ; i++)
  {
    DynamicObject* dyn = objects[i];
    if (dyn->HasBody (body))
      return dyn;
  }
  return 0;
}

iDynamicObject* DynamicWorld::FindObject (iMeshWrapper* mesh)
{
  size_t i;
  for (i = 0 ; i < objects.GetSize () ; i++)
  {
    DynamicObject* dyn = objects[i];
    if (dyn->GetMesh () == mesh)
      return dyn;
  }
  return 0;
}

void DynamicWorld::Setup (iSector* sector, iDynamicSystem* dynSys)
{
  DynamicWorld::sector = sector;
  DynamicWorld::dynSys = dynSys;
}

void DynamicWorld::SetRadius (float radius)
{
  DynamicWorld::radius = radius;
}

void DynamicWorld::Save (iDocumentNode* node)
{
  csRef<iSyntaxService> syn = csQueryRegistryOrLoad<iSyntaxService> (object_reg,
      "crystalspace.syntax.loader.service.text");

  for (size_t i = 0 ; i < objects.GetSize () ; i++)
  {
    csRef<iDocumentNode> el = node->CreateNodeBefore (CS_NODE_ELEMENT);
    el->SetValue ("obj");
    objects[i]->Save (el, syn);
  }
}

csRef<iString> DynamicWorld::Load (iDocumentNode* node)
{
  csRef<iSyntaxService> syn = csQueryRegistryOrLoad<iSyntaxService> (object_reg,
      "crystalspace.syntax.loader.service.text");

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "obj")
    {
      csRef<DynamicObject> dynobj;
      dynobj.AttachNew (new DynamicObject ());
      if (!dynobj->Load (child, syn, this))
      {
        csRef<iString> str;
        str.AttachNew (new scfString ("Error loading object!"));
        return str;
      }
      objects.Push (dynobj);
      tree.AddObject (dynobj);
    }
  }

  return 0;
}

}
CS_PLUGIN_NAMESPACE_END(DynWorld)

