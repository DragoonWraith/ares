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

#include "manageassets.h"
#include "uimanager.h"
#include "edcommon/listctrltools.h"
#include "../apparesed.h"
#include "edcommon/uitools.h"
#include "iassetmanager.h"

#include <wx/html/htmlwin.h>

#define ASSET2_COL_PATH 0
#define ASSET2_COL_FILE 1
#define ASSET2_COL_MODIFIED 2
#define ASSET2_COL_WRITE 3
#define ASSET2_COL_MOUNT 4

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ManageAssetsDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), ManageAssetsDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), ManageAssetsDialog :: OnCancelButton)
  EVT_BUTTON (XRCID("addAssetButton"), ManageAssetsDialog :: OnAddAssetButton)
  EVT_BUTTON (XRCID("delAssetButton"), ManageAssetsDialog :: OnDelAssetButton)
  EVT_BUTTON (XRCID("updateButton"), ManageAssetsDialog :: OnUpdateAssetButton)
  EVT_BUTTON (XRCID("moveUpButton"), ManageAssetsDialog :: OnMoveUpButton)
  EVT_BUTTON (XRCID("moveDownButton"), ManageAssetsDialog :: OnMoveDownButton)
  EVT_COMBOBOX (XRCID("path_Combo"), ManageAssetsDialog :: OnPathSelected)
  EVT_LIST_ITEM_SELECTED (XRCID("assetListCtrl"), ManageAssetsDialog :: OnAssetSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("assetListCtrl"), ManageAssetsDialog :: OnAssetDeselected)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

csString ManageAssetsDialog::ConstructMountString (const char* path, const char* filePath,
    csString& file)
{
  csString mountable = path;
  if (mountable.StartsWith (appDir, true))
  {
    mountable = mountable.Slice (appDir.Length ());
    if (mountable.StartsWith ("/") || mountable.StartsWith ("\\"))
      mountable = mountable.Slice (1);
    mountable = "$^"+mountable;
  }
  mountable.ReplaceAll ("/", "$/");
  mountable.ReplaceAll ("\\", "$/");
  if (!filePath || !*filePath)
  {
    // 'path' is a directory. In this case make sure we have the correct slashes
    // and quotes that mount likes.
    if (mountable[mountable.Length ()-1] != '/')
      mountable += "$/";
    file.Empty ();
  }
  else
  {
    if (mountable.Slice (mountable.Length ()-4) == ".zip")
      file.Empty ();
    else
    {
      size_t idx = mountable.FindLast ("/");
      if (idx != csArrayItemNotFound)
      {
	file = mountable.Slice (idx+1);
	mountable = mountable.Slice (0, idx+1);
      }
      else
      {
	// @@@ Can this happen?
	printf ("oops?\n"); fflush (stdout);
      }
    }
  }
  return mountable;
}

csString ManageAssetsDialog::ConstructRelativePath (const char* path, const char* filePath)
{
  csString stripped = path;
  for (size_t i = 0 ; i < assetPath.GetSize () ; i++)
  {
    csString a = assetPath.Get (i);
    if (stripped.StartsWith (a, true))
    {
      stripped = "$#"+stripped.Slice (a.Length ());
      break;
    }
  }
  stripped.ReplaceAll ("\\", "/");
  if ((!filePath || !*filePath) && stripped[stripped.Length ()-1] != '/')
  {
    // We have a directory so we add a '/' at the end.
    stripped += '/';
  }
  return stripped;
}

void ManageAssetsDialog::OnPathSelected (wxCommandEvent& event)
{
  csString path = UITools::GetValue (this, "path_Combo");
  wxGenericDirCtrl* dir = XRCCTRL (*this, "browser_Dir", wxGenericDirCtrl);
  dir->ExpandPath (wxString::FromUTF8 (path));
}

void ManageAssetsDialog::OnDirSelChange (wxCommandEvent& event)
{
  wxGenericDirCtrl* dir = XRCCTRL (*this, "browser_Dir", wxGenericDirCtrl);
  csString path = (const char*)(dir->GetPath ().mb_str (wxConvUTF8));
  csString filePath = (const char*)(dir->GetFilePath ().mb_str (wxConvUTF8));

  csString stripped = ConstructRelativePath (path, filePath);
  csString file;
  csString mount = ConstructMountString (path, filePath, file);

  if (!file.IsEmpty ())
    stripped = stripped.Slice (0, stripped.Length () - file.Length ());

  printf ("Stripped: %s\n", (const char*)stripped);
  printf ("Mount: %s (file '%s')\n", (const char*)mount, (const char*)file);
  fflush(stdout);

  wxTextCtrl* normpath_Text = XRCCTRL (*this, "realPath_Text", wxTextCtrl);
  normpath_Text->SetValue (wxString::FromUTF8 (stripped));

  vfs->Unmount ("/tmp/__mnt__", 0);
  vfs->Mount ("/tmp/__mnt__", mount);
  LoadManifest ("/tmp/__mnt__", file, true);
}

