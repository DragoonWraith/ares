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

#ifndef __ARES_DYNWORLD_H__
#define __ARES_DYNWORLD_H__

#include "csutil/scf.h"

class csBox3;
class csVector3;
class csReversibleTransform;
struct iCamera;
struct iSector;
struct iDynamicSystem;
struct iRigidBody;
struct iString;
struct iMeshWrapper;

/**
 * A factory object in the dynamic world.
 */
struct iDynamicFactory : public virtual iBase
{
  SCF_INTERFACE(iDynamicFactory,0,0,1);

  virtual const char* GetName () const = 0;

  /**
   * Generic attribute system.
   */
  virtual void SetAttribute (const char* name, const char* value) = 0;

  /**
   * Get an attribute.
   */
  virtual const char* GetAttribute (const char* name) const = 0;

  /**
   * Get the maximum relative radius for this factory.
   */
  virtual float GetMaximumRadiusRelative () const = 0;

  /**
   * Get the bounding box in object space as reported by iObjectModel
   * for the given mesh factory.
   */
  virtual const csBox3& GetBBox () const = 0;

  /**
   * Get the bounding box as calculated from the physical objects that
   * have been attached to this factory.
   */
  virtual const csBox3& GetPhysicsBBox () const = 0;

  /**
   * Create a box rigid body.
   */
  virtual void AddRigidBox (const csVector3& offset, const csVector3& size,
      float mass) = 0;

  /**
   * Create a sphere rigid body.
   */
  virtual void AddRigidSphere (float radius, const csVector3& offset,
      float mass) = 0;

  /**
   * Create a sphere rigid body.
   */
  virtual void AddRigidCylinder (float radius, float length,
      const csVector3& offset, float mass) = 0;

  /**
   * Create a mesh rigid body.
   */
  virtual void AddRigidMesh (const csVector3& offset, float mass) = 0;

  /**
   * Create a convex mesh rigid body.
   */
  virtual void AddRigidConvexMesh (const csVector3& offset, float mass) = 0;
};

/**
 * An object in the dynamic world.
 */
struct iDynamicObject : public virtual iBase
{
  SCF_INTERFACE(iDynamicObject,0,0,1);

  /**
   * Get the factory from which this dynamic object was created.
   */
  virtual iDynamicFactory* GetFactory () const = 0;

  /**
   * Make static.
   */
  virtual void MakeStatic () = 0;

  /**
   * Make dynamic.
   */
  virtual void MakeDynamic () = 0;

  /**
   * Make kinematic.
   */
  virtual void MakeKinematic () = 0;

  /**
   * Undo kinematic and restore previous static or dynamic state.
   */
  virtual void UndoKinematic () = 0;

  /**
   * Is static?
   */
  virtual bool IsStatic () const = 0;

  /**
   * Set hilight.
   */
  virtual void SetHilight (bool hi) = 0;

  /**
   * Check hilight.
   */
  virtual bool IsHilight () const = 0;

  /**
   * Get the bounding box in world space.
   */
  virtual const csBox3& GetBBox () const = 0;

  /**
   * Get the mesh for this object. Can be 0 if the
   * object is currently not visible.
   */
  virtual iMeshWrapper* GetMesh () const = 0;

  /**
   * Get the body for this object. Can be 0 if the
   * object is currently not visible.
   */
  virtual iRigidBody* GetBody () const = 0;

  /**
   * Refresh the colliders for this object.
   */
  virtual void RefreshColliders () = 0;

  /**
   * Get the transform of this object.
   */
  virtual const csReversibleTransform& GetTransform () = 0;

  /**
   * Set the transform of this object. It is usually recommended
   * to make the object kinematic before you do this. This function
   * will not automatically do that.
   */
  virtual void SetTransform (const csReversibleTransform& trans) = 0;
};

/**
 * Interface to the dynamic world plugin.
 */
struct iDynamicWorld : public virtual iBase
{
  SCF_INTERFACE(iDynamicWorld,0,0,1);

  //------------------------------------------------------------------------------

  /**
   * Add a new dynamic factory to the world.
   * @param maxradius is a relative maximum radius (0 to 1) at which point
   * the object should become visible. It is relative to the total maximum
   * radius maintained by this world.
   * @param imposterradius is the relative radius (0 to 1) at which the object
   * should be impostered. Set to negative if you don't want to use imposters at all.
   * It is also relative to the total maximum radius maintained by this world.
   * Note that imposterradius should always be less then maxradius.
   */
  virtual iDynamicFactory* AddFactory (const char* factory, float maxradius,
      float imposterradius) = 0;

  /**
   * Remove a factory from the world.
   */
  virtual void RemoveFactory (iDynamicFactory* factory) = 0;

  /**
   * Get the number of factories.
   */
  virtual size_t GetFactoryCount () const = 0;

  /**
   * Get a factory.
   */
  virtual iDynamicFactory* GetFactory (size_t index) const = 0;

  //------------------------------------------------------------------------------

  /**
   * Add a new dynamic object to the world. This object will have no physics
   * properties yet.
   */
  virtual iDynamicObject* AddObject (const char* factory,
      const csReversibleTransform& trans) = 0;

  /**
   * Remove an object.
   */
  virtual void DeleteObject (iDynamicObject* dynobj) = 0;

  /**
   * Delete all objects.
   */
  virtual void DeleteObjects () = 0;

  /**
   * Set the sector for this dynamic world.
   */
  virtual void Setup (iSector* sector, iDynamicSystem* dynSys) = 0;

  /**
   * Set the view radius. Default radius is 20.
   */
  virtual void SetRadius (float radius) = 0;

  /**
   * Get the view radius.
   */
  virtual float GetRadius () const = 0;

  /**
   * Prepare the sector for viewing at a certain location.
   */
  virtual void PrepareView (iCamera* camera, csTicks elapsed_ticks) = 0;

  /**
   * Find an object given its rigid body.
   */
  virtual iDynamicObject* FindObject (iRigidBody* body) = 0;

  /**
   * Find an object given its mesh.
   */
  virtual iDynamicObject* FindObject (iMeshWrapper* mesh) = 0;

  /**
   * Save the world to XML.
   */
  virtual void Save (iDocumentNode* node) = 0;

  /**
   * Load the world from XML.
   * Return 0 on success or otherwise a string with the error.
   */
  virtual csRef<iString> Load (iDocumentNode* node) = 0;
};

#endif // __ARES_DYNWORLD_H__

