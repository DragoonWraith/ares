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

#ifndef __appares_pcdialog_h
#define __appares_pcdialog_h

#include <crystalspace.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/choicebk.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

struct iCelEntityTemplate;
struct iCelPropertyClassTemplate;
struct iCelParameterIterator;
struct iParameter;
class UIDialog;
class UIManager;
class EntityMode;

typedef csHash<csRef<iParameter>,csStringID> ParHash;

enum
{
  ID_Prop_Add = wxID_HIGHEST + 10000,
  ID_Prop_Edit,
  ID_Prop_Delete,
  ID_Inv_Add,
  ID_Inv_Edit,
  ID_Inv_Delete,
  ID_Spawn_Add,
  ID_Spawn_Edit,
  ID_Spawn_Delete,
  ID_Quest_Add,
  ID_Quest_Edit,
  ID_Quest_Delete,
  ID_WirePar_Add,
  ID_WirePar_Edit,
  ID_WirePar_Delete,
  ID_WireMsg_Add,
  ID_WireMsg_Edit,
  ID_WireMsg_Delete,
};


class PropertyClassPanel : public wxPanel
{
private:
  UIManager* uiManager;
  EntityMode* emode;
  wxSizer* parentSizer;
  iCelEntityTemplate* tpl;
  iCelPropertyClassTemplate* pctpl;

  void OnApplyButton (wxCommandEvent& event);
  void OnRevertButton (wxCommandEvent& event);

  bool CheckHitList (const char* listname, bool& hasItems, const wxPoint& pos);
  void OnContextMenu (wxContextMenuEvent& event);

  // Properties
  UIDialog* propDialog;
  UIDialog* GetPropertyDialog ();
  bool UpdateProperties ();
  void FillProperties ();
  void OnPropertyRMB (bool hasItems);
  void OnPropertyAdd (wxCommandEvent& event);
  void OnPropertyDel (wxCommandEvent& event);
  void OnPropertyEdit (wxCommandEvent& event);

  // Inventory.
  UIDialog* invTempDialog;
  UIDialog* GetInventoryTemplateDialog ();
  bool UpdateInventory ();
  void FillInventory ();
  void OnInventoryTemplateRMB (bool hasItems);
  void OnInventoryTemplateAdd (wxCommandEvent& event);
  void OnInventoryTemplateDel (wxCommandEvent& event);
  void OnInventoryTemplateEdit (wxCommandEvent& event);

  // Quest.
  UIDialog* questParDialog;
  UIDialog* GetQuestDialog ();
  bool UpdateQuest ();
  void FillQuest ();
  void OnQuestParameterRMB (bool hasItems);
  void OnQuestParameterAdd (wxCommandEvent& event);
  void OnQuestParameterDel (wxCommandEvent& event);
  void OnQuestParameterEdit (wxCommandEvent& event);

  // Spawn.
  UIDialog* spawnTempDialog;
  UIDialog* GetSpawnTemplateDialog ();
  bool UpdateSpawn ();
  void FillSpawn ();
  void OnSpawnTemplateRMB (bool hasItems);
  void OnSpawnTemplateAdd (wxCommandEvent& event);
  void OnSpawnTemplateDel (wxCommandEvent& event);
  void OnSpawnTemplateEdit (wxCommandEvent& event);

  // Wire.
  UIDialog* wireParDialog;
  UIDialog* GetWireParDialog ();
  UIDialog* wireMsgDialog;
  UIDialog* GetWireMsgDialog ();
  csHash<ParHash,csString> wireParams;
  bool UpdateWire ();
  void FillWire ();
  bool UpdateCurrentWireParams ();
  void OnWireMessageRMB (bool hasItems);
  void OnWireMessageAdd (wxCommandEvent& event);
  void OnWireMessageEdit (wxCommandEvent& event);
  void OnWireMessageDel (wxCommandEvent& event);
  void OnWireMessageSelected (wxListEvent& event);
  void OnWireMessageDeselected (wxListEvent& event);
  void OnWireParameterRMB (bool hasItems);
  void OnWireParameterAdd (wxCommandEvent& event);
  void OnWireParameterEdit (wxCommandEvent& event);
  void OnWireParameterDel (wxCommandEvent& event);

  // Update the property class. Returns false on error.
  bool UpdatePC ();

public:
  PropertyClassPanel (wxWindow* parent, UIManager* uiManager, EntityMode* emode);
  ~PropertyClassPanel();

  // Switch this dialog to editing of a PC. If the PC is null then we
  // are going to create a new pctpl.
  void SwitchToPC (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl);

  void Show () { wxPanel::Show (); parentSizer->Layout (); }
  void Hide () { wxPanel::Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return IsShown (); }

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_pcdialog_h

