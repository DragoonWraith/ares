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
class UIManager;

typedef csHash<csRef<iParameter>,csStringID> ParHash;

struct PCEditCallback : public csRefCount
{
  virtual void OkPressed (iCelPropertyClassTemplate* pctpl) = 0;
};

class PropertyClassDialog : public wxDialog
{
private:
  UIManager* uiManager;
  iCelEntityTemplate* tpl;
  iCelPropertyClassTemplate* pctpl;

  csRef<PCEditCallback> callback;

  void OnOkButton (wxCommandEvent& event);
  void OnCancelButton (wxCommandEvent& event);

  void AddRowFromInput (const char* listComp,
    const char* nameComp, const char* valueComp, const char* typeComp);
  void FillFieldsFromRow (const char* listComp,
    const char* nameComp, const char* valueComp, const char* typeComp,
    const char* delComp,
    long selIndex);

  // Properties
  long propSelIndex;
  void UpdateProperties ();
  void FillProperties ();
  void OnPropertyAdd (wxCommandEvent& event);
  void OnPropertyDel (wxCommandEvent& event);
  void OnPropertySelected (wxListEvent& event);
  void OnPropertyDeselected (wxListEvent& event);

  // Inventory.
  long invSelIndex;
  void UpdateInventory ();
  void FillInventory ();
  void OnInventoryTemplateAdd (wxCommandEvent& event);
  void OnInventoryTemplateDel (wxCommandEvent& event);
  void OnInvTemplateSelected (wxListEvent& event);
  void OnInvTemplateDeselected (wxListEvent& event);

  // Quest.
  long questSelIndex;
  void UpdateQuest ();
  void FillQuest ();
  void OnQuestParameterAdd (wxCommandEvent& event);
  void OnQuestParameterDel (wxCommandEvent& event);
  void OnQuestParameterSelected (wxListEvent& event);
  void OnQuestParameterDeselected (wxListEvent& event);

  // Spawn.
  long spawnSelIndex;
  void UpdateSpawn ();
  void FillSpawn ();
  void OnSpawnTemplateAdd (wxCommandEvent& event);
  void OnSpawnTemplateDel (wxCommandEvent& event);
  void OnSpawnTemplateSelected (wxListEvent& event);
  void OnSpawnTemplateDeselected (wxListEvent& event);

  // Wire.
  int wireMsgSelIndex;
  int wireParSelIndex;
  csHash<ParHash,csString> wireParams;
  void UpdateWire ();
  void FillWire ();
  void UpdateCurrentWireParams ();
  void OnWireMessageAdd (wxCommandEvent& event);
  void OnWireMessageDel (wxCommandEvent& event);
  void OnWireMessageSelected (wxListEvent& event);
  void OnWireMessageDeselected (wxListEvent& event);
  void OnWireParameterAdd (wxCommandEvent& event);
  void OnWireParameterDel (wxCommandEvent& event);
  void OnWireParameterSelected (wxListEvent& event);
  void OnWireParameterDeselected (wxListEvent& event);

  // Update the property class. Returns false on error.
  bool UpdatePC ();

public:
  PropertyClassDialog (wxWindow* parent, UIManager* uiManager);
  ~PropertyClassDialog();

  // Switch this dialog to editing of a PC. If the PC is null then we
  // are going to create a new pctpl.
  void SwitchToPC (iCelEntityTemplate* tpl, iCelPropertyClassTemplate* pctpl);

  void Show (PCEditCallback* cb);

  DECLARE_EVENT_TABLE ();
};

#endif // __appares_pcdialog_h

