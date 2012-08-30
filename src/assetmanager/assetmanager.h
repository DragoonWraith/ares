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

#ifndef __ASSETMANAGER_H__
#define __ASSETMANAGER_H__

#include "iassetmanager.h"

struct iCurvedMeshCreator;
struct iRoomMeshCreator;
struct iCollection;

class IntAsset : public scfImplementation1<IntAsset,iAsset>
{
private:
  csString file;	// A simple filename ('library', 'world', ...)
  csString normPath;	// Normalized path for the file.
  csString mountPoint;	// If given then this mount point must be used.
  bool writable;
  csRef<iCollection> collection;
  bool modified;	// True if modified. Can be true even if there are no
  			// modified resources because a resource could have been deleted.
  csSet<csPtrKey<iObject> > modifiedResources;

public:
  IntAsset (const char* file, bool writable) :
    scfImplementationType (this),
    file (file), writable (writable), modified (false)
  { }
  virtual ~IntAsset () { }

  void SetCollection (iCollection* col) { collection = col; }
  virtual iCollection* GetCollection () const { return collection; }

  void SetNormalizedPath (const char* p) { normPath = p; }
  void SetMountPoint (const char* m) { mountPoint = m; }

  virtual const csString& GetFile () const { return file; }
  virtual const csString& GetNormalizedPath () const { return normPath; }
  virtual const csString& GetMountPoint () const { return mountPoint; }

  void SetWritable (bool e) { writable = e; }
  virtual bool IsWritable () const { return writable; }

  void SetModified (bool m) { modified = m; }
  virtual bool IsModified () const { return modified; }
  csSet<csPtrKey<iObject> >& GetModifiedResources () { return modifiedResources; }
};

class AssetManager : public scfImplementation2<AssetManager,iAssetManager,iComponent>
{
private:
  iObjectRegistry* object_reg;
  csRef<iLoader> loader;
  csRef<iVFS> vfs;
  csRef<iEngine> engine;
  csRef<iCurvedMeshCreator> curvedMeshCreator;
  csRef<iRoomMeshCreator> roomMeshCreator;
  csRef<iPcDynamicWorld> dynworld;

  int mntCounter;
  csRefArray<iAsset> assets;
  int colCounter;

  bool generallyModified;	// A general modification outside of an asset has occured.

  csArray<iDynamicFactory*> curvedFactories;
  csArray<iDynamicFactory*> roomFactories;

  csRef<iDocument> SaveDoc ();
  bool LoadDoc (iDocument* doc);
  bool LoadLibrary (const char* path, const char* file, iCollection* collection);

  bool SaveAsset (iDocumentSystem* docsys, iAsset* asset);
  iAsset* HasAsset (const BaseAsset& a);
  bool LoadAsset (const csString& normpath, const csString& file, const csString& mount,
      iCollection* collection);

  /**
   * Find a suitable asset to save this resource. Returns 0
   * if there is none (more then one possibility). Otherwise the
   * resource is attached to the asset.
   */
  IntAsset* FindSuitableAsset (iObject* resource);

  /**
   * Find an asset for this resource. If needed and possible the resource will
   * be assigned to an asset. If this fails this function returns 0.
   */
  IntAsset* FindAssetForResource (iObject* resource);

  /**
   * Find the asset representing this collection.
   */
  IntAsset* FindAssetForCollection (iCollection* collection);

public:
  AssetManager (iBase* parent);
  virtual ~AssetManager () { }

  virtual bool Initialize (iObjectRegistry* object_reg);

  /**
   * Set the zone on which to operate.
   */
  virtual void SetZone (iPcDynamicWorld* dynworld)
  {
    AssetManager::dynworld = dynworld;
  }

  /**
   * Find an asset in the asset path. The filename should be a noramlized
   * path. If use_first_if_not_found is given then this function will use
   * the first location in the asset path if no file was found on the path.
   * This is useful when saving new assets.
   * This function returns a mountable path where the appropriate prefix
   * path from the asset path has been prepended. Or empty string if
   * the file could not be found on the path (only if use_first_if_not_found == false).
   */
  virtual csPtr<iString> FindAsset (iStringArray* assets, const char* filename,
    bool use_first_if_not_found = false);

  /**
   * Load a file from a path and set the parsed document or null on error.
   * In case of error an error string is also returned.
   * Note that a non-existing file is not an error. In that case doc will be null
   * but the error string will be empty.
   */
  virtual csPtr<iString> LoadDocument (iObjectRegistry* object_reg,
      csRef<iDocument>& doc,
      const char* vfspath, const char* file);

  /**
   * Get the currently loaded assets.
   */
  virtual const csRefArray<iAsset>& GetAssets () const { return assets; }

  /**
   * Load the world from a file.
   */
  virtual bool LoadFile (const char* filename);

  /**
   * Save the world to a file.
   */
  virtual bool SaveFile (const char* filename);

  /**
   * Create a new project with the given assets.
   */
  virtual bool NewProject ();

  /**
   * Update the assets of the current project.
   */
  virtual bool UpdateAssets (const csArray<BaseAsset>& newassets);

  virtual const csArray<iDynamicFactory*> GetCurvedFactories () const { return curvedFactories; }
  virtual const csArray<iDynamicFactory*> GetRoomFactories () const { return roomFactories; }

  virtual bool IsModifiable (iObject* resource);
  virtual bool IsModified (iObject* resource);
  virtual bool IsModified ();
  virtual bool RegisterModification (iObject* resource);
  virtual void RegisterModification ();
  virtual void RegisterRemoval (iObject* resource);
  virtual void PlaceResource (iObject* resource, iAsset* asset);
  virtual iAsset* GetAssetForResource (iObject* resource);
};

#endif // __ASSETMANAGER_H__

