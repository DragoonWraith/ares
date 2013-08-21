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

#include "../apparesed.h"
#include "../aresview.h"
#include "settingsdialog.h"
#include "uimanager.h"
#include "edcommon/uitools.h"

#include "celtool/stdparams.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), SettingsDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), SettingsDialog :: OnCancelButton)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void SettingsDialog::OnOkButton (wxCommandEvent& event)
{
  Save ();
  EndModal (TRUE);
}

void SettingsDialog::OnCancelButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void SettingsDialog::CheckBool (iConfigManager* cfgmgr,
    const char* configName, const char* gridName,
    bool& changed, bool* restart)
{
  bool oldValue = cfgmgr->GetBool (configName);
  bool newValue = settingsGrid->GetPropertyValueAsBool (wxString::FromUTF8 (gridName));
  if (oldValue != newValue)
  {
    cfgmgr->SetBool (configName, newValue);
    changed = true;
    if (restart)
      *restart = true;
  }
}

void SettingsDialog::CheckInt (iConfigManager* cfgmgr,
    const char* configName, const char* gridName,
    bool& changed, bool* restart)
{
  int oldValue = cfgmgr->GetInt (configName);
  int newValue = settingsGrid->GetPropertyValueAsInt (wxString::FromUTF8 (gridName));
  if (oldValue != newValue)
  {
    cfgmgr->SetInt (configName, newValue);
    changed = true;
    if (restart)
      *restart = true;
  }
}

void SettingsDialog::CheckString (iConfigManager* cfgmgr,
    const char* configName, const char* gridName,
    bool& changed, bool* restart)
{
  csString oldValue = cfgmgr->GetStr (configName);
  wxString newValueWX = settingsGrid->GetPropertyValueAsString (wxString::FromUTF8 (gridName));
  csString newValue = (const char*)newValueWX.mb_str (wxConvUTF8);
  if (oldValue != newValue)
  {
    cfgmgr->SetStr (configName, newValue);
    changed = true;
    if (restart)
      *restart = true;
  }
}

#define SETTING_MAXIMIZED "Window maximized"
#define SETTING_WINDOWWIDTH "Window width (when not maximized)"
#define SETTING_WINDOWHEIGHT "Window height (when not maximized)"
#define SETTING_HELPOVERLAY "Help overlay in 3D view"
#define SETTING_TOOLBARTEXT "Show text on toolbar"
#define SETTING_RENDERMANAGER "Rendermanager"

void SettingsDialog::Save ()
{
  bool restart = false;
  bool changed = false;

  csRef<iConfigManager> cfgmgr = csQueryRegistry<iConfigManager> (
      uiManager->GetApplication ()->GetObjectRegistry ());

  CheckBool (cfgmgr, "Video.Maximized", SETTING_MAXIMIZED, changed, &restart);
  CheckInt (cfgmgr, "Video.ScreenWidth", SETTING_WINDOWWIDTH, changed, &restart);
  CheckInt (cfgmgr, "Video.ScreenHeight", SETTING_WINDOWHEIGHT, changed, &restart);
  CheckBool (cfgmgr, "Ares.HelpOverlay", SETTING_HELPOVERLAY, changed);
  CheckBool (cfgmgr, "Ares.ToolbarText", SETTING_TOOLBARTEXT, changed, &restart);
  CheckString (cfgmgr, "Engine.RenderManager.Default", SETTING_RENDERMANAGER, changed, &restart);

  if (changed)
  {
    cfgmgr->Save ();
    if (restart)
      uiManager->Message ("A restart is needed to apply some of the changed values");
    uiManager->GetApplication ()->ReadConfig ();
  }
}

void SettingsDialog::FillGrid ()
{
  settingsGrid->Freeze ();
  settingsGrid->Clear ();

  csRef<iConfigManager> cfgmgr = csQueryRegistry<iConfigManager> (
      uiManager->GetApplication ()->GetObjectRegistry ());

  bool maximized = cfgmgr->GetBool ("Video.Maximized");
  settingsGrid->Append (new wxBoolProperty (wxT (SETTING_MAXIMIZED), wxPG_LABEL, maximized));
  int screenwidth = cfgmgr->GetInt ("Video.ScreenWidth");
  settingsGrid->Append (new wxIntProperty (wxT (SETTING_WINDOWWIDTH), wxPG_LABEL, screenwidth));
  int screenheight = cfgmgr->GetInt ("Video.ScreenHeight");
  settingsGrid->Append (new wxIntProperty (wxT (SETTING_WINDOWHEIGHT), wxPG_LABEL, screenheight));
  bool helpOverlay = cfgmgr->GetBool ("Ares.HelpOverlay", true);
  settingsGrid->Append (new wxBoolProperty (wxT (SETTING_HELPOVERLAY), wxPG_LABEL, helpOverlay));
  bool toolbarText = cfgmgr->GetBool ("Ares.ToolbarText", true);
  settingsGrid->Append (new wxBoolProperty (wxT (SETTING_TOOLBARTEXT), wxPG_LABEL, toolbarText));
  csString rendermanager = cfgmgr->GetStr ("Engine.RenderManager.Default");
  wxArrayString rendermanagers;
  rendermanagers.Add (wxT ("crystalspace.rendermanager.unshadowed"));
  rendermanagers.Add (wxT ("crystalspace.rendermanager.deferred"));
  settingsGrid->Append (new wxEnumProperty (wxT (SETTING_RENDERMANAGER), wxPG_LABEL,
	rendermanagers, wxArrayInt (), rendermanagers.Index (
	  wxString::FromUTF8 (rendermanager))));

  settingsGrid->FitColumns ();
  settingsGrid->Thaw ();

  wxPanel* mainPanel = XRCCTRL (*this, "mainPanel", wxPanel);
  mainPanel->GetSizer ()->Layout ();

  settingsGrid->FitColumns ();
}

void SettingsDialog::Show ()
{
  FillGrid ();
  ShowModal ();
}

SettingsDialog::SettingsDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("SettingsDialog"));

  wxPanel* mainPanel = XRCCTRL (*this, "mainPanel", wxPanel);
  settingsGrid = new wxPropertyGrid (mainPanel);
  mainPanel->GetSizer ()->Add (settingsGrid, 1, wxALL | wxEXPAND);
}

SettingsDialog::~SettingsDialog ()
{
}


