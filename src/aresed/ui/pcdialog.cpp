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

#include "pcdialog.h"
#include "uimanager.h"
#include "physicallayer/entitytpl.h"
#include "celtool/stdparams.h"
#include "tools/questmanager.h"
#include "../apparesed.h"
#include "listctrltools.h"
#include "../inspect.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PropertyClassDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), PropertyClassDialog :: OnOkButton)
  EVT_BUTTON (XRCID("cancelButton"), PropertyClassDialog :: OnCancelButton)
  EVT_BUTTON (XRCID("propertyAddButton"), PropertyClassDialog :: OnPropertyAdd)
  EVT_BUTTON (XRCID("propertyDeleteButton"), PropertyClassDialog :: OnPropertyDel)
  EVT_LIST_ITEM_SELECTED (XRCID("propertyListCtrl"), PropertyClassDialog :: OnPropertySelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("propertyListCtrl"), PropertyClassDialog :: OnPropertyDeselected)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void PropertyClassDialog::OnOkButton (wxCommandEvent& event)
{
  pctpl = 0;
  EndModal (TRUE);
}

void PropertyClassDialog::OnCancelButton (wxCommandEvent& event)
{
  pctpl = 0;
  EndModal (TRUE);
}

void PropertyClassDialog::Show ()
{
  ShowModal ();
}

static size_t FindNotebookPage (wxChoicebook* book, const char* name)
{
  wxString iname (name, wxConvUTF8);
  for (size_t i = 0 ; i < book->GetPageCount () ; i++)
  {
    wxString pageName = book->GetPageText (i);
    if (pageName == iname) return i;
  }
  return csArrayItemNotFound;
}

void PropertyClassDialog::UpdateWire ()
{
}

void PropertyClassDialog::FillWire ()
{
  wxListCtrl* outputList = XRCCTRL (*this, "wireOutputList", wxListCtrl);
  outputList->DeleteAllItems ();
  wxListCtrl* parList = XRCCTRL (*this, "wireParameterList", wxListCtrl);
  parList->DeleteAllItems ();
  wxTextCtrl* inputMaskText = XRCCTRL (*this, "wireInputMaskText", wxTextCtrl);
  inputMaskText->SetValue (wxT (""));

  if (csString ("pclogic.wire") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  csString inputMask = InspectTools::GetActionParameterValueString (pl, pctpl,
      "AddInput", "mask");
  inputMaskText->SetValue (wxString (inputMask, wxConvUTF8));

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csStringID msgID = pl->FetchStringID ("msgid");
    csStringID entityID = pl->FetchStringID ("entity");
    if (name == "AddOutput")
    {
      csString msgName;
      csString entName;
      while (params->HasNext ())
      {
	csStringID parid;
	iParameter* par = params->Next (parid);
	// @@@ No support for expressions.
	if (parid == msgID) msgName = par->Get (0);
	else if (parid == entityID) entName = par->Get (0);
      }
      ListCtrlTools::AddRow (outputList, msgName.GetData (), entName.GetData (), 0);
    }
  }
}

void PropertyClassDialog::UpdateSpawn ()
{
}

void PropertyClassDialog::FillSpawn ()
{
  wxListCtrl* list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  list->DeleteAllItems ();
  wxCheckBox* repeatCB = XRCCTRL (*this, "spawnRepeatCheckBox", wxCheckBox);
  repeatCB->SetValue (false);
  wxCheckBox* randomCB = XRCCTRL (*this, "spawnRandomCheckBox", wxCheckBox);
  randomCB->SetValue (false);
  wxCheckBox* spawnuniqueCB = XRCCTRL (*this, "spawnUniqueCheckBox", wxCheckBox);
  spawnuniqueCB->SetValue (false);
  wxCheckBox* namecounterCB = XRCCTRL (*this, "spawnNameCounterCheckBox", wxCheckBox);
  namecounterCB->SetValue (false);
  wxTextCtrl* minDelayText = XRCCTRL (*this, "spawnMinDelayText", wxTextCtrl);
  minDelayText->SetValue (wxT (""));
  wxTextCtrl* maxDelayText = XRCCTRL (*this, "spawnMaxDelayText", wxTextCtrl);
  maxDelayText->SetValue (wxT (""));
  wxTextCtrl* inhibitText = XRCCTRL (*this, "spawnInhibitText", wxTextCtrl);
  inhibitText->SetValue (wxT (""));

  if (csString ("pclogic.spawn") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  bool repeat = InspectTools::GetActionParameterValueBool (pl, pctpl,
      "SetTiming", "repeat");
  repeatCB->SetValue (repeat);
  bool random = InspectTools::GetActionParameterValueBool (pl, pctpl,
      "SetTiming", "random");
  randomCB->SetValue (random);
  bool valid;
  long mindelay = InspectTools::GetActionParameterValueLong (pl, pctpl,
      "SetTiming", "mindelay", &valid);
  if (valid)
  {
    csString s; s.Format ("%ld", mindelay);
    minDelayText->SetValue (wxString (s, wxConvUTF8));
  }
  long maxdelay = InspectTools::GetActionParameterValueLong (pl, pctpl,
      "SetTiming", "maxdelay", &valid);
  if (valid)
  {
    csString s; s.Format ("%ld", maxdelay);
    maxDelayText->SetValue (wxString (s, wxConvUTF8));
  }

  bool unique = InspectTools::GetPropertyValueBool (pl, pctpl, "spawnunique");
  spawnuniqueCB->SetValue (unique);
  bool namecounter = InspectTools::GetPropertyValueBool (pl, pctpl, "namecounter");
  namecounterCB->SetValue (namecounter);

  long inhibit = InspectTools::GetActionParameterValueLong (pl, pctpl,
      "Inhibit", "count", &valid);
  if (valid)
  {
    csString s; s.Format ("%ld", inhibit);
    inhibitText->SetValue (wxString (s, wxConvUTF8));
  }

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csStringID tplID = pl->FetchStringID ("template");
    if (name == "AddEntityTemplateType")
    {
      csString tplName;
      while (params->HasNext ())
      {
	csStringID parid;
	iParameter* par = params->Next (parid);
	// @@@ No support for expressions.
	if (parid == tplID) tplName = par->Get (0);
      }
      if (!tplName.IsEmpty ())
        ListCtrlTools::AddRow (list, tplName.GetData (), 0);
    }
  }
}