void ManageAssetsDialog::LoadManifest (const char* path, const char* file, bool override)
{
  csString descriptionText, mountText, fileText;

  vfs->PushDir (path);
  if (file && *file)
    fileText = file;
  else if (vfs->Exists ("library"))
    fileText = "library";
  else if (vfs->Exists ("world"))
    fileText = "world";
  else if (vfs->Exists ("level.xml"))
    fileText = "level.xml";
  vfs->PopDir ();

  descriptionText = "<html><body>";

  csRef<iDocument> doc;
  csRef<iString> error = uiManager->GetApp ()->GetAssetManager ()->LoadDocument (
      uiManager->GetApp ()->GetObjectRegistry (),
      doc, path, "manifest.xml");
  if (!doc && error)
    descriptionText += error->GetData ();
  else if (doc)
  {
    csRef<iDocumentNode> root = doc->GetRoot ();
    csRef<iDocumentNode> manifestNode = root->GetNode ("manifest");
    if (!manifestNode)
    {
      descriptionText += "Manifest.xml is not valid";
    }
    else
    {
      csRef<iDocumentNode> authorNode = manifestNode->GetNode ("author");
      if (authorNode)
	descriptionText.AppendFmt ("<b>Author(s): </b>%s<br>", authorNode->GetContentsValue ());
      csRef<iDocumentNode> licenseNode = manifestNode->GetNode ("license");
      if (licenseNode)
	descriptionText.AppendFmt ("<b>License: </b>%s<br>", licenseNode->GetContentsValue ());
      csRef<iDocumentNode> descriptionNode = manifestNode->GetNode ("description");
      if (licenseNode)
	descriptionText.AppendFmt ("%s<br>", descriptionNode->GetContentsValue ());
      csRef<iDocumentNode> mountNode = manifestNode->GetNode ("mount");
      if (mountNode)
	mountText = mountNode->GetContentsValue ();
      csRef<iDocumentNode> fileNode = manifestNode->GetNode ("file");
      if (fileNode)
	fileText = fileNode->GetContentsValue ();
    }
  }

  descriptionText += "</body></html>";

  wxHtmlWindow* description_Html = XRCCTRL (*this, "description_Html", wxHtmlWindow);
  description_Html->SetPage (wxString::FromUTF8 (descriptionText));


  if (override)
  {
    wxTextCtrl* mountCtrl = XRCCTRL (*this, "mount_Text", wxTextCtrl);
    mountCtrl->SetValue (wxString::FromUTF8 (mountText));
    wxTextCtrl* fileCtrl = XRCCTRL (*this, "file_Text", wxTextCtrl);
    fileCtrl->SetValue (wxString::FromUTF8 (fileText));
  }

  ScanLoadableFile (path, fileText);
}

void ManageAssetsDialog::OnOkButton (wxCommandEvent& event)
{
  csArray<BaseAsset> assets;

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  for (int i = 0 ; i < assetList->GetItemCount () ; i++)
  {
    csStringArray row = ListCtrlTools::ReadRow (assetList, i);
    csString flags = row[ASSET2_COL_WRITE];
    bool writable = flags == "RW";
    BaseAsset a = BaseAsset (row[ASSET2_COL_FILE], writable);
    a.SetNormalizedPath (row[ASSET2_COL_PATH]);
    a.SetMountPoint (row[ASSET2_COL_MOUNT]);
    assets.Push (a);
  }
  callback->OkPressed (assets);
  callback = 0;

  EndModal (TRUE);
}

void ManageAssetsDialog::OnCancelButton (wxCommandEvent& event)
{
  callback = 0;
  EndModal (TRUE);
}

