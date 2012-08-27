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

#include "aresextern.h"
#include "propclass/dynworld.h"

struct iCurvedMeshCreator;
struct iRoomMeshCreator;

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

class ARES_EXPORT BaseAsset
{
public:
  csString file;	// A simple filename ('library', 'world', ...)
  csString normPath;	// Normalized path for the file.
  csString mountPoint;	// If given then this mount point must be used.
  bool saveDynfacts;
  bool saveTemplates;
  bool saveQuests;
  bool saveLightFacts;

  BaseAsset (const char* file,
      bool saveDynfacts, bool saveTemplates, bool saveQuests,
      bool saveLightFacts) :
    file (file),
    saveDynfacts (saveDynfacts), saveTemplates (saveTemplates),
    saveQuests (saveQuests), saveLightFacts (saveLightFacts)
  { }

  void SetNormalizedPath (const char* p) { normPath = p; }
  void SetMountPoint (const char* m) { mountPoint = m; }

  const csString& GetFile () const { return file; }
  const csString& GetNormalizedPath () const { return normPath; }
  const csString& GetMountPoint () const { return mountPoint; }

  void SetDynfactSavefile (bool e) { saveDynfacts = e; }
  bool IsDynfactSavefile () const { return saveDynfacts; }
  void SetTemplateSavefile (bool e) { saveTemplates = e; }
  bool IsTemplateSavefile () const { return saveTemplates; }
  void SetQuestSavefile (bool e) { saveQuests = e; }
  bool IsQuestSavefile () const { return saveQuests; }
  void SetLightFactSaveFile (bool e) { saveLightFacts = e; }
  bool IsLightFactSaveFile () const { return saveLightFacts; }
};

/**
 * A representation of an asset.
 */
struct iAsset : public virtual iBase
{
  //void SetNormalizedPath (const char* p) { normPath = p; }
  //void SetMountPoint (const char* m) { mountPoint = m; }

  virtual const csString& GetFile () const = 0;
  virtual const csString& GetNormalizedPath () const = 0;
  virtual const csString& GetMountPoint () const = 0;

  //void SetDynfactSavefile (bool e) { saveDynfacts = e; }
  virtual bool IsDynfactSavefile () const = 0;

  //void SetTemplateSavefile (bool e) { saveTemplates = e; }
  virtual bool IsTemplateSavefile () const = 0;

  //void SetQuestSavefile (bool e) { saveQuests = e; }
  virtual bool IsQuestSavefile () const = 0;

  //void SetLightFactSaveFile (bool e) { saveLightFacts = e; }
  virtual bool IsLightFactSaveFile () const = 0;
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
};

#endif // __ARES_ASSETMANAGER_H__

