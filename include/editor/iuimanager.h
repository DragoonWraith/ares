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

#ifndef __iuimanager_h__
#define __iuimanager_h__

#include "csutil/scf.h"
#include <wx/wx.h>

#include "edcommon/model.h"

struct iAresEditor;

/**
 * The UI Manager.
 */
struct iUIManager : public virtual iBase
{
  SCF_INTERFACE(iUIManager,0,0,1);

  virtual void About () = 0;
  virtual void Message (const char* description, ...) = 0;
  virtual bool Error (const char* description, ...) = 0;
  virtual bool Ask (const char* description, ...) = 0;

  virtual iAresEditor* GetApplication () const = 0;

  /**
   * Ask a question to the user and return the answer as a string.
   */
  virtual csRef<iString> AskDialog (const char* description, const char* label,
      const char* value = 0) = 0;

  /**
   * Ask the user to select a value out of a list and return the selected
   * value. This only works with indexed values.
   */
  virtual Ares::Value* AskDialog (const char* description, Ares::Value* collection,
      const char* heading, ...) = 0;

  /**
   * Create a dynamically buildable dialog.
   * The version with 'format' allows for a compact way to describe the dialog contents. The
   * format is as follows:
   *     <format> := <row> { NL <row> }
   *     <row> := <component> { ';' <component> }
   *     <component> := <label> | <text> | <choice> | <multitext> | <typedtext>
   *                    <button> | <wizardbutton>
   *     <label> := 'L' <string>
   *     <button> := 'B' <string>
   *     <wizardbutton> := 'W' <string>
   *     <text> := 'T' <name>
   *     <multitext> := 'M' <name>
   *     <choice> := 'C' <name> ',' <string> { ',' <string> }
   *     <typedtext> := 'E' <type> <name>
   *     <type> := 'T' | 'Q' | 'E'
   *     <name> := <string>
   */
  virtual csPtr<iUIDialog> CreateDialog (const char* title, int width = -1) = 0;
  virtual csPtr<iUIDialog> CreateDialog (const char* title, const char* format, int width = -1) = 0;
  virtual csPtr<iUIDialog> CreateDialog (wxWindow* par, const char* title, int width = -1) = 0;

  /**
   * Allocate a new Id for a context menu.
   */
  virtual int AllocContextMenuID () = 0;
};


#endif // __iuimanager_h__

