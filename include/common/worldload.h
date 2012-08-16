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

#ifndef __aresed_worldload_h
#define __aresed_worldload_h

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

class ARES_EXPORT Asset
{
private:
  csString file;	// A simple filename ('library', 'world', ...)
  csString normPath;	// Normalized path for the file.
  csString mountPoint;	// If given then this mount point must be used.
  bool saveDynfacts;
  bool saveTemplates;
  bool saveQuests;
  bool saveLightFacts;

public:
  Asset (const char* file,
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

class ARES_EXPORT WorldLoader
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
  csArray<Asset> assets;

  csArray<iDynamicFactory*> curvedFactories;
  csArray<iDynamicFactory*> roomFactories;

  csRef<iDocument> SaveDoc ();
  bool LoadDoc (iDocument* doc);
  bool LoadLibrary (const char* path, const char* file);

  bool SaveAsset (iDocumentSystem* docsys, const Asset& asset);
  bool HasAsset (const Asset& a);
  bool LoadAsset (const csString& normpath, const csString& file, const csString& mount);

public:
  WorldLoader (iObjectRegistry* object_reg);

  /**
   * Set the zone on which to operate.
   */
  void SetZone (iPcDynamicWorld* dynworld)
  {
    WorldLoader::dynworld = dynworld;
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
  static csString FindAsset (iStringArray* assets, const char* filename,
    bool use_first_if_not_found = false);

  /**
   * Load a file from a path and set the parsed document or null on error.
   * In case of error an error string is also returned.
   * Note that a non-existing file is not an error. In that case doc will be null
   * but the error string will be empty.
   */
  static csString LoadDocument (iObjectRegistry* object_reg,
      csRef<iDocument>& doc,
      const char* vfspath, const char* file);

  /**
   * Get the currently loaded assets.
   */
  const csArray<Asset>& GetAssets () const { return assets; }

  /**
   * Load the world from a file.
   */
  bool LoadFile (const char* filename);

  /**
   * Save the world to a file.
   */
  bool SaveFile (const char* filename);

  /**
   * Create a new project with the given assets.
   */
  bool NewProject (const csArray<Asset>& newassets);

  /**
   * Update the assets of the current project.
   */
  bool UpdateAssets (const csArray<Asset>& newassets);

  const csArray<iDynamicFactory*> GetCurvedFactories () const { return curvedFactories; }
  const csArray<iDynamicFactory*> GetRoomFactories () const { return roomFactories; }
};

#endif // __aresed_worldload_h

