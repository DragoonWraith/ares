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

void SettingsDialog::Save ()
{
  bool restart = false;
  bool changed = false;

  csRef<iConfigManager> cfgmgr = csQueryRegistry<iConfigManager> (
      uiManager->GetApplication ()->GetObjectRegistry ());

  for (size_t i = 0 ; i < settings.GetSize () ; i++)
  {
    Setting& s = settings.Get (i);
    switch (s.type)
    {
      case TYPE_BOOL:
	CheckBool (cfgmgr, s.configName, s.gridName, changed, s.restart ? &restart : 0);
	break;
      case TYPE_LONG:
	CheckInt (cfgmgr, s.configName, s.gridName, changed, s.restart ? &restart : 0);
	break;
      case TYPE_STRING:
      case TYPE_ENUM:
	CheckString (cfgmgr, s.configName, s.gridName, changed, s.restart ? &restart : 0);
	break;
    }
  }

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

  wxPropertyCategory* propVideo = new wxPropertyCategory (wxT ("Video"), wxPG_LABEL);
  settingsGrid->Append (propVideo);
  wxPropertyCategory* propUI = new wxPropertyCategory (wxT ("UI"), wxPG_LABEL);
  settingsGrid->Append (propUI);

  for (size_t i = 0 ; i < settings.GetSize () ; i++)
  {
    Setting& s = settings.Get (i);
    wxPGProperty* prop = 0;
    switch (s.type)
    {
      case TYPE_BOOL:
	prop = new wxBoolProperty (wxString::FromUTF8 (s.gridName), wxPG_LABEL,
	    cfgmgr->GetBool (s.configName));
	break;
      case TYPE_LONG:
	prop = new wxIntProperty (wxString::FromUTF8 (s.gridName), wxPG_LABEL,
	    cfgmgr->GetInt (s.configName));
	break;
      case TYPE_STRING:
	prop = new wxStringProperty (wxString::FromUTF8 (s.gridName), wxPG_LABEL,
	    wxString::FromUTF8 (cfgmgr->GetStr (s.configName)));
	break;
      case TYPE_ENUM:
	prop = new wxEnumProperty (wxString::FromUTF8 (s.gridName), wxPG_LABEL,
	    s.choices, wxArrayInt (), s.choices.Index (wxString::FromUTF8 (
	      cfgmgr->GetStr (s.configName))));
	break;
    }
    if (prop)
    {
      wxPGProperty* catProp = settingsGrid->GetProperty (wxString::FromUTF8 (s.category));
      settingsGrid->AppendIn (catProp, prop);
    }
    else
    {
      csPrintf ("Huh!!!\n");
    }
  }

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

  settings.Push (Setting ("Video", "Video.Maximized", "Window maximized", TYPE_BOOL, true));
  settings.Push (Setting ("Video", "Video.ScreenWidth", "Window width (when not maximized)", TYPE_LONG, true));
  settings.Push (Setting ("Video", "Video.ScreenHeight", "Window height (when not maximized)", TYPE_LONG, true));

  wxArrayString rendermanagers;
  rendermanagers.Add (wxT ("crystalspace.rendermanager.unshadowed"));
  rendermanagers.Add (wxT ("crystalspace.rendermanager.deferred"));
  settings.Push (Setting ("Video", "Engine.RenderManager.Default", "Rendermanager", TYPE_ENUM,
	rendermanagers, true));

  settings.Push (Setting ("UI", "Ares.HelpOverlay", "Help overlay in 3D view", TYPE_BOOL));
  settings.Push (Setting ("UI", "Ares.ToolbarText", "Show text on toolbar", TYPE_BOOL, true));
}

SettingsDialog::~SettingsDialog ()
{
}