void PropertyClassDialog::UpdateQuest ()
{
}

void PropertyClassDialog::FillQuest ()
{
  wxListBox* list = XRCCTRL (*this, "questStateBox", wxListBox);
  list->Clear ();
  wxTextCtrl* text = XRCCTRL (*this, "questText", wxTextCtrl);
  text->SetValue (wxT (""));

  if (csString ("pclogic.quest") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();
  csString questName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "NewQuest", "name");
  if (questName.IsEmpty ()) return;

  text->SetValue (wxString (questName, wxConvUTF8));

  csRef<iQuestManager> quest_mgr = csQueryRegistryOrLoad<iQuestManager> (
    uiManager->GetApp ()->GetObjectRegistry (),
    "cel.manager.quests");
  iQuestFactory* questFact = quest_mgr->GetQuestFactory (questName);
  if (!questFact) return;

  csString defaultState = InspectTools::GetPropertyValueString (pl, pctpl, "state");

  wxArrayString states;
  csRef<iQuestStateFactoryIterator> it = questFact->GetStates ();
  size_t selIdx = csArrayItemNotFound;
  size_t i = 0;
  while (it->HasNext ())
  {
    iQuestStateFactory* stateFact = it->Next ();
    states.Add (wxString (stateFact->GetName (), wxConvUTF8));
    if (defaultState == stateFact->GetName ()) selIdx = i;
    i++;
  }
  list->InsertItems (states, 0);
  if (selIdx != csArrayItemNotFound)
    list->SetSelection (selIdx);
}

void PropertyClassDialog::UpdateInventory ()
{
}

void PropertyClassDialog::FillInventory ()
{
  wxListCtrl* list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  list->DeleteAllItems ();
  wxTextCtrl* lootText = XRCCTRL (*this, "inventoryLootTextCtrl", wxTextCtrl);
  lootText->SetValue (wxT (""));

  if (csString ("pctools.inventory") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csStringID nameID = pl->FetchStringID ("name");
    csStringID amountID = pl->FetchStringID ("amount");
    if (name == "AddTemplate")
    {
      csString parName;
      csString parAmount;
      while (params->HasNext ())
      {
	csStringID parid;
	iParameter* par = params->Next (parid);
	// @@@ No support for expressions.
	if (parid == nameID) parName = par->Get (0);
	else if (parid == amountID) parAmount = par->Get (0);
      }
      ListCtrlTools::AddRow (list, parName.GetData (), parAmount.GetData (), 0);
    }
  }

  csString lootName = InspectTools::GetActionParameterValueString (pl, pctpl,
      "SetLootGenerator", "name");
  lootText->SetValue (wxString (lootName, wxConvUTF8));
}

void PropertyClassDialog::OnPropertySelected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "propertyDeleteButton", wxButton);
  delButton->Enable ();

  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  wxTextCtrl* nameText = XRCCTRL (*this, "propertyNameText", wxTextCtrl);
  wxTextCtrl* valueText = XRCCTRL (*this, "propertyValueText", wxTextCtrl);
  wxChoice* typeChoice = XRCCTRL (*this, "propertyTypeChoice", wxChoice);

  selIndex = event.GetIndex ();
  csStringArray row = ListCtrlTools::ReadRow (list, selIndex);
  nameText->SetValue (wxString (row[0], wxConvUTF8));
  valueText->SetValue (wxString (row[1], wxConvUTF8));
  typeChoice->SetStringSelection (wxString (row[2], wxConvUTF8));
}

void PropertyClassDialog::OnPropertyDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "propertyDeleteButton", wxButton);
  delButton->Disable ();
}

