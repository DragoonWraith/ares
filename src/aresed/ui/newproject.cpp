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
#include "filereq.h"

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
  EndModal (TRUE);
}

void NewProjectDialog::OnCancelButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void NewProjectDialog::OnSearchFileButton (wxCommandEvent& event)
{
  filereq->Show (new SetFilenameCallback (this));
}

void NewProjectDialog::SetFilename (const char* filename)
{
  wxTextCtrl* pathText = XRCCTRL (*this, "pathTextCtrl", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "fileTextCtrl", wxTextCtrl);
  pathText->SetValue (wxString (vfs->GetCwd (), wxConvUTF8));
  fileText->SetValue (wxString (filename, wxConvUTF8));
}

void NewProjectDialog::OnAddAssetButton (wxCommandEvent& event)
{
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  wxTextCtrl* pathText = XRCCTRL (*this, "pathTextCtrl", wxTextCtrl);
  wxTextCtrl* fileText = XRCCTRL (*this, "fileTextCtrl", wxTextCtrl);

  long idx = assetList->InsertItem (assetList->GetItemCount (), pathText->GetValue ());
  assetList->SetItem (idx, 1, fileText->GetValue ());
  assetList->SetColumnWidth (0, wxLIST_AUTOSIZE | wxLIST_AUTOSIZE_USEHEADER);
  assetList->SetColumnWidth (1, wxLIST_AUTOSIZE | wxLIST_AUTOSIZE_USEHEADER);
}

void NewProjectDialog::OnDelAssetButton (wxCommandEvent& event)
{
  if (selIndex >= 0)
  {
    wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
    assetList->DeleteItem (selIndex);
    assetList->SetColumnWidth (0, wxLIST_AUTOSIZE | wxLIST_AUTOSIZE_USEHEADER);
    assetList->SetColumnWidth (1, wxLIST_AUTOSIZE | wxLIST_AUTOSIZE_USEHEADER);
  }
}

void NewProjectDialog::OnAssetSelected (wxListEvent& event)
{
  selIndex = event.GetIndex ();
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Enable ();
}

void NewProjectDialog::OnAssetDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Disable ();
}

void NewProjectDialog::Show ()
{
  wxButton* delButton = XRCCTRL (*this, "delAssetButton", wxButton);
  delButton->Disable ();
  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);
  assetList->DeleteAllItems ();
  selIndex = -1;
  ShowModal ();
}

NewProjectDialog::NewProjectDialog (wxWindow* parent, FileReq* filereq, iVFS* vfs) :
  filereq (filereq), vfs (vfs)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("NewProjectDialog"));

  wxListCtrl* assetList = XRCCTRL (*this, "assetListCtrl", wxListCtrl);

  wxListItem colPath;
  colPath.SetId (0);
  colPath.SetText (wxT ("Path"));
  colPath.SetWidth (200);
  assetList->InsertColumn (0, colPath);

  wxListItem colFile;
  colFile.SetId (1);
  colFile.SetText (wxT ("File"));
  colFile.SetWidth (200);
  assetList->InsertColumn (1, colFile);
}

NewProjectDialog::~NewProjectDialog ()
{
}


