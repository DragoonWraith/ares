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

class ARES_EXPORT Asset
{
private:
  csString path;
  csString file;
  bool saveDynfacts;
  bool saveTemplates;
  bool saveQuests;
  bool saveLightFacts;

public:
  Asset (const char* path, const char* file,
      bool saveDynfacts, bool saveTemplates, bool saveQuests,
      bool saveLightFacts) :
    path (path), file (file),
    saveDynfacts (saveDynfacts), saveTemplates (saveTemplates),
    saveQuests (saveQuests), saveLightFacts (saveLightFacts)
  { }

  const csString& GetPath () const { return path; }
  const csString& GetFile () const { return file; }

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

  csArray<Asset> assets;

  csArray<iDynamicFactory*> curvedFactories;
  csArray<iDynamicFactory*> roomFactories;

  csRef<iDocument> SaveDoc ();
  bool LoadDoc (iDocument* doc);
  bool LoadLibrary (const char* path, const char* file);

  bool SaveAsset (iDocumentSystem* docsys, const Asset& asset);
  bool HasAsset (const Asset& a);

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

