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

bool FileReq::StdDlgOkButton (const CEGUI::EventArgs& e)
{
  stddlg->hide();

  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  CEGUI::Window* inputpath = winMgr->getWindow("StdDlg/Path");
  CEGUI::String path = inputpath->getProperty("Text");
  if (path.empty()) return true;

  vfs->ChDir (path.c_str());

  CEGUI::Window* inputfile = winMgr->getWindow("StdDlg/File");
  CEGUI::String file = inputfile->getProperty("Text");
  if (path.empty()) return true;

  callback->OkPressed (file.c_str());

  return true;
}

bool FileReq::StdDlgCancleButton (const CEGUI::EventArgs& e)
{
  stddlg->hide();
  return true;
}

bool FileReq::StdDlgFileSelect (const CEGUI::EventArgs& e)
{
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  CEGUI::Listbox* list = (CEGUI::Listbox*) winMgr->getWindow("StdDlg/FileSelect");
  CEGUI::ListboxItem* item = list->getFirstSelectedItem();
  if (!item) return true;

  CEGUI::String text = item->getText();
  if (text.empty()) return true;

  CEGUI::Window* file = winMgr->getWindow("StdDlg/File");
  file->setProperty("Text", text);
  return true;
}

bool FileReq::StdDlgDirSelect (const CEGUI::EventArgs& e)
{
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  CEGUI::Listbox* list = (CEGUI::Listbox*) winMgr->getWindow("StdDlg/DirSelect");
  CEGUI::ListboxItem* item = list->getFirstSelectedItem();
  if (!item) return true;

  CEGUI::String text = item->getText();
  if (text.empty()) return true;

  csPrintf("cd %s\n",text.c_str());

  CEGUI::Window* inputpath = winMgr->getWindow("StdDlg/Path");
  CEGUI::String path = inputpath->getProperty("Text");
  if (path.empty()) return true;

  csString newpath(path.c_str());

  if (csString("..") == text.c_str())
  {
    size_t i = newpath.Slice(0,newpath.Length()-1).FindLast('/')+1;
    csPrintf("%zu", i);
    newpath = newpath.Slice(0,i);
  }
  else
  {
    newpath.Append(text.c_str());
    newpath.Append("/");
  }

  if (!newpath.GetData()) newpath.Append("/");
  vfs->ChDir (newpath.GetData ());

  inputpath->setProperty("Text", newpath.GetData());
  StdDlgUpdateLists(newpath.GetData());
  return true;
}

bool FileReq::StdDlgDirChange (const CEGUI::EventArgs& e)
{
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  CEGUI::Window* inputpath = winMgr->getWindow("StdDlg/Path");
  CEGUI::String path = inputpath->getProperty("Text");
  if (path.empty()) return true;

  csPrintf("cd %s\n",path.c_str());

  vfs->ChDir (path.c_str ());

  inputpath->setProperty("Text", path.c_str());
  StdDlgUpdateLists(path.c_str());
  return true;
}

void FileReq::StdDlgUpdateLists(const char* filename)
{
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();

  CEGUI::Listbox* dirlist = (CEGUI::Listbox*)winMgr->getWindow("StdDlg/DirSelect");
  CEGUI::Listbox* filelist = (CEGUI::Listbox*)winMgr->getWindow("StdDlg/FileSelect");

  dirlist->resetList();
  filelist->resetList();

  CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem("..");
  item->setTextColours(CEGUI::colour(0,0,0));
  //item->setSelectionBrushImage("ice", "TextSelectionBrush");
  //item->setSelectionColours(CEGUI::colour(0.5f,0.5f,1));
  dirlist->addItem(item);

  csRef<iStringArray> files = vfs->FindFiles(filename);
  
  for (size_t i = 0; i < files->GetSize(); i++)
  {
    char* file = (char*)files->Get(i);
    if (!file) continue;

    size_t dirlen = strlen(file);
    if (dirlen)
      dirlen--;
    while (dirlen && file[dirlen-1]!= '/')
      dirlen--;
    file=file+dirlen;

    if (file[strlen(file)-1] == '/')
    {
      file[strlen(file)-1]='\0';
      CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(file);
      item->setTextColours(CEGUI::colour(0,0,0));
      //item->setSelectionBrushImage("ice", "TextSelectionBrush");
      //item->setSelectionColours(CEGUI::colour(0.5f,0.5f,1));
      dirlist->addItem(item);
    }
    else
    {
      CEGUI::ListboxTextItem* item = new CEGUI::ListboxTextItem(file);
      item->setTextColours(CEGUI::colour(0,0,0));
      //item->setSelectionBrushImage("ice", "TextSelectionBrush");
      //item->setSelectionColours(CEGUI::colour(0.5f,0.5f,1));
      filelist->addItem(item);
    }
  }
}

void FileReq::Show (OKCallback* callback)
{
  this->callback = callback;
  stddlg->show ();
}

FileReq::FileReq (iCEGUI* cegui, iVFS* vfs, const char* path) : cegui (cegui), vfs (vfs)
{
  CEGUI::WindowManager* winMgr = cegui->GetWindowManagerPtr ();
  stddlg = winMgr->getWindow ("StdDlg");

  CEGUI::Window* btn;

  btn = winMgr->getWindow("StdDlg/OkButton");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&FileReq::StdDlgOkButton, this));

  btn = winMgr->getWindow("StdDlg/CancleButton");
  btn->subscribeEvent(CEGUI::PushButton::EventClicked,
    CEGUI::Event::Subscriber(&FileReq::StdDlgCancleButton, this));

  btn = winMgr->getWindow("StdDlg/FileSelect");
  btn->subscribeEvent(CEGUI::Listbox::EventSelectionChanged,
    CEGUI::Event::Subscriber(&FileReq::StdDlgFileSelect, this));

  btn = winMgr->getWindow("StdDlg/DirSelect");
  btn->subscribeEvent(CEGUI::Listbox::EventSelectionChanged,
    CEGUI::Event::Subscriber(&FileReq::StdDlgDirSelect, this));

  btn = winMgr->getWindow("StdDlg/Path");
  btn->subscribeEvent(CEGUI::Editbox::EventTextAccepted,
    CEGUI::Event::Subscriber(&FileReq::StdDlgDirChange, this));

  vfs->ChDir (path);
  btn = winMgr->getWindow("StdDlg/Path");
  btn->setProperty("Text", vfs->GetCwd());
  StdDlgUpdateLists(vfs->GetCwd());
}

FileReq::~FileReq ()
{
}


