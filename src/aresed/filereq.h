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

#ifndef __appares_filereq_h
#define __appares_filereq_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

struct OKCallback : public csRefCount
{
  virtual void OkPressed (const char* filename) = 0;
};

class FileReq : public wxDialog
{
private:
  csRef<iVFS> vfs;
  csRef<OKCallback> callback;

  void StdDlgUpdateLists (const char* filename);
  void OnFileViewSelChange (wxListEvent& event);
  void OnFileViewActivated (wxListEvent& event);
  void OnDirViewSelChange (wxListEvent& event);
  void OnDirViewActivated (wxListEvent& event);
  void OnOkButton (wxCommandEvent& event);
  void OnCancelButton (wxCommandEvent& event);

  csString currentPath;

  void DoOk ();

public:
  FileReq (wxWindow* parent, iVFS* vfs, const char* path);
  ~FileReq();

  void Show (OKCallback* callback);

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_filereq_h

