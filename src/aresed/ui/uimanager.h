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

#ifndef __uimanager_h
#define __uimanager_h

class AppAresEditWX;
class FileReq;
class NewProjectDialog;

class UIManager
{
private:
  AppAresEditWX* app;
  wxWindow* parent;

  FileReq* filereqDialog;
  NewProjectDialog* newprojectDialog;

public:
  UIManager (AppAresEditWX* app, wxWindow* parent);
  ~UIManager ();

  void Message (const char* description, ...);
  void Error (const char* description, ...);

  FileReq* GetFileReqDialog () const { return filereqDialog; }
  NewProjectDialog* GetNewProjectDialog () const { return newprojectDialog; }
};

#endif // __uimanager_h

