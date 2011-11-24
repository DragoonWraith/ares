/*
The MIT License

Copyright (c) 2010 by Jorrit Tyberghein

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

#include "filereq.h"

BEGIN_EVENT_TABLE(FileReq, wxDialog)
  EVT_BUTTON (XRCID("okButton"), FileReq :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), FileReq :: OnCancelButton)
  EVT_LISTBOX (XRCID("fileListBox"), FileReq :: OnFileViewSelChange)
  EVT_LISTBOX_DCLICK (XRCID("fileListBox"), FileReq :: OnFileViewActivated)
  EVT_LISTBOX (XRCID("dirListBox"), FileReq :: OnDirViewSelChange)
  EVT_LISTBOX_DCLICK (XRCID("dirListBox"), FileReq :: OnDirViewActivated)
END_EVENT_TABLE()

void FileReq::DoOk ()
{
  wxTextCtrl* text = XRCCTRL (*this, "fileNameText", wxTextCtrl);
  csString file = (const char*)text->GetValue ().mb_str (wxConvUTF8);
  EndModal (TRUE);
  callback->OkPressed (file);
  callback = 0;
}

void FileReq::OnOkButton (wxCommandEvent& event)
{
  DoOk ();
}

void FileReq::OnCancelButton (wxCommandEvent& event)
{
  callback = 0;
  EndModal (TRUE);
}

void FileReq::OnFileViewSelChange (wxCommandEvent& event)
{
  wxListBox* filelist = XRCCTRL (*this, "fileListBox", wxListBox);
  csString filename = (const char*)filelist->GetStringSelection ().mb_str(wxConvUTF8);
  wxTextCtrl* text = XRCCTRL (*this, "fileNameText", wxTextCtrl);
  //wxString path (currentPath, wxConvUTF8);
  //path.Append (wxString (filename, wxConvUTF8));
  wxString path (filename, wxConvUTF8);
  text->SetValue (path);
}

void FileReq::OnFileViewActivated (wxCommandEvent& event)
{
  OnFileViewSelChange (event);
  DoOk ();
}

void FileReq::OnDirViewSelChange (wxCommandEvent& event)
{
}

void FileReq::OnDirViewActivated (wxCommandEvent& event)
{
  wxListBox* dirlist = XRCCTRL (*this, "dirListBox", wxListBox);
  csString dir = (const char*)dirlist->GetStringSelection ().mb_str(wxConvUTF8);
  vfs->ChDir (dir);
  currentPath = vfs->GetCwd ();
  StdDlgUpdateLists ();
}

void FileReq::StdDlgUpdateLists ()
{
  wxListBox* dirlist = XRCCTRL (*this, "dirListBox", wxListBox);
  dirlist->Clear ();
  wxListBox* filelist = XRCCTRL (*this, "fileListBox", wxListBox);
  filelist->Clear ();

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
      file[strlen(file)-1] = '\0';
      wxString name = wxString (file, wxConvUTF8);
      dirs.Add (name);
    }
    else
    {
      wxString name = wxString (file, wxConvUTF8);
      files.Add (name);
    }
  }

  dirlist->InsertItems (dirs, 0);
  filelist->InsertItems (files, 0);
}

void FileReq::Show (OKCallback* callback)
{
  FileReq::callback = callback;
  ShowModal ();
}

FileReq::FileReq (wxWindow* parent, iVFS* vfs, const char* path) : vfs (vfs)
{
  currentPath = path;
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("FileRequesterDialog"));

  vfs->ChDir (path);
  //btn = winMgr->getWindow("StdDlg/Path");
  //btn->setProperty("Text", vfs->GetCwd());
  StdDlgUpdateLists ();
}

FileReq::~FileReq ()
{
}