void ManageAssetsDialog::ScanLoadableFile (const char* path, const char* file)
{
printf ("path=%s file=%s\n", path, file); fflush (stdout);
  csRef<iDocument> doc;
  csRef<iString> error = uiManager->GetApp ()->GetAssetManager ()->LoadDocument (
      uiManager->GetApp ()->GetObjectRegistry (),
      doc, path, file);
  csString msg;
  if (!doc && error)
    msg = error->GetData ();
  else if (doc)
  {
    msg = "Empty XML";
    csRef<iDocumentNode> root = doc->GetRoot ();
    csRef<iDocumentNodeIterator> it = root->GetNodes ();
    while (it->HasNext ())
    {
      csRef<iDocumentNode> child = it->Next ();
      if (child->GetType () != CS_NODE_ELEMENT) continue;
      csString value = child->GetValue ();
      if (value == "dynlevel") msg = "Dynamic level";
      else if (value == "library")
      {
	msg = "Library";
	ScanCSNode (msg, child);
      }
      else if (value == "world")
      {
	msg = "World file";
	ScanCSNode (msg, child);
      }
      else msg = "Unknown XML";
      break;
    }
  }
  else
  {
    msg.Format ("File '%s' can't load...", file);
  }
  wxStaticText* contents = XRCCTRL (*this, "contentsStaticText", wxStaticText);
  contents->SetLabel (wxString::FromUTF8 (msg.GetData ()));
}

void ManageAssetsDialog::ScanCSNode (csString& msg, iDocumentNode* node)
{
  bool hasTexturesMaterials = false;
  bool hasSounds = false;
  bool hasDynFacts = false;
  bool hasQuests = false;
  bool hasLootPackages = false;
  int cntLibraries = 0;
  int cntMeshFacts = 0;
  int cntLightFacts = 0;
  int cntSectors = 0;
  int cntEntityTpl = 0;
  int cntUnkownAddons = 0;

  csRef<iDocumentNodeIterator> it = node->GetNodes ();
  while (it->HasNext ())
  {
    csRef<iDocumentNode> child = it->Next ();
    if (child->GetType () != CS_NODE_ELEMENT) continue;
    csString value = child->GetValue ();
    if (value == "textures" || value == "materials") hasTexturesMaterials = true;
    else if (value == "sounds") hasSounds = true;
    else if (value == "library") cntLibraries++;
    else if (value == "meshfact") cntMeshFacts++;
    else if (value == "lightfact") cntLightFacts++;
    else if (value == "sector") cntSectors++;
    else if (value == "addon")
    {
      csString plugin = child->GetAttributeValue ("plugin");
      if (plugin == "cel.addons.dynamicworld.loader") hasDynFacts = true;
      else if (plugin == "cel.addons.questdef") hasQuests = true;
      else if (plugin == "cel.addons.celentitytpl") cntEntityTpl++;
      else if (plugin == "cel.addons.lootloader") hasLootPackages = true;
      else cntUnkownAddons++;
    }
  }
  if (hasTexturesMaterials) msg += ", textures";
  if (hasSounds) msg += ", sounds";
  if (hasDynFacts) msg += ", dynfacts";
  if (hasQuests) msg += ", quests";
  if (hasLootPackages) msg += ", loot";
  if (cntLibraries) msg.AppendFmt (", %d libraries", cntLibraries);
  if (cntMeshFacts) msg.AppendFmt (", %d factories", cntMeshFacts);
  if (cntLightFacts) msg.AppendFmt (", %d light factories", cntLightFacts);
  if (cntSectors) msg.AppendFmt (", %d sectors", cntSectors);
  if (cntEntityTpl) msg.AppendFmt (", %d templates", cntEntityTpl);
  if (cntUnkownAddons) msg.AppendFmt (", %d unknown", cntUnkownAddons);
}

void ManageAssetsDialog::SetPathFile (const char* file,
    bool writable, const char* normPath, const char* mount)
{
  wxTextCtrl* normpathText = XRCCTRL (*this, "realPath_Text", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "file_Text", wxTextCtrl);
  wxTextCtrl* mountText = XRCCTRL (*this, "mount_Text", wxTextCtrl);
  wxCheckBox* writableCheck = XRCCTRL (*this, "writable_Check", wxCheckBox);
  normpathText->SetValue (wxString::FromUTF8 (normPath));
  fileText->SetValue (wxString::FromUTF8 (file));
  mountText->SetValue (wxString::FromUTF8 (mount));
  writableCheck->SetValue (writable);

  if (file && *file && ((normPath && *normPath) || (mount && *mount)))
  {
    csString path;
    if (!(normPath && *normPath) && mount && *mount)
    {
      path = mount;
      printf ("### LoadManifest mount=%s %s\n", path.GetData (), file);
      LoadManifest (path, file, false);
    }
    else
    {
      csRef<iStringArray> assetLocalPath = vfs->GetRealMountPaths ("/assetslocal/");
      csRef<iStringArray> assetPath = vfs->GetRealMountPaths ("/assets/");
      csRef<iString> p;
      p = uiManager->GetApp ()->GetAssetManager ()->FindAsset (assetLocalPath, normPath, file);
      if (!p)
      {
        p = uiManager->GetApp ()->GetAssetManager ()->FindAsset (assetPath, normPath, file);
        if (!p)
        {
          // @@@ Proper reporting
          printf ("Cannot find asset '%s' in the asset path!\n", normPath);
          return;
        }
      }
      path = p->GetData ();
      printf ("### LoadManifest path=%s %s\n", path.GetData (), file);
      vfs->Unmount ("/tmp/__mnt__", 0);
      vfs->Mount ("/tmp/__mnt__", path);
      LoadManifest ("/tmp/__mnt__", file, false);
    }
  }
}

