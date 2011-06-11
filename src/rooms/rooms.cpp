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
#include "cssysdef.h"

#include "csutil/event.h"
#include "csutil/eventnames.h"
#include "csutil/eventhandlers.h"
#include "csutil/stringarray.h"
#include "csutil/csstring.h"
#include "csutil/xmltiny.h"
#include "csutil/scfstr.h"
#include "csutil/scanstr.h"
#include "iutil/objreg.h"
#include "iutil/object.h"
#include "iutil/eventh.h"
#include "iutil/plugin.h"
#include "csgeom/tri.h"

#include "imap/services.h"
#include "iengine/movable.h"
#include "iengine/sector.h"

#include "rooms.h"

#define VERBOSE 0


CS_PLUGIN_NAMESPACE_BEGIN(RoomMesh)
{

//---------------------------------------------------------------------------------------

RoomFactory::RoomFactory (RoomMeshCreator* creator, const char* name) :
  scfImplementationType (this), creator (creator), name (name)
{
  material = 0;

  factory = creator->engine->CreateMeshFactory (
	"crystalspace.mesh.object.genmesh", name);
  state = scfQueryInterface<iGeneralFactoryState> (factory->GetMeshObjectFactory ());
}

RoomFactory::~RoomFactory ()
{
}

void RoomFactory::GenerateGeometry (iMeshWrapper* thisMesh)
{
}

void RoomFactory::SetMaterial (const char* materialName)
{
  material = creator->engine->FindMaterial (materialName);
  if (!material)
  {
    printf ("Could not find material '%s' for room factory '%s'!\n",
	materialName, name.GetData ());
    fflush (stdout);
  }
}

size_t RoomFactory::AddRoom (const csVector3& tl, const csVector3& br)
{
  return anchorRooms.Push (RoomEntry (tl, br));
}

void RoomFactory::DeleteRoom (size_t idx)
{
  anchorRooms.DeleteIndex (idx);
}

void RoomFactory::Save (iDocumentNode* node, iSyntaxService* syn)
{
  node->SetAttribute ("name", name);
  node->SetAttribute ("material", material->QueryObject ()->GetName ());
  size_t i;
  for (i = 0 ; i < anchorRooms.GetSize () ; i++)
  {
    csRef<iDocumentNode> el = node->CreateNodeBefore (CS_NODE_ELEMENT);
    el->SetValue ("r");
    csString vector;
    vector.Format ("%g %g %g", anchorRooms[i].tl.x, anchorRooms[i].tl.y, anchorRooms[i].tl.z);
    el->SetAttribute ("tl", (const char*)vector);
    vector.Format ("%g %g %g", anchorRooms[i].br.x, anchorRooms[i].br.y, anchorRooms[i].br.z);
    el->SetAttribute ("br", (const char*)vector);
  }
}

bool RoomFactory::Load (iDocumentNode* node, iSyntaxService* syn)
{
  csString materialName = node->GetAttributeValue ("material");
  SetMaterial (materialName);
  anchorRooms.DeleteAll ();
  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "r")
    {
      csVector3 tl, br;
      csString vector = child->GetAttributeValue ("tl");
      if (vector.Length () > 0)
	csScanStr ((const char*)vector, "%f %f %f", &tl.x, &tl.y, &tl.z);
      vector = child->GetAttributeValue ("br");
      if (vector.Length () > 0)
	csScanStr ((const char*)vector, "%f %f %f", &br.x, &br.y, &br.z);
      AddRoom (tl, br);
    }
  }

  return true;
}

//---------------------------------------------------------------------------------------

RoomFactoryTemplate::RoomFactoryTemplate (RoomMeshCreator* creator,
    const char* name) : scfImplementationType (this), creator (creator), name (name)
{
}

RoomFactoryTemplate::~RoomFactoryTemplate ()
{
}

void RoomFactoryTemplate::SetAttribute (const char* name, const char* value)
{
  attributes.Put (name, value);
}

