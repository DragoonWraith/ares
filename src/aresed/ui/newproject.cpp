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
#include "filereq.h"
#include "listctrltools.h"
#include "../apparesed.h"
#include "../../common/worldload.h"

//--------------------------------------------------------------------------

struct SetFilenameCallback : public OKCallback
{
  NewProjectDialog* dialog;
  SetFilenameCallback (NewProjectDialog* dialog) : dialog (dialog) { }
  virtual ~SetFilenameCallback () { }
  virtual void OkPressed (const char* filename)
  {
    dialog->SetFilename (filename);
  }
};

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(NewProjectDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), NewProjectDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), NewProjectDialog :: OnCancelButton)
  EVT_BUTTON (XRCID("searchFileButton"), NewProjectDialog :: OnSearchFileButton)
  EVT_BUTTON (XRCID("addAssetButton"), NewProjectDialog :: OnAddAssetButton)
  EVT_BUTTON (XRCID("delAssetButton"), NewProjectDialog :: OnDelAssetButton)
  EVT_LIST_ITEM_SELECTED (XRCID("assetListCtrl"), NewProjectDialog :: OnAssetSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("assetListCtrl"), NewProjectDialog :: OnAssetDeselected)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void NewProjectDialog::OnOkButton (wxCommandEvent& event)
{
  csArray<Asset> assets;

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  for (int i = 0 ; i < assetList->GetItemCount () ; i++)
  {
    csStringArray row = ListCtrlTools::ReadRow (assetList, i);
    assets.Push (Asset (row[0], row[1]));
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

void NewProjectDialog::OnSearchFileButton (wxCommandEvent& event)
{
  uiManager->GetFileReqDialog ()->Show (new SetFilenameCallback (this));
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
  if (cntSectors) msg.AppendFmt (", %d sectors", cntSectors);
  if (cntEntityTpl) msg.AppendFmt (", %d templates", cntEntityTpl);
  if (cntUnkownAddons) msg.AppendFmt (", %d unknown", cntUnkownAddons);
}

void NewProjectDialog::SetPathFile (const char* path, const char* file)
{
  wxTextCtrl* pathText = XRCCTRL (*this, "pathTextCtrl", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "fileTextCtrl", wxTextCtrl);
  pathText->SetValue (wxString::FromUTF8 (path));
  fileText->SetValue (wxString::FromUTF8 (file));

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
}

void NewProjectDialog::SetFilename (const char* filename)
{
  SetPathFile (vfs->GetCwd (), filename);
}

void NewProjectDialog::OnAddAssetButton (wxCommandEvent& event)
{
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  wxTextCtrl* pathText = XRCCTRL (*this, "pathTextCtrl", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "fileTextCtrl", wxTextCtrl);
  ListCtrlTools::AddRow (assetList,
      (const char*)(pathText->GetValue ().mb_str (wxConvUTF8)),
      (const char*)(fileText->GetValue ().mb_str (wxConvUTF8)),
      0);
  SetPathFile ("", "");
}

void NewProjectDialog::OnDelAssetButton (wxCommandEvent& event)
{
  if (selIndex >= 0)
  {
    wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
    assetList->DeleteItem (selIndex);
    assetList->SetColumnWidth (0, wxLIST_AUTOSIZE | wxLIST_AUTOSIZE_USEHEADER);
    assetList->SetColumnWidth (1, wxLIST_AUTOSIZE | wxLIST_AUTOSIZE_USEHEADER);
    SetPathFile ("", "");
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
  SetPathFile (row[0], row[1]);
}

void NewProjectDialog::OnAssetDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Disable ();
}

void NewProjectDialog::Show (NewProjectCallback* cb)
{
  this->callback = cb;
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Disable ();
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  assetList->DeleteAllItems ();
  selIndex = -1;
  ShowModal ();
}

NewProjectDialog::NewProjectDialog (wxWindow* parent, UIManager* uiManager, iVFS* vfs) :
  uiManager (uiManager), vfs (vfs)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("NewProjectDialog"));

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (assetList, 0, "Path", 200);
  ListCtrlTools::SetColumn (assetList, 1, "File", 200);
}

NewProjectDialog::~NewProjectDialog ()
{
}


