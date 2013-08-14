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

#ifndef __appares_smart_picker_h
#define __appares_smart_picker_h

#include <csutil/stringarray.h>
#include "edcommon/aresextern.h"
#include <wx/wx.h>

class wxWindow;
struct iUIManager;

enum SmartPickerType
{
  SPT_NONE = 0,
  SPT_ENTITY,
  SPT_QUEST,
  SPT_TEMPLATE,
  SPT_MESSAGE
};

/**
 * Tools to generate smart value 'pickers' for various types of objects.
 */
class ARES_EDCOMMON_EXPORT SmartPickerLogic : public wxEvtHandler
{
private:
  iUIManager* uiManager;
  csHash<wxTextCtrl*,csPtrKey<wxButton> > buttonToText;
  csHash<SmartPickerType,csPtrKey<wxButton> > buttonToType;

  void OnSearchButton (wxCommandEvent& event);

public:
  SmartPickerLogic (iUIManager* uiManager) : uiManager (uiManager) { }
  virtual ~SmartPickerLogic () { Cleanup (); }

  /**
   * Clean up the connections.
   */
  void Cleanup ();

  void AddLabel (wxWindow* parent, wxBoxSizer* rowSizer, const char* txt);
  wxTextCtrl* AddText (wxWindow* parent, wxBoxSizer* rowSizer);
  wxButton* AddButton (wxWindow* parent, wxBoxSizer* rowSizer, const char* str,
      bool exact = true);

  /**
   * Add a smart picker.
   */
  wxTextCtrl* AddPicker (SmartPickerType type, wxWindow* parent, wxBoxSizer* rowSizer,
      const char* label = 0);

  /**
   * Setup a smar5 picker on already existing components.
   */
  void SetupPicker (SmartPickerType type, wxTextCtrl* entityText, wxButton* searchButton);

  /**
   * Setup a smart picker on already existing components.
   * Return false on failure (components don't exist or don't have right type).
   */
  bool SetupPicker (SmartPickerType type, wxWindow* parent,
      const char* entityText, const char* searchButton);
};

#endif // __appares_smart_picker_h

