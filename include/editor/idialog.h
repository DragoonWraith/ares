/*
The MIT License

Copyright (c) 2012 by Jorrit Tyberghein

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

#ifndef __idialog_h__
#define __idialog_h__

#include "csutil/scf.h"
#include <wx/wx.h>

struct iDynamicObject;
struct i3DView;
struct iAresEditor;

/**
 * A dialog plugin for the editor.
 */
struct iDialogPlugin : public virtual iBase
{
  SCF_INTERFACE(iDialogPlugin,0,0,1);

  /**
   * Set the application.
   */
  virtual void SetApplication (iAresEditor* app) = 0;

  /**
   * Set the wxWindow parent for the panel of this mode.
   */
  virtual void SetParent (wxWindow* parent) = 0;

  /**
   * The name of this panel.
   */
  virtual const char* GetDialogName () const = 0;

  /**
   * Open the dialog.
   */
  virtual void ShowDialog () = 0;
};


#endif // __idialog_h__

