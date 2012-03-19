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

#ifndef __iapp_h__
#define __iapp_h__

#include "csutil/scf.h"
#include <wx/wx.h>

struct iUIManager;

/**
 * The editor application.
 */
struct iAresEditor : public virtual iBase
{
  SCF_INTERFACE(iAresEditor,0,0,1);

  virtual iUIManager* GetUI () const = 0;

  /// Set the state of the menus correctly depending on context.
  virtual void SetMenuState () = 0;

  /// Set the help status message at the bottom of the frame.
  virtual void SetStatus (const char* statusmsg, ...) = 0;
  /// Clear the help status message (go back to default).
  virtual void ClearStatus () = 0;

};


#endif // __iapp_h__