const char* RoomFactoryTemplate::GetAttribute (const char* name) const
{
  return attributes.Get (name, (const char*)0);
}

void RoomFactoryTemplate::SetMaterial (const char* materialName)
{
  material = materialName;
}

size_t RoomFactoryTemplate::AddRoom (const csVector3& tl, const csVector3& br)
{
  return rooms.Push (RoomEntry (tl, br));
}

//---------------------------------------------------------------------------------------

SCF_IMPLEMENT_FACTORY (RoomMeshCreator)

RoomMeshCreator::RoomMeshCreator (iBase *iParent)
  : scfImplementationType (this, iParent)
{  
  object_reg = 0;
}

RoomMeshCreator::~RoomMeshCreator ()
{
}

bool RoomMeshCreator::Initialize (iObjectRegistry *object_reg)
{
  RoomMeshCreator::object_reg = object_reg;
  engine = csQueryRegistry<iEngine> (object_reg);
  return true;
}

void RoomMeshCreator::DeleteFactories ()
{
  size_t i;
  for (i = 0 ; i < factories.GetSize () ; i++)
    engine->RemoveObject (factories[i]->GetFactory ());
  factories.DeleteAll ();
  factory_hash.Empty ();
}

iRoomFactoryTemplate* RoomMeshCreator::AddRoomFactoryTemplate (const char* name)
{
  RoomFactoryTemplate* cf = new RoomFactoryTemplate (this, name);
  factoryTemplates.Push (cf);
  cf->DecRef ();
  return cf;
}

RoomFactoryTemplate* RoomMeshCreator::FindFactoryTemplate (const char* name)
{
  size_t i;
  csString tn = name;
  for (i = 0 ; i < factoryTemplates.GetSize () ; i++)
  {
    if (tn == factoryTemplates[i]->GetName ())
      return factoryTemplates[i];
  }
  return 0;
}

void RoomMeshCreator::DeleteRoomFactoryTemplates ()
{
  factoryTemplates.DeleteAll ();
}

iRoomFactory* RoomMeshCreator::AddRoomFactory (const char* name, const char* templatename)
{
  RoomFactoryTemplate* cftemp = FindFactoryTemplate (templatename);
  if (!cftemp)
  {
    printf ("ERROR! Can't find factory template '%s'!\n", templatename);
    return 0;
  }
  RoomFactory* cf = new RoomFactory (this, name);
  cf->SetMaterial (cftemp->GetMaterial ());
  const csArray<RoomEntry>& rooms = cftemp->GetRooms ();
  for (size_t i = 0 ; i < rooms.GetSize () ; i++)
    cf->AddRoom (rooms[i].tl, rooms[i].br);
  factories.Push (cf);
  factory_hash.Put (name, cf);
  cf->DecRef ();
  return cf;
}

void RoomMeshCreator::Save (iDocumentNode* node)
{
  csRef<iSyntaxService> syn = csQueryRegistryOrLoad<iSyntaxService> (object_reg,
      "crystalspace.syntax.loader.service.text");
  for (size_t i = 0 ; i < factories.GetSize () ; i++)
  {
    csRef<iDocumentNode> el = node->CreateNodeBefore (CS_NODE_ELEMENT);
    el->SetValue ("fact");
    factories[i]->Save (el, syn);
  }
}

csRef<iString> RoomMeshCreator::Load (iDocumentNode* node)
{
  csRef<iSyntaxService> syn = csQueryRegistryOrLoad<iSyntaxService> (object_reg,
      "crystalspace.syntax.loader.service.text");

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "fact")
    {
      csRef<RoomFactory> cfact;
      csString name = child->GetAttributeValue ("name");
      cfact.AttachNew (new RoomFactory (this, name));
      if (!cfact->Load (child, syn))
      {
        csRef<iString> str;
        str.AttachNew (new scfString ("Error loading factory!"));
        return str;
      }
      factories.Push (cfact);
      factory_hash.Put (name, cfact);
    }
  }

  return 0;
}

}
CS_PLUGIN_NAMESPACE_END(RoomMesh)

