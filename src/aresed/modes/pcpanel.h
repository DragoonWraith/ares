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

struct iCelPlLayer;
struct iCelEntityTemplate;
struct iCelPropertyClassTemplate;
struct iCelParameterIterator;
struct iParameter;
class UIDialog;
class UIManager;
class EntityMode;

class PropertyRowModel;
class InventoryRowModel;
class QuestRowModel;
class SpawnRowModel;
class WireMsgRowModel;
class WireParRowModel;
class ListCtrlView;

typedef csHash<csRef<iParameter>,csStringID> ParHash;

enum
{
  ID_Inv_Add = wxID_HIGHEST + 10000,
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
  iCelPlLayer* pl;
  UIManager* uiManager;
  EntityMode* emode;
  wxSizer* parentSizer;
  iCelEntityTemplate* tpl;
  iCelPropertyClassTemplate* pctpl;

  void OnChoicebookPageChange (wxChoicebookEvent& event);
  void OnUpdateEvent (wxCommandEvent& event);

  /**
   * Possibly switch the type of the PC. Do nothing if the PC is
   * already of the right type. Otherwise clear all properties.
   */
  void SwitchPCType (const char* pcType);

  // Properties
  ListCtrlView* propertyView;
  csRef<PropertyRowModel> propertyModel;
  UIDialog* propDialog;
  bool UpdateProperties ();
  void FillProperties ();

  // Inventory.
  ListCtrlView* inventoryView;
  csRef<InventoryRowModel> inventoryModel;
  UIDialog* invTempDialog;
  bool UpdateInventory ();
  void FillInventory ();

  // Quest.
  ListCtrlView* questView;
  csRef<QuestRowModel> questModel;
  UIDialog* questParDialog;
  bool UpdateQuest ();
  void FillQuest ();

  // Spawn.
  ListCtrlView* spawnView;
  csRef<SpawnRowModel> spawnModel;
  UIDialog* spawnTempDialog;
  bool UpdateSpawn ();
  void FillSpawn ();

  // Wire.
  ListCtrlView* wireMsgView;
  csRef<WireMsgRowModel> wireMsgModel;
  ListCtrlView* wireParView;
  csRef<WireParRowModel> wireParModel;
  UIDialog* wireParDialog;
  UIDialog* wireMsgDialog;
  bool UpdateWire ();
  void FillWire ();
  bool UpdateCurrentWireParams ();
  void OnWireMessageSelected (wxListEvent& event);
  void OnWireMessageDeselected (wxListEvent& event);

  // Update the property class. Returns false on error.
  bool UpdatePC ();

public:
  csHash<ParHash,csString> wireParams;
  void UpdateWireMsg ();

public:
  PropertyClassPanel (wxWindow* parent, UIManager* uiManager, EntityMode* emode);
  ~PropertyClassPanel();

  // Switch this dialog to editing of a PC.
  void SwitchToPC (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl);
  UIManager* GetUIManager () const { return uiManager; }
  iCelPlLayer* GetPL () const { return pl; }
  EntityMode* GetEntityMode () const { return emode; }

  UIDialog* GetPropertyDialog ();
  UIDialog* GetInventoryTemplateDialog ();
  UIDialog* GetQuestDialog ();
  UIDialog* GetSpawnTemplateDialog ();
  UIDialog* GetWireParDialog ();
  UIDialog* GetWireMsgDialog ();

  void Show () { wxPanel::Show (); parentSizer->Layout (); }
  void Hide () { wxPanel::Hide (); parentSizer->Layout (); }
  bool IsVisible () const { return IsShown (); }

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_pcdialog_h

