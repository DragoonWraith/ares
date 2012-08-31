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

#ifndef __ARES_ASSETMANAGER_H__
#define __ARES_ASSETMANAGER_H__

#include "propclass/dynworld.h"

struct iCurvedMeshCreator;
struct iRoomMeshCreator;
struct iCollection;

// The asset manager knows four kinds of paths:
// - VFS paths: this is a path on the VFS file system
// - Real paths: this is a path on the real filesystem. It is OS
//   dependend (\ or /)
// - Mountable path: this is a real path where all \ or / have been
//   converted to $/ so that it can be used with vfs->Mount.
// - Normalized path: this is a normalized real path. It is normalized in
//   two ways: if it is a path relative to the asset path (the list of
//   real directories that are mounted to /assets) then this asset path
//   will be indicated with $#. In addition slashes are all converted
//   to '/'

class BaseAsset
{
public:
  csString file;	// A simple filename ('library', 'world', ...)
  csString normPath;	// Normalized path for the file.
  csString mountPoint;	// If given then this mount point must be used.
  bool writable;

  BaseAsset (const char* file, bool writable) :
    file (file), writable (writable)
  { }

  void SetNormalizedPath (const char* p) { normPath = p; }
  void SetMountPoint (const char* m) { mountPoint = m; }

  const csString& GetFile () const { return file; }
  const csString& GetNormalizedPath () const { return normPath; }
  const csString& GetMountPoint () const { return mountPoint; }

  void SetWritable (bool e) { writable = e; }
  bool IsWritable () const { return writable; }
};

/**
 * A representation of an asset.
 */
struct iAsset : public virtual iBase
{
  virtual const csString& GetFile () const = 0;
  virtual const csString& GetNormalizedPath () const = 0;
  virtual const csString& GetMountPoint () const = 0;

  virtual bool IsWritable () const = 0;
  virtual bool IsModified () const = 0;
  virtual iCollection* GetCollection () const = 0;
};

/**
 * The asset manager.
 */
struct iAssetManager : public virtual iBase
{
  /**
   * Set the zone on which to operate.
   */
  virtual void SetZone (iPcDynamicWorld* dynworld) = 0;

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
    bool use_first_if_not_found = false) = 0;

  /**
   * Load a file from a path and set the parsed document or null on error.
   * In case of error an error string is also returned.
   * Note that a non-existing file is not an error. In that case doc will be null
   * but the error string will be empty.
   */
  virtual csPtr<iString> LoadDocument (iObjectRegistry* object_reg,
      csRef<iDocument>& doc,
      const char* vfspath, const char* file) = 0;

  /**
   * Get the currently loaded assets.
   */
  virtual const csRefArray<iAsset>& GetAssets () const = 0;

  /**
   * Load the world from a file.
   */
  virtual bool LoadFile (const char* filename) = 0;

  /**
   * Save the world to a file.
   */
  virtual bool SaveFile (const char* filename) = 0;

  /**
   * Create a new project empty project.
   */
  virtual bool NewProject () = 0;

  /**
   * Update the assets of the current project.
   */
  virtual bool UpdateAssets (const csArray<BaseAsset>& newassets) = 0;

  virtual const csArray<iDynamicFactory*> GetCurvedFactories () const = 0;
  virtual const csArray<iDynamicFactory*> GetRoomFactories () const = 0;

  /**
   * Check if a resource can be modified. If it is part of a readonly asset
   * then this function will return false.
   */
  virtual bool IsModifiable (iObject* resource) = 0;

  /**
   * Return true if a resource is marked as being modified.
   */
  virtual bool IsModified (iObject* resource) = 0;

  /**
   * Return true if there is a modification in general.
   */
  virtual bool IsModified () = 0;

  /**
   * Register modified resource. Use this function to register that
   * a resource has been modified or created. This function does not
   * check if a resource can be modified (use IsModifiable() for that).
   * This function returns false if the asset manager doesn't know
   * where to put this resource.
   */
  virtual bool RegisterModification (iObject* resource) = 0;

  /**
   * Register a modification in general. This is useful if something
   * outside of the regular assets is modified (like one of the dynamic objects).
   */
  virtual void RegisterModification () = 0;

  /**
   * Register the removal of some asset.
   */
  virtual void RegisterRemoval (iObject* resource) = 0;

  /**
   * Place a resource with some asset. If the 'asset' is 0 then the asset
   * manager will remember that the user specifically requested the resource not
   * to belong in any asset.
   */
  virtual void PlaceResource (iObject* resource, iAsset* asset) = 0;

  /**
   * Get the asset for this resource or 0 if the asset manager doesn't
   * know where the resource belongs.
   */
  virtual iAsset* GetAssetForResource (iObject* resource) = 0;

  /**
   * Is this a resource without an asset? This returns true if this
   * resource has been explicitelly registered as not having an asset
   * (PlaceResource() with asset parameter 0). This function returns false
   * if the resource is placed with an asset. This function also returns
   * false if the resouce is not placed with an asset but it was not marked
   * as not requiring an asset.
   */
  virtual bool IsResourceWithoutAsset (iObject* resource) = 0;
};

#endif // __ARES_ASSETMANAGER_H__

