/*
The MIT License

Copyright (c) 2013 by Jorrit Tyberghein

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

#ifndef __appares_settingsdialog_h
#define __appares_settingsdialog_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

#include "cseditor/wx/propgrid/propgrid.h"

class UIManager;

using namespace Ares;

class SettingsDialog : public wxDialog
{
private:
  UIManager* uiManager;
  wxPropertyGrid* settingsGrid;

  void OnOkButton (wxCommandEvent& event);
  void OnCancelButton (wxCommandEvent& event);

  void FillGrid ();
  void Save ();

  void CheckBool (iConfigManager* cfgmgr,
    const char* configName, const char* gridName,
    bool& changed, bool* restart = 0);
  void CheckInt (iConfigManager* cfgmgr,
    const char* configName, const char* gridName,
    bool& changed, bool* restart = 0);

public:
  SettingsDialog (wxWindow* parent, UIManager* uiManager);
  ~SettingsDialog ();

  void Show ();

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_settingsdialog_h