void ManageAssetsDialog::UpdateAsset (int idx, const char* file,
    bool writable, const char* normPath, const char* mount, bool modified)
{
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  ListCtrlTools::ReplaceRow (assetList, idx, normPath, file, modified ? "*" : " ", writable ? "RW" : "-", mount,
      (const char*)0);
}

void ManageAssetsDialog::AddAsset (const char* file,
    bool writable, const char* normPath, const char* mount, bool modified)
{
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  ListCtrlTools::AddRow (assetList, normPath, file, modified ? "*" : " ", writable ? "RW" : "-", mount,
      (const char*)0);
}

void ManageAssetsDialog::OnAddAssetButton (wxCommandEvent& event)
{
  wxTextCtrl* normPathText = XRCCTRL (*this, "realPath_Text", wxTextCtrl);
  wxTextCtrl* mountText = XRCCTRL (*this, "mount_Text", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "file_Text", wxTextCtrl);
  wxCheckBox* writableCheck = XRCCTRL (*this, "writable_Check", wxCheckBox);
  csString file = (const char*)(fileText->GetValue ().mb_str (wxConvUTF8));
  csString normPath = (const char*)(normPathText->GetValue ().mb_str (wxConvUTF8));
  csString mount = (const char*)(mountText->GetValue ().mb_str (wxConvUTF8));
  bool writable = writableCheck->GetValue ();
  AddAsset (file, writable, normPath, mount, true);
  SetPathFile (file, writable, normPath, mount);
}

void ManageAssetsDialog::OnMoveUpButton (wxCommandEvent& event)
{
  if (selIndex > 0)
  {
    wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
    csStringArray row = ListCtrlTools::ReadRow (assetList, selIndex);
    assetList->DeleteItem (selIndex);
    ListCtrlTools::InsertRow (assetList, selIndex-1, row);
    selIndex--;
    ListCtrlTools::SelectRow (assetList, selIndex);
  }
}

void ManageAssetsDialog::OnMoveDownButton (wxCommandEvent& event)
{
  if (selIndex >= 0)
  {
    wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
    if (selIndex >= assetList->GetItemCount ()-1)
      return;
    csStringArray row = ListCtrlTools::ReadRow (assetList, selIndex);
    assetList->DeleteItem (selIndex);
    ListCtrlTools::InsertRow (assetList, selIndex+1, row);
    selIndex++;
    ListCtrlTools::SelectRow (assetList, selIndex);
  }
}

void ManageAssetsDialog::OnUpdateAssetButton (wxCommandEvent& event)
{
  if (selIndex >= 0)
  {
    wxTextCtrl* normPathText = XRCCTRL (*this, "realPath_Text", wxTextCtrl);
    wxTextCtrl* mountText = XRCCTRL (*this, "mount_Text", wxTextCtrl);
    wxTextCtrl* fileText = XRCCTRL (*this, "file_Text", wxTextCtrl);
    wxCheckBox* writableCheck = XRCCTRL (*this, "writable_Check", wxCheckBox);
    csString file = (const char*)(fileText->GetValue ().mb_str (wxConvUTF8));
    csString normPath = (const char*)(normPathText->GetValue ().mb_str (wxConvUTF8));
    csString mount = (const char*)(mountText->GetValue ().mb_str (wxConvUTF8));
    bool writable = writableCheck->GetValue ();
    UpdateAsset (selIndex,
        file, writable,
        normPath, mount, true);
    SetPathFile (file, writable, normPath, mount);
    wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
    ListCtrlTools::SelectRow (assetList, selIndex);
  }
}

void ManageAssetsDialog::OnDelAssetButton (wxCommandEvent& event)
{
  if (selIndex >= 0)
  {
    wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
    assetList->DeleteItem (selIndex);
    SetPathFile ("", false, "", "");
    selIndex = -1;
  }
}

