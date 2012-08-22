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

#ifndef __appares_newproject_h
#define __appares_newproject_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>
#include <wx/dirctrl.h>

class UIManager;
class Asset;

struct NewProjectCallback : public csRefCount
{
  virtual void OkPressed (const csArray<Asset>& assets) = 0;
};

class NewProjectDialog : public wxDialog
{
private:
  iObjectRegistry* object_reg;
  UIManager* uiManager;
  iVFS* vfs;
  csStringArray assetPath;
  csString appDir;
  csString currentPath;

  void OnOkButton (wxCommandEvent& event);
  void OnCancelButton (wxCommandEvent& event);
  void OnAddAssetButton (wxCommandEvent& event);
  void OnDelAssetButton (wxCommandEvent& event);
  void OnUpdateAssetButton (wxCommandEvent& event);
  void OnMoveUpButton (wxCommandEvent& event);
  void OnMoveDownButton (wxCommandEvent& event);
  void OnAssetSelected (wxListEvent& event);
  void OnAssetDeselected (wxListEvent& event);
  void OnDirSelChange (wxCommandEvent& event);
  void OnPathSelected (wxCommandEvent& event);

  long selIndex;
  csRef<NewProjectCallback> callback;

  void SetPathFile (const char* file,
      bool saveDynfacts, bool saveTemplates, bool saveQuests,
      bool saveLights, const char* normPath, const char* mount);
  void ScanLoadableFile (const char* path, const char* file);
  void ScanCSNode (csString& msg, iDocumentNode* node);
  void UpdateAsset (int idx, const char* file,
      bool dynfacts, bool templates, bool quests, bool lights,
      const char* normPath, const char* mount);
  void AddAsset (const char* file,
      bool dynfacts, bool templates, bool quests, bool lights,
      const char* normPath, const char* mount);

  /**
   * Try to find the manifest file and see what kind of information can
   * be found in the asset.
   * If 'override' is true then the information from the manifest can potentially
   * override the values already set in the 'file' and 'mount' widgets.
   */
  void LoadManifest (const char* path, const char* file, bool override);

  /**
   * Construct a path that is mountable.
   * 'path' and 'filePath' are the path and filePath that you get from
   * a wxGenericDirCtrl. This means that filePath will be empty if it is a directory
   * and otherwise they will be the same.
   */
  csString ConstructMountString (const char* path, const char* filePath, csString& file);

  /**
   * Construct a relative path (to asset dir(s)) that can be saved in the project file.
   * 'path' and 'filePath' are the path and filePath that you get from
   * a wxGenericDirCtrl. This means that filePath will be empty if it is a directory
   * and otherwise they will be the same.
   */
  csString ConstructRelativePath (const char* path, const char* filePath);

  void Setup (NewProjectCallback* cb);

  /**
   * Enable/disable buttons that have to be enabled when an asset is selected.
   */
  void EnableAssetButtons (bool e);

public:
  NewProjectDialog (wxWindow* parent, iObjectRegistry* object_reg, UIManager* uiManager, iVFS* vfs);
  ~NewProjectDialog();

  void Show (NewProjectCallback* cb);
  void Show (NewProjectCallback* cb, const csArray<Asset>& assets);
  //void SetFilename (const char* filename);

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_newproject_h

