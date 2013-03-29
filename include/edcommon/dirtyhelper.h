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

#ifndef __appares_dirtyhelper_h
#define __appares_dirtyhelper_h

#include "edcommon/aresextern.h"
#include <wx/wx.h>

class wxWindow;
class wxCheckBox;
class wxTextCtrl;
class wxNotebook;
class wxChoicebook;

struct DirtyListener;

/**
 * A helper keeping track of the dirty state of several
 * components. It remembers the current value of components and registers
 * listeners so that it knows when values change.
 */
class ARES_EDCOMMON_EXPORT DirtyHelper : public wxEvtHandler
{
private:
  csArray<wxCheckBox*> checkBoxes;
  csArray<bool> checkBoxStates;
  csArray<wxTextCtrl*> textControls;
  csStringArray textControlStates;
  csArray<wxNotebook*> notebookControls;
  csStringArray notebookControlStates;
  csArray<wxChoicebook*> choicebookControls;
  csStringArray choicebookControlStates;

  csRefArray<DirtyListener> listeners;

  bool dirty;
  bool CheckDirty ();

  void OnComponentChanged (wxCommandEvent& event);

public:
  DirtyHelper () : dirty (false) { }
  ~DirtyHelper ();

  /**
   * Register a component that needs to be checked for dirty state.
   * Components that are supported are choicebooks, notebooks, checkboxes
   * and text controls.
   * Returns false on error (unknown component or unsupported type).
   */
  bool RegisterComponent (wxWindow* parent, const char* name);

  /**
   * Register a series of components to check for dirty state.
   * End the parameter list with (const char*)0.
   */
  bool RegisterComponents (wxWindow* parent, ...);

  /**
   * Add dirty listener.
   */
  void AddDirtyListener (DirtyListener* listener);

  /**
   * Remove dirty listener.
   */
  void RemoveDirtyListener (DirtyListener* listener);

  /**
   * Set/clear the dirty flag.
   */
  void SetDirty (bool dirty);

  /**
   * Get the current dirty state.
   */
  bool IsDirty () const { return dirty; }
};

#endif // __appares_dirtyhelper_h

