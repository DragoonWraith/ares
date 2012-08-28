/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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
#include "assetmanager.h"

#include "propclass/dynworld.h"
#include "physicallayer/pl.h"
#include "physicallayer/entitytpl.h"
#include "tools/questmanager.h"
#include "tools/dynworldload.h"

SCF_IMPLEMENT_FACTORY (AssetManager)

AssetManager::AssetManager (iBase* parent) : scfImplementationType (this, parent)
{
}

bool AssetManager::Initialize (iObjectRegistry* object_reg)
{
  AssetManager::object_reg = object_reg;
  loader = csQueryRegistry<iLoader> (object_reg);
  vfs = csQueryRegistry<iVFS> (object_reg);
  engine = csQueryRegistry<iEngine> (object_reg);
  curvedMeshCreator = csQueryRegistry<iCurvedMeshCreator> (object_reg);
  roomMeshCreator = csQueryRegistry<iRoomMeshCreator> (object_reg);
  mntCounter = 0;
  colCounter = 0;
  return true;
}

bool AssetManager::LoadLibrary (const char* path, const char* file, iCollection* collection)
{
  // Set current VFS dir to the level dir, helps with relative paths in maps
  vfs->PushDir (path);
  csLoadResult rc = loader->Load (file, collection);
  if (!rc.success)
  {
    vfs->PopDir ();
    //@@@return ReportError("Couldn't load library file %s!", path);
    return false;
  }
  vfs->PopDir ();
  return true;
}

csPtr<iString> AssetManager::LoadDocument (iObjectRegistry* object_reg,
    csRef<iDocument>& doc,
    const char* vfspath, const char* file)
{
  doc.Invalidate ();
  csRef<iVFS> vfs = csQueryRegistry<iVFS> (object_reg);
  if (vfspath) vfs->PushDir (vfspath);

  csRef<iDataBuffer> buf = vfs->ReadFile (file);
  if (vfspath) vfs->PopDir ();
  if (!buf)
    return 0;	// No error, just a non-existing file.

  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (object_reg);
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  doc = docsys->CreateDocument ();
  const char* error = doc->Parse (buf->GetData ());
  if (error)
  {
    scfString* msg = new scfString ();;
    msg->Format ("Can't parse '%s': %s", file, error);
    doc.Invalidate ();
    return msg;
  }

  return 0;
}

csPtr<iString> AssetManager::FindAsset (iStringArray* assets, const char* filename,
    bool use_first_if_not_found)
{
  csString path;
  if (csString (filename).StartsWith ("$#"))
  {
    filename += 2;

    // Trick: we go to == assets size in order to make sure we pick
    // the first location when we cannot find the file.
    for (size_t i = 0 ; i <= assets->GetSize () ; i++)
    {
      if (i == assets->GetSize () && !use_first_if_not_found) return 0;
      path = assets->Get (i % assets->GetSize ());	// Make sure to wrap around
      if (path[path.Length ()-1] != '\\' && path[path.Length ()-1] != '/')
	path += CS_PATH_SEPARATOR;
      path += filename;
      if (CS_PATH_SEPARATOR != '/')
      {
	csString sep;
	sep = CS_PATH_SEPARATOR;
	path.ReplaceAll ("/", sep);
      }
      if (i == assets->GetSize () && use_first_if_not_found) break;
      struct stat buf;
      csString sp;
      if (path[path.Length ()-1] == '\\' || path[path.Length ()-1] == '/')
	sp = path.Slice (0, path.Length ()-1);
      else
	sp = path;
      if (stat (sp, &buf) == 0)
      {
	if (S_ISREG (buf.st_mode) || S_ISDIR (buf.st_mode))
	  break;
      }
    }
  }
  else
    path = filename;
  path.ReplaceAll ("/", "$/");
  return new scfString (path);
}

bool AssetManager::LoadDoc (iDocument* doc)
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
      csString normpath = child->GetAttributeValue ("path");
      csString file = child->GetAttributeValue ("file");
      csString mount = child->GetAttributeValue ("mount");
      bool writable = child->GetAttributeValueAsBool ("writable");
      csString colName;
      colName.Format ("__col__%d__", colCounter++);
      iCollection* collection = engine->CreateCollection (colName);
      LoadAsset (normpath, file, mount, collection);

      csRef<IntAsset> asset;
      asset.AttachNew (new IntAsset (file, writable));
      asset->SetMountPoint (mount);
      asset->SetNormalizedPath (normpath);
      asset->SetCollection (collection);
      assets.Push (asset);
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
      fflush (stdout);
      return false;
    }
  }
  return true;
}