void PropertyClassDialog::OnPropertyAdd (wxCommandEvent& event)
{
  wxTextCtrl* nameText = XRCCTRL (*this, "propertyNameText", wxTextCtrl);
  wxTextCtrl* valueText = XRCCTRL (*this, "propertyValueText", wxTextCtrl);
  wxChoice* typeChoice = XRCCTRL (*this, "propertyTypeChoice", wxChoice);
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);

  csString name = (const char*)nameText->GetValue ().mb_str (wxConvUTF8);
  csString value = (const char*)valueText->GetValue ().mb_str (wxConvUTF8);
  csString type = (const char*)typeChoice->GetStringSelection ().mb_str (wxConvUTF8);

  long idx = ListCtrlTools::FindRow (list, 0, name.GetData ());
  if (idx == -1)
    ListCtrlTools::AddRow (list, name.GetData (), value.GetData (), type.GetData (), 0);
  else
    ListCtrlTools::ReplaceRow (list, idx, name.GetData (), value.GetData (), type.GetData (), 0);
}

void PropertyClassDialog::OnPropertyDel (wxCommandEvent& event)
{
  if (selIndex == -1) return;
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  list->DeleteItem (selIndex);
  list->SetColumnWidth (0, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (1, wxLIST_AUTOSIZE_USEHEADER);
  list->SetColumnWidth (2, wxLIST_AUTOSIZE_USEHEADER);
  selIndex = -1;
}

void PropertyClassDialog::UpdateProperties ()
{
}

void PropertyClassDialog::FillProperties ()
{
  wxListCtrl* list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  list->DeleteAllItems ();

  if (csString ("pctools.properties") != pctpl->GetName ()) return;

  iCelPlLayer* pl = uiManager->GetApp ()->GetAresView ()->GetPlLayer ();

  for (size_t i = 0 ; i < pctpl->GetPropertyCount () ; i++)
  {
    // @@@ Should do something with actions.
    csStringID id;
    celData data;
    csRef<iCelParameterIterator> params = pctpl->GetProperty (i, id, data);
    csString name = pl->FetchString (id);
    csString value, type;
    celParameterTools::ToString (data, value);
    switch (data.type)
    {
      case CEL_DATA_NONE: type = "none"; break;
      case CEL_DATA_BOOL: type = "bool"; break;
      case CEL_DATA_BYTE: type = "byte"; break;
      case CEL_DATA_WORD: type = "word"; break;
      case CEL_DATA_LONG: type = "long"; break;
      case CEL_DATA_UBYTE: type = "ubyte"; break;
      case CEL_DATA_UWORD: type = "uword"; break;
      case CEL_DATA_ULONG: type = "ulong"; break;
      case CEL_DATA_FLOAT: type = "float"; break;
      case CEL_DATA_VECTOR2: type = "vector2"; break;
      case CEL_DATA_VECTOR3: type = "vector3"; break;
      case CEL_DATA_VECTOR4: type = "vector4"; break;
      case CEL_DATA_STRING: type = "string"; break;
      case CEL_DATA_PCLASS: type = "pc"; break;
      case CEL_DATA_ENTITY: type = "entity"; break;
      case CEL_DATA_COLOR: type = "color"; break;
      case CEL_DATA_COLOR4: type = "color4"; break;
      default: type = "?"; break;
    }
    ListCtrlTools::AddRow (list, name.GetData (), value.GetData (), type.GetData (), 0);
  }
}

void PropertyClassDialog::SwitchToPC (iCelPropertyClassTemplate* pctpl)
{
  PropertyClassDialog::pctpl = pctpl;

  csString pcName = pctpl->GetName ();
  csString tagName = pctpl->GetTag ();

  wxChoicebook* book = XRCCTRL (*this, "pcChoicebook", wxChoicebook);
  size_t pageIdx = FindNotebookPage (book, pcName);
  book->ChangeSelection (pageIdx);

  wxTextCtrl* text = XRCCTRL (*this, "tagTextCtrl", wxTextCtrl);
  text->SetValue (wxString (tagName, wxConvUTF8));

  FillProperties ();
  FillInventory ();
  FillQuest ();
  FillSpawn ();
  FillWire ();
}

PropertyClassDialog::PropertyClassDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("PropertyClassDialog"));

  wxListCtrl* list;

  list = XRCCTRL (*this, "propertyListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);
  ListCtrlTools::SetColumn (list, 2, "Type", 100);

  list = XRCCTRL (*this, "inventoryTemplateListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Amount", 100);

  list = XRCCTRL (*this, "spawnTemplateListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Template", 100);

  list = XRCCTRL (*this, "wireOutputList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Message", 100);
  ListCtrlTools::SetColumn (list, 1, "Entity", 100);

  list = XRCCTRL (*this, "wireParameterList", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Name", 100);
  ListCtrlTools::SetColumn (list, 1, "Value", 100);
  ListCtrlTools::SetColumn (list, 2, "Type", 100);

  wxButton* delButton = XRCCTRL (*this, "propertyDeleteButton", wxButton);
  delButton->Disable ();

  selIndex = -1;
}

PropertyClassDialog::~PropertyClassDialog ()
{
}