void ManageAssetsDialog::EnableAssetButtons (bool e)
{
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  wxButton* moveUpButton = XRCCTRL (*this, "moveUpButton", wxButton);
  wxButton* moveDownButton = XRCCTRL (*this, "moveDownButton", wxButton);
  wxButton* updateButton = XRCCTRL (*this, "updateButton", wxButton);
  if (e)
  {
    delButton->Enable ();
    moveUpButton->Enable ();
    moveDownButton->Enable ();
    updateButton->Enable ();
  }
  else
  {
    delButton->Disable ();
    moveUpButton->Disable ();
    moveDownButton->Disable ();
    updateButton->Disable ();
  }
}

void ManageAssetsDialog::OnAssetSelected (wxListEvent& event)
{
  EnableAssetButtons (true);

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  selIndex = event.GetIndex ();
  csStringArray row = ListCtrlTools::ReadRow (assetList, selIndex);
  csString flags = row[ASSET2_COL_WRITE];
  bool writable = flags == "RW";
  SetPathFile (row[ASSET2_COL_FILE], writable, row[ASSET2_COL_PATH], row[ASSET2_COL_MOUNT]);
}

void ManageAssetsDialog::OnAssetDeselected (wxListEvent& event)
{
  EnableAssetButtons (false);
  SetPathFile ("", false, "", "");
}

void ManageAssetsDialog::Setup (ManageAssetsCallback* cb)
{
  this->callback = cb;
  EnableAssetButtons (false);

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  assetList->DeleteAllItems ();
  selIndex = -1;
  currentPath = "/assets";
  vfs->ChDir (currentPath);
}

void ManageAssetsDialog::Show (ManageAssetsCallback* cb)
{
  Setup (cb);
  ShowModal ();
}

void ManageAssetsDialog::Show (ManageAssetsCallback* cb, const csRefArray<iAsset>& assets)
{
  Setup (cb);
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    iAsset* a = assets[i];
    AddAsset (a->GetFile (), a->IsWritable (), a->GetNormalizedPath (), a->GetMountPoint (), a->IsModified ());
  }
  ShowModal ();
}

ManageAssetsDialog::ManageAssetsDialog (wxWindow* parent, iObjectRegistry* object_reg,
    UIManager* uiManager, iVFS* vfs) : object_reg (object_reg), uiManager (uiManager), vfs (vfs)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("ManageAssetsDialog"));

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (assetList, ASSET2_COL_PATH, "Path", 170);
  ListCtrlTools::SetColumn (assetList, ASSET2_COL_FILE, "File", 150);
  ListCtrlTools::SetColumn (assetList, ASSET2_COL_MODIFIED, "M", 20);
  ListCtrlTools::SetColumn (assetList, ASSET2_COL_WRITE, "Write", 50);
  ListCtrlTools::SetColumn (assetList, ASSET2_COL_MOUNT, "Mount", 100);

  wxGenericDirCtrl* dir = XRCCTRL (*this, "browser_Dir", wxGenericDirCtrl);
  dir->Connect (wxEVT_COMMAND_TREE_SEL_CHANGED,
	  wxCommandEventHandler (ManageAssetsDialog :: OnDirSelChange), 0, this);

  wxComboBox* path_Combo = XRCCTRL (*this, "path_Combo", wxComboBox);
  path_Combo->Clear ();

  csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser> (object_reg);
  appDir = cmdline->GetAppDir ();
  wxString defaultdir;
  csRef<iStringArray> localpath = vfs->GetRealMountPaths ("/assetslocal/");
  csRef<iStringArray> path = vfs->GetRealMountPaths ("/assets/");
  for (size_t i = 0 ; i < (path->GetSize () + localpath->GetSize ()) ; i++)
  {
    csString p;
    if (i >= path->GetSize ())
      p = localpath->Get (i-path->GetSize ());
    else
      p = path->Get (i);
    // To work around a problem on linux where sometimes assets have a './' in the path
    // we replace '/./' with '/'.
    p.ReplaceAll ("/./", "/");
    assetPath.Push (p);
    path_Combo->Append (wxString::FromUTF8 (p));

  }
  if (assetPath.GetSize () > 0)
    defaultdir = wxString::FromUTF8 (assetPath.Get (0));
  else
    defaultdir = wxString::FromUTF8 (appDir);
  dir->SetDefaultPath (defaultdir);
  dir->SetPath (defaultdir);

  SetSize (900, 700);
}

ManageAssetsDialog::~ManageAssetsDialog ()
{
  wxGenericDirCtrl* dir = XRCCTRL (*this, "browser_Dir", wxGenericDirCtrl);
  dir->Disconnect (wxEVT_COMMAND_TREE_SEL_CHANGED,
	  wxCommandEventHandler (ManageAssetsDialog :: OnDirSelChange), 0, this);
}