bool AssetManager::LoadFile (const char* filename)
{
  assets.DeleteAll ();

  curvedMeshCreator->DeleteFactories ();
  curvedMeshCreator->DeleteCurvedFactoryTemplates ();
  curvedFactories.DeleteAll ();

  roomMeshCreator->DeleteFactories ();
  roomMeshCreator->DeleteRoomFactoryTemplates ();
  roomFactories.DeleteAll ();

  csRef<iDocument> doc;
  csRef<iString> error = LoadDocument (object_reg, doc, 0, filename);
  if (!doc && !error)
  {
    error.AttachNew (new scfString ());
    error->Format ("ERROR reading file '%s'", filename);
  }
  else
    return LoadDoc (doc);

  printf ("%s\n", error->GetData ());
  return false;
}

bool AssetManager::LoadAsset (const csString& normpath, const csString& file, const csString& mount,
    iCollection* collection)
{
  csRef<iString> path;
  if (!normpath.IsEmpty ())
  {
    csRef<iStringArray> assetPath = vfs->GetRealMountPaths ("/assets/");
    path = FindAsset (assetPath, normpath);
    if (!path)
    {
      // @@@ Proper reporting
      printf ("Cannot find asset '%s' in the asset path!\n", normpath.GetData ());
      return false;
    }
  }

  csString rmount;
  if (mount.IsEmpty ())
  {
    rmount.Format ("/assets/__mnt_%d__/", mntCounter);
    mntCounter++;
  }
  else
  {
    rmount = mount;
  }

  if (path)
  {
    vfs->Mount (rmount, path->GetData ());
    printf ("Mounting '%s' to '%s'\n", path->GetData (), rmount.GetData ());
  }

  vfs->PushDir (rmount);
  // If the file doesn't exist we don't try to load it. That's not an error
  // as it might be saved later.
  bool exists = vfs->Exists (file);
  vfs->PopDir ();
  if (exists)
  {
    if (!LoadLibrary (rmount, file, collection))
      return false;
  }
  else
  {
    printf ("Warning! File '%s/%s' does not exist!\n",
	(!path) ? rmount.GetData () : path->GetData (), file.GetData ());
  }

  //if (!path.IsEmpty ())
    //vfs->Unmount (rmount, path);
  return true;
}

bool AssetManager::NewProject ()
{
  // @@@ Should this also unload all loaded data? Probably yes.
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    IntAsset* ia = static_cast<IntAsset*> (assets[i]);
    if (ia->GetCollection ())
      engine->RemoveCollection (ia->GetCollection ());
  }
  assets.DeleteAll ();
  return true;
}

iAsset* AssetManager::HasAsset (const BaseAsset& a)
{
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    if (assets[i]->GetNormalizedPath () == a.GetNormalizedPath () &&
	assets[i]->GetFile () == a.GetFile () &&
	assets[i]->GetMountPoint () == a.GetMountPoint ())
    {
      // Update the asset we have with the new flags.
      IntAsset* ia = static_cast<IntAsset*> (assets[i]);
      ia->SetWritable (a.IsWritable ());
      return assets[i];
    }
  }
  return 0;
}

bool AssetManager::UpdateAssets (const csArray<BaseAsset>& update)
{
  // @@@ Removing assets is not yet supported. At least they will not get unloaded.
  // The assets table will be updated however. So a Save/Load will remove the asset.
  csRefArray<iAsset> newassets;

  // @@@ Remove the collection for removed assets!

  for (size_t i = 0 ; i < update.GetSize () ; i++)
  {
    const BaseAsset& a = update[i];
    iAsset* currentAsset = HasAsset (a);
    if (currentAsset)
    {
      newassets.Push (currentAsset);
    }
    else
    {
      csString normpath = a.GetNormalizedPath ();
      csString file = a.GetFile ();
      csString mount = a.GetMountPoint ();
      csString colName;
      colName.Format ("__col__%d__", colCounter++);
      iCollection* collection = engine->CreateCollection (colName);
      if (!LoadAsset (normpath, file, mount, collection))
	return false;

      csRef<IntAsset> asset;
      asset.AttachNew (new IntAsset (file, a.IsWritable ()));
      asset->SetMountPoint (mount);
      asset->SetNormalizedPath (normpath);
      asset->SetCollection (collection);
      newassets.Push (asset);
    }
  }
  assets = newassets;

  return true;
}

