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

#include "../apparesed.h"
#include "../aresview.h"
#include "celldialog.h"
#include "uimanager.h"
#include "edcommon/listctrltools.h"

//--------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CellDialog, wxDialog)
  EVT_BUTTON (XRCID("okButton"), CellDialog :: OnOkButton)
  EVT_BUTTON (XRCID("setDefaultButton"), CellDialog :: OnSetDefaultButton)
  EVT_BUTTON (XRCID("addCellButton"), CellDialog :: OnAddCellButton)
  EVT_BUTTON (XRCID("delCellButton"), CellDialog :: OnDelCellButton)
  EVT_LIST_ITEM_SELECTED (XRCID("cellListCtrl"), CellDialog :: OnCellSelected)
  EVT_LIST_ITEM_DESELECTED (XRCID("cellListCtrl"), CellDialog :: OnCellDeselected)
END_EVENT_TABLE()

//--------------------------------------------------------------------------

void CellDialog::OnOkButton (wxCommandEvent& event)
{
  EndModal (TRUE);
}

void CellDialog::OnSetDefaultButton (wxCommandEvent& event)
{
  wxTextCtrl* nameText = XRCCTRL (*this, "cellNameTextCtrl", wxTextCtrl);
  csString name = (const char*)(nameText->GetValue ().mb_str (wxConvUTF8));
  iPcDynamicWorld* dynworld = uiManager->GetApp ()->GetAresView ()->GetDynamicWorld ();
  iDynamicCell* cell = dynworld->FindCell (name);
  if (cell)
  {
    uiManager->GetApp ()->GetAresView ()->WarpCell (cell);
    UpdateList ();
  }
  else
  {
    uiManager->Error ("'%s' is not a valid cell!", (const char*)name);
  }
}

void CellDialog::OnAddCellButton (wxCommandEvent& event)
{
  wxTextCtrl* nameText = XRCCTRL (*this, "cellNameTextCtrl", wxTextCtrl);
  csString name = (const char*)(nameText->GetValue ().mb_str (wxConvUTF8));
  iPcDynamicWorld* dynworld = uiManager->GetApp ()->GetAresView ()->GetDynamicWorld ();
  iDynamicCell* cell = dynworld->FindCell (name);
  if (!cell)
  {
    csRef<iEngine> engine = csQueryRegistry<iEngine> (
      uiManager->GetApp ()->GetObjectRegistry ());
    iSector* sector = engine->FindSector (name);
    if (!sector) sector = engine->CreateSector (name);
    cell = dynworld->AddCell (name, sector);
  }
  UpdateList ();
}

void CellDialog::OnDelCellButton (wxCommandEvent& event)
{
  uiManager->Message ("Deleting cells is not implemented yet!");
  //if (selIndex >= 0)
  //{
    //wxListCtrl* list = XRCCTRL (*this, "cellListCtrl", wxListCtrl);
    //list->DeleteItem (selIndex);
    //list->SetColumnWidth (0, wxLIST_AUTOSIZE | wxLIST_AUTOSIZE_USEHEADER);
    //selIndex = -1;
  //}
}

void CellDialog::OnCellSelected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "delCellButton", wxButton);
  delButton->Enable ();
  wxListCtrl* list = XRCCTRL (*this, "cellListCtrl", wxListCtrl);
  selIndex = event.GetIndex ();
  csStringArray row = ListCtrlTools::ReadRow (list, selIndex);
  wxTextCtrl* nameText = XRCCTRL (*this, "cellNameTextCtrl", wxTextCtrl);
  nameText->SetValue (wxString::FromUTF8 (row[0]));
}

void CellDialog::OnCellDeselected (wxListEvent& event)
{
  wxButton* delButton = XRCCTRL (*this, "delCellButton", wxButton);
  delButton->Disable ();
}

void CellDialog::UpdateList ()
{
  wxListCtrl* list = XRCCTRL (*this, "cellListCtrl", wxListCtrl);
  list->DeleteAllItems ();

  csRef<iEngine> engine = csQueryRegistry<iEngine> (
      uiManager->GetApp ()->GetObjectRegistry ());
  iPcDynamicWorld* dynworld = uiManager->GetApp ()->GetAresView ()->GetDynamicWorld ();

  csSet<csString> namesSet;
  iSectorList* sl = engine->GetSectors ();
  for (int i = 0 ; i < sl->GetCount () ; i++)
    namesSet.Add (sl->Get (i)->QueryObject ()->GetName ());

  csRef<iDynamicCellIterator> cellIt = dynworld->GetCells ();
  while (cellIt->HasNext ())
  {
    iDynamicCell* cell = cellIt->NextCell ();
    namesSet.Add (cell->GetName ());
  }

  csStringArray namesArray;
  csSet<csString>::GlobalIterator it = namesSet.GetIterator ();
  while (it.HasNext ())
    namesArray.Push (it.Next ());
  namesArray.Sort ();

  for (size_t i = 0 ; i < namesArray.GetSize () ; i++)
  {
    const char* name = namesArray[i];
    iSector* sector = engine->FindSector (name);
    iDynamicCell* cell = dynworld->FindCell (name);
    csString info;
    if (sector)
    {
      info.AppendFmt ("%d meshes", sector->GetMeshes ()->GetCount());
      if (cell) info += ", ";
    }
    if (cell) info.AppendFmt ("%d objects", cell->GetObjectCount ());

    int idx = ListCtrlTools::AddRow (list, name, (const char*)info, (const char*)0);
    if (!cell)
      ListCtrlTools::ColorRow (list, idx, 128, 128, 128);
    else if (cell == dynworld->GetCurrentCell ())
      ListCtrlTools::BackgroundColorRow (list, idx, 150, 240, 210);
  }
}

void CellDialog::Show ()
{
  wxButton* delButton = XRCCTRL (*this, "delCellButton", wxButton);
  delButton->Disable ();
  selIndex = -1;
  UpdateList ();
  ShowModal ();
}

CellDialog::CellDialog (wxWindow* parent, UIManager* uiManager) :
  uiManager (uiManager)
{
  wxXmlResource::Get()->LoadDialog (this, parent, wxT ("CellDialog"));
  wxListCtrl* list = XRCCTRL (*this, "cellListCtrl", wxListCtrl);
  ListCtrlTools::SetColumn (list, 0, "Sector/Cell", 300);
  ListCtrlTools::SetColumn (list, 1, "Information", 300);
}

CellDialog::~CellDialog ()
{
}


