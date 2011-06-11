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

#ifndef __ARES_ROOMS_IMP_H__
#define __ARES_ROOMS_IMP_H__

#include "csutil/scf.h"
#include "csutil/scf_implementation.h"
#include "csutil/weakref.h"
#include "csutil/set.h"
#include "csutil/hash.h"
#include "csutil/eventhandlers.h"
#include "csutil/refarr.h"
#include "csutil/parray.h"
#include "iutil/comp.h"
#include "iutil/virtclk.h"

#include "csgeom/vector3.h"
#include "csgeom/math3d.h"
#include "csgeom/path.h"

#include "imesh/genmesh.h"
#include "iengine/material.h"
#include "iengine/engine.h"
#include "iengine/mesh.h"

#include "include/igeometrygen.h"
#include "include/irooms.h"

CS_PLUGIN_NAMESPACE_BEGIN(RoomMesh)
{

struct RoomEntry
{
  csVector3 tl, br;
  RoomEntry () { }
  RoomEntry (const csVector3& tl, const csVector3& br) :
    tl (tl), br (br) { }
};

class RoomMeshCreator;

class RoomFactory : public scfImplementation2<RoomFactory, iRoomFactory,
  iGeometryGenerator>
{
private:
  RoomMeshCreator* creator;
  csString name;
  iMaterialWrapper* material;

  csRef<iMeshFactoryWrapper> factory;
  csRef<iGeneralFactoryState> state;

  csArray<RoomEntry> anchorRooms;

public:
  RoomFactory (RoomMeshCreator* creator, const char* name);
  virtual ~RoomFactory ();

  virtual const char* GetName () const { return name; }
  virtual iMeshFactoryWrapper* GetFactory () { return factory; }
  virtual void SetMaterial (const char* materialName);
  virtual size_t AddRoom (const csVector3& tl, const csVector3& br);
  virtual void DeleteRoom (size_t idx);
  virtual size_t GetRoomCount () const
  {
    return anchorRooms.GetSize ();
  }

  void Save (iDocumentNode* node, iSyntaxService* syn);
  bool Load (iDocumentNode* node, iSyntaxService* syn);

  virtual void GenerateGeometry (iMeshWrapper* mesh);
};

class RoomFactoryTemplate : public scfImplementation1<RoomFactoryTemplate,
  iRoomFactoryTemplate>
{
private:
  RoomMeshCreator* creator;
  csString name;
  csString material;

  csArray<RoomEntry> rooms;
  csHash<csString,csString> attributes;

public:
  RoomFactoryTemplate (RoomMeshCreator* creator, const char* name);
  virtual ~RoomFactoryTemplate ();

  virtual const char* GetName () const { return name; }
  virtual void SetAttribute (const char* name, const char* value);
  virtual const char* GetAttribute (const char* name) const;

  virtual void SetMaterial (const char* materialName);
  virtual size_t AddRoom (const csVector3& tl, const csVector3& br);

  const char* GetMaterial () const { return material; }
  const csArray<RoomEntry>& GetRooms () const { return rooms; }
  //void Save (iDocumentNode* node, iSyntaxService* syn);
  //bool Load (iDocumentNode* node, iSyntaxService* syn);
};

class RoomMeshCreator : public scfImplementation2<RoomMeshCreator,
  iRoomMeshCreator, iComponent>
{
public:
  iObjectRegistry *object_reg;
  csRef<iEngine> engine;
  csRefArray<RoomFactory> factories;
  csHash<RoomFactory*,csString> factory_hash;
  csRefArray<RoomFactoryTemplate> factoryTemplates;

  RoomFactoryTemplate* FindFactoryTemplate (const char* name);

public:
  RoomMeshCreator (iBase *iParent);
  virtual ~RoomMeshCreator ();
  virtual bool Initialize (iObjectRegistry *object_reg);

  virtual iRoomFactoryTemplate* AddRoomFactoryTemplate (const char* name);
  virtual size_t GetRoomFactoryTemplateCount () const
  {
    return factoryTemplates.GetSize ();
  }
  virtual iRoomFactoryTemplate* GetRoomFactoryTemplate (size_t idx) const
  {
    return factoryTemplates[idx];
  }
  virtual void DeleteRoomFactoryTemplates ();

  virtual void DeleteFactories ();
  virtual iRoomFactory* AddRoomFactory (const char* name, const char* templatename);
  virtual size_t GetRoomFactoryCount () const
  {
    return factories.GetSize ();
  }
  virtual iRoomFactory* GetRoomFactory (size_t idx) const
  {
    return factories[idx];
  }
  virtual iRoomFactory* GetRoomFactory (const char* name) const
  {
    return factory_hash.Get (name, 0);
  }

  virtual void Save (iDocumentNode* node);
  virtual csRef<iString> Load (iDocumentNode* node);
};

}
CS_PLUGIN_NAMESPACE_END(RoomMesh)

#endif // __ARES_ROOMS_IMP_H__