bool AssetManager::SaveAsset (iDocumentSystem* docsys, iAsset* asset)
{
  csRef<iDocument> docasset = docsys->CreateDocument ();
  IntAsset* ia = static_cast<IntAsset*> (asset);
  iCollection* collection = ia->GetCollection ();

  csRef<iDocumentNode> root = docasset->CreateRoot ();
  csRef<iDocumentNode> rootNode = root->CreateNodeBefore (CS_NODE_ELEMENT);
  rootNode->SetValue ("library");

  {
    csRef<iSaver> saver = csQueryRegistryOrLoad<iSaver> (object_reg,
	"crystalspace.level.saver");
    if (!saver)
    {
      printf ("ERROR! Saver plugin is missing. Cannot save!\n");
      return false;
    }
    if (!saver->SaveLightFactories (collection, rootNode))
    {
      printf ("ERROR! Error saving light factories!\n");
      return false;
    }
  }

  {
    csRef<iQuestManager> questmgr = csQueryRegistryOrLoad<iQuestManager> (object_reg,
	"cel.manager.quests");
    if (!questmgr) return false;
    csRef<iDocumentNode> addonNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
    addonNode->SetValue ("addon");
    addonNode->SetAttribute ("plugin", "cel.addons.questdef");
    if (!questmgr->Save (addonNode, collection))
      return false;
  }

  {
    csRef<iDynamicWorldSaver> saver = csLoadPluginCheck<iDynamicWorldSaver> (object_reg,
	"cel.addons.dynamicworld.loader");
    if (!saver) return false;
    csRef<iDocumentNode> addonNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
    addonNode->SetValue ("addon");
    addonNode->SetAttribute ("plugin", "cel.addons.dynamicworld.loader");
    if (!saver->WriteFactories (dynworld, addonNode, collection))
      return false;
  }

  {
    csRef<iSaverPlugin> saver = csLoadPluginCheck<iSaverPlugin> (object_reg,
	"cel.addons.celentitytpl");
    if (!saver) return 0;
    csRef<iCelPlLayer> pl = csQueryRegistry<iCelPlLayer> (object_reg);
    csRef<iCelEntityTemplateIterator> tempIt = pl->GetEntityTemplates ();
    while (tempIt->HasNext ())
    {
      iCelEntityTemplate* temp = tempIt->Next ();
      if (!collection || collection->IsParentOf (temp->QueryObject ()))
      {
        csString tempName = temp->GetName ();
        csRef<iDocumentNode> addonNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
        addonNode->SetValue ("addon");
        addonNode->SetAttribute ("plugin", "cel.addons.celentitytpl");
        if (!saver->WriteDown (temp, addonNode, 0))
	  return false;
      }
    }
  }


  csRef<iString> xml;
  xml.AttachNew (new scfString ());
  docasset->Write (xml);

  // In order to control the exact location to save we make a new temporary mount
  // point where we can save. That's to avoid the problem where an asset comes from
  // a location which is mounted on different real paths.
  // If the asset cannot be found yet then we will save to the first location on the path.
  csString normpath = asset->GetNormalizedPath ();
  if (normpath.IsEmpty ())
  {
    printf ("Writing '%s' at '%s\n", asset->GetFile ().GetData (), asset->GetMountPoint ().GetData ());
    vfs->PushDir (asset->GetMountPoint ());
    vfs->WriteFile (asset->GetFile (), xml->GetData (), xml->Length ());
    vfs->PopDir ();
  }
  else
  {
    printf ("Writing '%s' at '%s\n", asset->GetFile ().GetData (), asset->GetNormalizedPath ().GetData ());
    csRef<iStringArray> assetPath = vfs->GetRealMountPaths ("/assets/");
    csRef<iString> path = FindAsset (assetPath, normpath, true);

    vfs->Mount ("/assets/__mnt_wl__", path->GetData ());
    vfs->PushDir ("/assets/__mnt_wl__");
    vfs->WriteFile (asset->GetFile (), xml->GetData (), xml->Length ());
    vfs->PopDir ();
    vfs->Unmount ("/assets/__mnt_wl__", path->GetData ());
  }
  return true;
}

