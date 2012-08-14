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

#include "newproject.h"
#include "uimanager.h"
#include "edcommon/listctrltools.h"
#include "../apparesed.h"
#include "common/worldload.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(NewProjectDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), NewProjectDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), NewProjectDialog :: OnCancelButton)
  EVT_BUTTON (XRCID("addAssetButton"), NewProjectDialog :: OnAddAssetButton)
  EVT_BUTTON (XRCID("delAssetButton"), NewProjectDialog :: OnDelAssetButton)
  EVT_LIST_ITEM_SELECTED (XRCID("assetListCtrl"), NewProjectDialog :: OnAssetSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("assetListCtrl"), NewProjectDialog :: OnAssetDeselected)
  EVT_LISTBOX (XRCID("browser_List"), NewProjectDialog :: OnBrowserSelChange)
  EVT_LISTBOX_DCLICK (XRCID("browser_List"), NewProjectDialog :: OnAddAssetButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void NewProjectDialog::OnBrowserSelChange (wxCommandEvent& event)
{
  wxListBox* list = XRCCTRL (*this, "browser_List", wxListBox);
  csString filename = (const char*)list->GetStringSelection ().mb_str(wxConvUTF8);

  if (filename[filename.Length ()-1] == '/' || filename == "..")
  {
    vfs->ChDir (filename);
    currentPath = vfs->GetCwd ();
    FillBrowser ();
    LoadManifest (currentPath, "");
    return;
  }

  SetFilename (filename);
}

csString NewProjectDialog::ConstructMountString (const char* path, const char* filePath,
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
	mountable = mountable.Slice (0, idx);
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

csString NewProjectDialog::ConstructRelativePath (const char* path, const char* filePath)
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

void NewProjectDialog::OnDirSelChange (wxCommandEvent& event)
{
  wxGenericDirCtrl* dir = XRCCTRL (*this, "browser_Dir", wxGenericDirCtrl);
  csString path = (const char*)(dir->GetPath ().mb_str (wxConvUTF8));
  csString filePath = (const char*)(dir->GetFilePath ().mb_str (wxConvUTF8));

  csString stripped = ConstructRelativePath (path, filePath);
  printf ("Stripped: %s\n", (const char*)stripped);

  csString file;
  csString mount = ConstructMountString (path, filePath, file);
  printf ("Mount: %s (file '%s')\n", (const char*)mount, (const char*)file);

  fflush(stdout);

  wxTextCtrl* realpath_Text = XRCCTRL (*this, "realPath_Text", wxTextCtrl);
  realpath_Text->SetValue (wxString::FromUTF8 (stripped));

  vfs->Unmount ("/tmp/_aresed_mgr_", 0);
  vfs->Mount ("/tmp/_aresed_mgr_", mount);
  LoadManifest ("/tmp/_aresed_mgr_", file);
}

void NewProjectDialog::LoadManifest (const char* path, const char* file)
{
  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (uiManager->GetApp ()->GetObjectRegistry ());
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  csRef<iDocument> doc = docsys->CreateDocument ();
  vfs->PushDir (path);
  csRef<iDataBuffer> buf = vfs->ReadFile ("manifest.xml");
  csString descriptionText, mountText;
  if (buf)
  {
    const char* error = doc->Parse (buf->GetData ());
    if (error)
    {
      descriptionText.Format ("Can't parse manifest.xml: %s", error);
    }
    else
    {
      csRef<iDocumentNode> root = doc->GetRoot ();
      csRef<iDocumentNode> manifestNode = root->GetNode ("manifest");
      if (!manifestNode)
      {
        descriptionText.Format ("Manifest.xml is not valid");
      }
      else
      {
	csRef<iDocumentNode> authorNode = manifestNode->GetNode ("author");
	if (authorNode)
	  descriptionText.AppendFmt ("Author: %s\n", authorNode->GetContentsValue ());
	csRef<iDocumentNode> licenseNode = manifestNode->GetNode ("license");
	if (licenseNode)
	  descriptionText.AppendFmt ("License: %s\n", licenseNode->GetContentsValue ());
	csRef<iDocumentNode> descriptionNode = manifestNode->GetNode ("description");
	if (licenseNode)
	  descriptionText.AppendFmt ("%s\n", descriptionNode->GetContentsValue ());
	csRef<iDocumentNode> mountNode = manifestNode->GetNode ("mount");
	if (mountNode)
	  mountText = mountNode->GetContentsValue ();
      }
    }
  }
  wxTextCtrl* descriptionCtrl = XRCCTRL (*this, "description_Text", wxTextCtrl);
  descriptionCtrl->SetValue (wxString::FromUTF8 (descriptionText));
  wxTextCtrl* mountCtrl = XRCCTRL (*this, "mount_Text", wxTextCtrl);
  mountCtrl->SetValue (wxString::FromUTF8 (mountText));

  vfs->PopDir ();
}

void NewProjectDialog::FillBrowser ()
{
  wxListBox* list = XRCCTRL (*this, "browser_List", wxListBox);
  list->Clear ();

  wxArrayString dirs, files;
  dirs.Add (wxT (".."));

  csRef<iStringArray> vfsFiles = vfs->FindFiles (vfs->GetCwd ());
  
  for (size_t i = 0; i < vfsFiles->GetSize(); i++)
  {
    char* file = (char*)vfsFiles->Get(i);
    if (!file) continue;

    size_t dirlen = strlen (file);
    if (dirlen)
      dirlen--;
    while (dirlen && file[dirlen-1]!= '/')
      dirlen--;
    file = file+dirlen;

    if (file[strlen(file)-1] == '/')
    {
      wxString name = wxString::FromUTF8 (file);
      dirs.Add (name);
    }
    else
    {
      wxString name = wxString::FromUTF8 (file);
      files.Add (name);
    }
  }

  list->InsertItems (files, 0);
  list->InsertItems (dirs, 0);
}

void NewProjectDialog::OnOkButton (wxCommandEvent& event)
{
  csArray<Asset> assets;

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  for (int i = 0 ; i < assetList->GetItemCount () ; i++)
  {
    csStringArray row = ListCtrlTools::ReadRow (assetList, i);
    bool saveDynfacts, saveTemplates, saveQuests, saveLights;
    csScanStr (row[2], "%b", &saveDynfacts);
    csScanStr (row[3], "%b", &saveTemplates);
    csScanStr (row[4], "%b", &saveQuests);
    csScanStr (row[5], "%b", &saveLights);
    Asset a = Asset (row[0], row[1], saveDynfacts, saveTemplates,
	  saveQuests, saveLights);
    a.SetRealPath (row[6]);
    a.SetMountPoint (row[7]);
    assets.Push (a);
  }
  callback->OkPressed (assets);
  callback = 0;

  EndModal (TRUE);
}

void NewProjectDialog::OnCancelButton (wxCommandEvent& event)
{
  callback = 0;
  EndModal (TRUE);
}

void NewProjectDialog::ScanCSNode (csString& msg, iDocumentNode* node)
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

void NewProjectDialog::SetPathFile (const char* path, const char* file,
    bool saveDynfacts, bool saveTemplates, bool saveQuests, bool saveLights)
{
  wxTextCtrl* pathText = XRCCTRL (*this, "pathTextCtrl", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "fileTextCtrl", wxTextCtrl);
  wxCheckBox* dynfactsCheck = XRCCTRL (*this, "dynfact_Check", wxCheckBox);
  wxCheckBox* templatesCheck = XRCCTRL (*this, "entity_Check", wxCheckBox);
  wxCheckBox* questsCheck = XRCCTRL (*this, "quest_Check", wxCheckBox);
  wxCheckBox* lightsCheck = XRCCTRL (*this, "light_Check", wxCheckBox);
  pathText->SetValue (wxString::FromUTF8 (path));
  fileText->SetValue (wxString::FromUTF8 (file));
  dynfactsCheck->SetValue (saveDynfacts);
  templatesCheck->SetValue (saveTemplates);
  questsCheck->SetValue (saveQuests);
  lightsCheck->SetValue (saveLights);

  wxStaticText* contents = XRCCTRL (*this, "contentsStaticText", wxStaticText);

  csRef<iDocumentSystem> docsys;
  docsys = csQueryRegistry<iDocumentSystem> (uiManager->GetApp ()->GetObjectRegistry ());
  if (!docsys)
    docsys.AttachNew (new csTinyDocumentSystem ());

  csRef<iDocument> doc = docsys->CreateDocument ();
  vfs->PushDir (path);
  csRef<iDataBuffer> buf = vfs->ReadFile (file);
  csString msg;
  if (buf)
  {
    const char* error = doc->Parse (buf->GetData ());
    if (error)
    {
      msg.Format ("Can't parse XML: %s", error);
    }
    else
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
  }
  else
  {
    msg = "File can't load...";
  }
  contents->SetLabel (wxString::FromUTF8 (msg.GetData ()));
  vfs->PopDir ();

  LoadManifest (path, file);
}

void NewProjectDialog::SetFilename (const char* filename)
{
  SetPathFile (vfs->GetCwd (), filename, false, false, false, false);
}

void NewProjectDialog::AddAsset (const char* path, const char* file,
    bool dynfacts, bool templates, bool quests, bool lights,
    const char* realPath, const char* mount)
{
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  ListCtrlTools::AddRow (assetList, path, file,
      dynfacts ? "true" : "",
      templates ? "true" : "",
      quests ? "true" : "",
      lights ? "true" : "",
      realPath, mount,
      (const char*)0);
}

void NewProjectDialog::OnAddAssetButton (wxCommandEvent& event)
{
  wxTextCtrl* pathText = XRCCTRL (*this, "pathTextCtrl", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "fileTextCtrl", wxTextCtrl);
  wxTextCtrl* realPathText = XRCCTRL (*this, "realPath_Text", wxTextCtrl);
  wxTextCtrl* mountText = XRCCTRL (*this, "mount_Text", wxTextCtrl);
  wxCheckBox* dynfactsCheck = XRCCTRL (*this, "dynfact_Check", wxCheckBox);
  wxCheckBox* templatesCheck = XRCCTRL (*this, "entity_Check", wxCheckBox);
  wxCheckBox* questsCheck = XRCCTRL (*this, "quest_Check", wxCheckBox);
  wxCheckBox* lightsCheck = XRCCTRL (*this, "light_Check", wxCheckBox);
  AddAsset (
      (const char*)(pathText->GetValue ().mb_str (wxConvUTF8)),
      (const char*)(fileText->GetValue ().mb_str (wxConvUTF8)),
      dynfactsCheck->GetValue (),
      templatesCheck->GetValue (),
      questsCheck->GetValue (),
      lightsCheck->GetValue (),
      (const char*)(realPathText->GetValue ().mb_str (wxConvUTF8)),
      (const char*)(mountText->GetValue ().mb_str (wxConvUTF8)));
  SetPathFile ("", "", false, false, false, false);
}

void NewProjectDialog::OnDelAssetButton (wxCommandEvent& event)
{
  if (selIndex >= 0)
  {
    wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
    assetList->DeleteItem (selIndex);
    SetPathFile ("", "", false, false, false, false);
    selIndex = -1;
  }
}

void NewProjectDialog::OnAssetSelected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Enable ();
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  selIndex = event.GetIndex ();
  csStringArray row = ListCtrlTools::ReadRow (assetList, selIndex);
  bool saveDynfacts, saveTemplates, saveQuests, saveLights;
  csScanStr (row[2], "%b", &saveDynfacts);
  csScanStr (row[3], "%b", &saveTemplates);
  csScanStr (row[4], "%b", &saveQuests);
  csScanStr (row[5], "%b", &saveLights);
  SetPathFile (row[0], row[1], saveDynfacts, saveTemplates, saveQuests,
      saveLights);
}

void NewProjectDialog::OnAssetDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Disable ();
}

