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
#include "include/icurvemesh.h"
#include "include/irooms.h"
#include "worldload.h"

#include "propclass/dynworld.h"

WorldLoader::WorldLoader (iObjectRegistry* object_reg) : object_reg (object_reg)
{
  loader = csQueryRegistry<iLoader> (object_reg);
  vfs = csQueryRegistry<iVFS> (object_reg);
  engine = csQueryRegistry<iEngine> (object_reg);
  curvedMeshCreator = csQueryRegistry<iCurvedMeshCreator> (object_reg);
  roomMeshCreator = csQueryRegistry<iRoomMeshCreator> (object_reg);
}

bool WorldLoader::LoadLibrary (const char* path, const char* file)
{
  // Set current VFS dir to the level dir, helps with relative paths in maps
  vfs->PushDir (path);
  csLoadResult rc = loader->Load (file);
  if (!rc.success)
  {
    vfs->PopDir ();
    //@@@return ReportError("Couldn't load library file %s!", path);
    return false;
  }
  vfs->PopDir ();
  return true;
}

bool WorldLoader::LoadDoc (iDocument* doc)
{
  csRef<iDocumentNode> root = doc->GetRoot ();
  csRef<iDocumentNode> dynlevelNode = root->GetNode ("dynlevel");

  csRef<iDocumentNodeIterator> it = dynlevelNode->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "asset")
    {
      csString path = child->GetAttributeValue ("path");
      csString file = child->GetAttributeValue ("file");
      if (!LoadLibrary (path, file))
	return false;
      assets.Push (Asset (path, file));
    }
    // Ignore the other tags. These are processed below.
  }

  csRef<iDocumentNode> curveNode = dynlevelNode->GetNode ("curves");
  if (curveNode)
  {
    csRef<iString> error = curvedMeshCreator->Load (curveNode);
    if (error)
    {
      printf ("Error loading curves '%s'!", error->GetData ());
      return false;
    }
  }

  for (size_t i = 0 ; i < curvedMeshCreator->GetCurvedFactoryCount () ; i++)
  {
    iCurvedFactory* cfact = curvedMeshCreator->GetCurvedFactory (i);
    iDynamicFactory* fact = dynworld->AddFactory (cfact->GetName (), 1.0, -1);
    csRef<iGeometryGenerator> ggen = scfQueryInterface<iGeometryGenerator> (cfact);
    if (ggen)
      fact->SetGeometryGenerator (ggen);
    fact->AddRigidMesh (csVector3 (0), 10.0);
    curvedFactories.Push (fact);
  }

  csRef<iDocumentNode> roomNode = dynlevelNode->GetNode ("rooms");
  if (roomNode)
  {
    csRef<iString> error = roomMeshCreator->Load (roomNode);
    if (error)
    {
      printf ("Error loading rooms '%s'!", error->GetData ());
      return false;
    }
  }

  for (size_t i = 0 ; i < roomMeshCreator->GetRoomFactoryCount () ; i++)
  {
    iRoomFactory* cfact = roomMeshCreator->GetRoomFactory (i);
    iDynamicFactory* fact = dynworld->AddFactory (cfact->GetName (), 1.0, -1);
    csRef<iGeometryGenerator> ggen = scfQueryInterface<iGeometryGenerator> (cfact);
    if (ggen)
      fact->SetGeometryGenerator (ggen);
    fact->AddRigidMesh (csVector3 (0), 10.0);
    roomFactories.Push (fact);
  }

  csRef<iDocumentNode> dynworldNode = dynlevelNode->GetNode ("dynworld");
  if (dynworldNode)
  {
    csRef<iString> error = dynworld->Load (dynworldNode);
    if (error)
    {
      printf ("Error loading dynworld '%s'!", error->GetData ());
      return false;
    }
  }
  return true;
}

bool WorldLoader::LoadFile (const char* filename)
{
  assets.DeleteAll ();
  dynworld->DeleteObjects ();
  dynworld->DeleteFactories ();

  curvedMeshCreator->DeleteFactories ();
  curvedMeshCreator->DeleteCurvedFactoryTemplates ();
  curvedFactories.DeleteAll ();

  roomMeshCreator->DeleteFactories ();
  roomMeshCreator->DeleteRoomFactoryTemplates ();
  roomFactories.DeleteAll ();

  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (object_reg);
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  csRef<iDocument> doc = docsys->CreateDocument ();
  csRef<iDataBuffer> buf = vfs->ReadFile (filename);
  const char* error = doc->Parse (buf->GetData ());
  if (error)
  {
    printf ("ERROR: %s\n", error); fflush (stdout);
    return false;
  }

  return LoadDoc (doc);
}

csRef<iDocument> WorldLoader::SaveDoc ()
{
  csRef<iDocumentSystem> docsys;
  docsys.AttachNew (new csTinyDocumentSystem ());
  csRef<iDocument> doc = docsys->CreateDocument ();

  csRef<iDocumentNode> root = doc->CreateRoot ();
  csRef<iDocumentNode> rootNode = root->CreateNodeBefore (CS_NODE_ELEMENT);
  rootNode->SetValue ("dynlevel");

  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    const Asset& asset = assets[i];
    csRef<iDocumentNode> assetNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
    assetNode->SetValue ("asset");
    assetNode->SetAttribute ("path", asset.GetPath ());
    assetNode->SetAttribute ("file", asset.GetFile ());
  }

  csRef<iDocumentNode> dynworldNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
  dynworldNode->SetValue ("dynworld");
  dynworld->Save (dynworldNode);

  csRef<iDocumentNode> curveNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
  curveNode->SetValue ("curves");
  curvedMeshCreator->Save (curveNode);

  csRef<iDocumentNode> roomNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
  roomNode->SetValue ("rooms");
  roomMeshCreator->Save (roomNode);
  return doc;
}

bool WorldLoader::SaveFile (const char* filename)
{
  csRef<iDocument> doc = SaveDoc ();

  csRef<iString> xml;
  xml.AttachNew (new scfString ());
  doc->Write (xml);
  vfs->WriteFile (filename, xml->GetData (), xml->Length ());
  return true;
}