csRef<iDocument> AssetManager::SaveDoc ()
{
  csRef<iDocumentSystem> docsys;
  docsys.AttachNew (new csTinyDocumentSystem ());
  csRef<iDocument> doc = docsys->CreateDocument ();

  csRef<iDocumentNode> root = doc->CreateRoot ();
  csRef<iDocumentNode> rootNode = root->CreateNodeBefore (CS_NODE_ELEMENT);
  rootNode->SetValue ("dynlevel");

  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    iAsset* asset = assets[i];
    csRef<iDocumentNode> assetNode = rootNode->CreateNodeBefore (CS_NODE_ELEMENT);
    assetNode->SetValue ("asset");
    if (!asset->GetNormalizedPath ().IsEmpty ())
      assetNode->SetAttribute ("path", asset->GetNormalizedPath ());
    assetNode->SetAttribute ("file", asset->GetFile ());
    if (!asset->GetMountPoint ().IsEmpty ())
      assetNode->SetAttribute ("mount", asset->GetMountPoint ());
    if (asset->IsWritable ())
      assetNode->SetAttribute ("writable", "true");
    if (!asset->GetMountPoint ().IsEmpty ())
      assetNode->SetAttribute ("mount", asset->GetMountPoint ());
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

  // Now save all assets in their respective files. @@@ In the future this should
  // be modified to only save the new assets and assets that actually came from here.
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    iAsset* asset = assets[i];
    if (asset->IsWritable ())
    {
      // @@@ Todo: proper error reporting.
      if (!SaveAsset (docsys, asset))
	return 0;
    }
  }

  return doc;
}

bool AssetManager::SaveFile (const char* filename)
{
  csRef<iDocument> doc = SaveDoc ();

printf ("Writing '%s' at '%s\n", filename, vfs->GetCwd ());
  csRef<iString> xml;
  xml.AttachNew (new scfString ());
  doc->Write (xml);
  vfs->WriteFile (filename, xml->GetData (), xml->Length ());
  return true;
}

bool AssetManager::IsModifiable (iObject* resource)
{
  iObject* parent = resource->GetObjectParent ();
  if (!parent) return true;	// Not in any asset, so it can be modified.
  csRef<iCollection> collection = scfQueryInterface<iCollection> (parent);
  if (!collection) return true;	// Parent is not a collection, so it can be modified.
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    IntAsset* ia = static_cast<IntAsset*> (assets[i]);
    if (ia->GetCollection () == collection)
      return ia->IsWritable ();
  }
  return true;	// Couldn't find resource. Assume it can be modified.
}

IntAsset* AssetManager::FindSuitableAsset (iObject* resource)
{
  IntAsset* writableAsset = 0;
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    IntAsset* ia = static_cast<IntAsset*> (assets[i]);
    if (ia->IsWritable ())
    {
      if (writableAsset)
	return 0;
      writableAsset = ia;
    }
  }
  if (!writableAsset) return 0;
  writableAsset->GetCollection ()->Add (resource);
  return writableAsset;
}

IntAsset* AssetManager::FindAssetForResource (iObject* resource)
{
  iObject* parent = resource->GetObjectParent ();
  if (!parent) return FindSuitableAsset (resource);
  csRef<iCollection> collection = scfQueryInterface<iCollection> (parent);
  if (!collection) return FindSuitableAsset (resource);	// @@@Can this actually happen?
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    IntAsset* ia = static_cast<IntAsset*> (assets[i]);
    if (ia->GetCollection () == collection)
      return ia;
  }
  return 0;
}

bool AssetManager::RegisterModification (iObject* resource)
{
  IntAsset* asset = FindAssetForResource (resource);
  if (asset)
  {
    // @@@ Register modified.
    return true;
  }
  return false;
}

void AssetManager::PlaceResource (iObject* resource, iAsset* asset)
{
  IntAsset* ia = static_cast<IntAsset*> (asset);
  ia->GetCollection ()->Add (resource);
}

iAsset* AssetManager::GetAssetForResource (iObject* resource)
{
  iObject* parent = resource->GetObjectParent ();
  if (!parent) return 0;
  csRef<iCollection> collection = scfQueryInterface<iCollection> (parent);
  if (!collection) return 0;
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    IntAsset* ia = static_cast<IntAsset*> (assets[i]);
    if (ia->GetCollection () == collection)
      return ia;
  }
  return 0;
}