void NewProjectDialog::Setup (NewProjectCallback* cb)
{
  this->callback = cb;
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Disable ();
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  assetList->DeleteAllItems ();
  selIndex = -1;
  currentPath = "/assets";
  vfs->ChDir (currentPath);
  FillBrowser ();
}

void NewProjectDialog::Show (NewProjectCallback* cb)
{
  Setup (cb);
  ShowModal ();
}

void NewProjectDialog::Show (NewProjectCallback* cb, const csArray<Asset>& assets)
{
  Setup (cb);
  for (size_t i = 0 ; i < assets.GetSize () ; i++)
  {
    const Asset& a = assets[i];
    AddAsset (a.GetPath (), a.GetFile (), a.IsDynfactSavefile (),
	a.IsTemplateSavefile (), a.IsQuestSavefile (), a.IsLightFactSaveFile (),
	a.GetRealPath (), a.GetMountPoint ());
  }
  ShowModal ();
}

NewProjectDialog::NewProjectDialog (wxWindow* parent, iObjectRegistry* object_reg,
    UIManager* uiManager, iVFS* vfs) : object_reg (object_reg), uiManager (uiManager), vfs (vfs)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("NewProjectDialog"));

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (assetList, 0, "Path", 220);
  ListCtrlTools::SetColumn (assetList, 1, "File", 100);
  ListCtrlTools::SetColumn (assetList, 2, "Dynf", 50);
  ListCtrlTools::SetColumn (assetList, 3, "Templ", 50);
  ListCtrlTools::SetColumn (assetList, 4, "Quest", 50);
  ListCtrlTools::SetColumn (assetList, 5, "Light", 50);
  ListCtrlTools::SetColumn (assetList, 6, "RealPath", 100);
  ListCtrlTools::SetColumn (assetList, 7, "Mount", 100);

  wxGenericDirCtrl* dir = XRCCTRL (*this, "browser_Dir", wxGenericDirCtrl);
  dir->Connect (wxEVT_COMMAND_TREE_SEL_CHANGED,
	  wxCommandEventHandler (NewProjectDialog :: OnDirSelChange), 0, this);

  csRef<iCommandLineParser> cmdline = csQueryRegistry<iCommandLineParser> (object_reg);
  appDir = cmdline->GetAppDir ();
  wxString defaultdir;
  csRef<iStringArray> path = vfs->GetRealMountPaths ("/assets/");
  for (size_t i = 0 ; i < path->GetSize () ; i++)
  {
    csString p = path->Get (i);
    // To work around a problem on linux where sometimes assets have a './' in the path
    // we replace '/./' with '/'.
    p.ReplaceAll ("/./", "/");
    assetPath.Push (p);
  }
  if (assetPath.GetSize () > 0)
    defaultdir = wxString::FromUTF8 (assetPath.Get (0));
  else
    defaultdir = wxString::FromUTF8 (appDir);
  dir->SetDefaultPath (defaultdir);
  dir->SetPath (defaultdir);
}

NewProjectDialog::~NewProjectDialog ()
{
  wxGenericDirCtrl* dir = XRCCTRL (*this, "browser_Dir", wxGenericDirCtrl);
  dir->Disconnect (wxEVT_COMMAND_TREE_SEL_CHANGED,
	  wxCommandEventHandler (NewProjectDialog :: OnDirSelChange), 0, this);
}


