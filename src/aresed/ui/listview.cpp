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

#include <crystalspace.h>

#include "listctrltools.h"
#include "listview.h"
#include "../models/rowmodel.h"
#include "uimanager.h"

//-----------------------------------------------------------------------------

enum
{
  ID_Add = wxID_HIGHEST + 10000,
  ID_Edit,
  ID_Delete,
};

void ListCtrlView::UnbindModel ()
{
  if (!model) return;
  list->Disconnect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (ListCtrlView :: OnContextMenu),
      0, static_cast<wxEvtHandler*> (this));
  list->Disconnect (ID_Add, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (ListCtrlView :: OnAdd), 0, this);
  list->Disconnect (ID_Delete, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (ListCtrlView :: OnDelete), 0, this);
  list->Disconnect (ID_Edit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (ListCtrlView :: OnEdit), 0, this);
  model = 0;
}

void ListCtrlView::BindModel (RowModel* model)
{
  if (model == ListCtrlView::model) return;
  UnbindModel ();
  ListCtrlView::model = model;

  list->Connect (wxEVT_CONTEXT_MENU, wxContextMenuEventHandler (ListCtrlView :: OnContextMenu), 0, this);
  list->Connect (ID_Add, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (ListCtrlView :: OnAdd), 0, this);
  list->Connect (ID_Delete, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (ListCtrlView :: OnDelete), 0, this);
  list->Connect (ID_Edit, wxEVT_COMMAND_MENU_SELECTED,
	  wxCommandEventHandler (ListCtrlView :: OnEdit), 0, this);

  const char* columnsString = model->GetColumns ();
  columns.DeleteAll ();
  columns.SplitString (columnsString, ",");
  for (size_t i = 0 ; i < columns.GetSize () ; i++)
    ListCtrlTools::SetColumn (list, i, columns[i], 100);
}

ListCtrlView::~ListCtrlView ()
{
  UnbindModel ();
  if (forcedDialog && ownForcedDialog)
    delete forcedDialog;
}

void ListCtrlView::Refresh ()
{
  list->DeleteAllItems ();
  model->ResetIterator ();
  while (model->HasRows ())
  {
    csStringArray row = model->NextRow ();
    ListCtrlTools::AddRow (list, row);
  }
}

csStringArray ListCtrlView::DialogEditRow (const csStringArray& origRow)
{
  UIDialog* dialog = forcedDialog ? forcedDialog : model->GetEditorDialog ();
  dialog->Clear ();
  if (origRow.GetSize () >= columns.GetSize ())
    for (size_t i = 0 ; i < columns.GetSize () ; i++)
      dialog->SetText (columns[i], origRow[i]);
  csStringArray ar;
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    for (size_t i = 0 ; i < columns.GetSize () ; i++)
      ar.Push (fields.Get (columns[i], ""));
  }
  return ar;
}

csStringArray ListCtrlView::DoDialog (const csStringArray& origRow)
{
  if (forcedDialog || model->GetEditorDialog ())
    return DialogEditRow (origRow);
  return model->EditRow (origRow);
}

void ListCtrlView::SetEditorDialog (UIDialog* dialog, bool own)
{
  if (forcedDialog && ownForcedDialog && forcedDialog != dialog) delete forcedDialog;
  forcedDialog = dialog;
  ownForcedDialog = own;
}

void ListCtrlView::OnAdd (wxCommandEvent& event)
{
  csStringArray empty;
  csStringArray row = DoDialog (empty);
  if (row.GetSize () > 0)
  {
    if (model->AddRow (row))
    {
      ListCtrlTools::AddRow (list, row);
      model->FinishUpdate ();
    }
  }
}

void ListCtrlView::OnEdit (wxCommandEvent& event)
{
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray oldRow = ListCtrlTools::ReadRow (list, idx);
  csStringArray row = DoDialog (oldRow);
  if (row.GetSize () > 0)
  {
    if (!model->DeleteRow (oldRow)) return;
    if (model->AddRow (row))
    {
      ListCtrlTools::ReplaceRow (list, idx, row);
      model->FinishUpdate ();
    }
    else
      model->AddRow (oldRow);
  }
}

void ListCtrlView::OnDelete (wxCommandEvent& event)
{
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray oldRow = ListCtrlTools::ReadRow (list, idx);
  if (!model->DeleteRow (oldRow)) return;

  list->DeleteItem (idx);
  for (size_t i = 0 ; i < columns.GetSize () ; i++)
    list->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);

  model->FinishUpdate ();
}

void ListCtrlView::OnContextMenu (wxContextMenuEvent& event)
{
  bool hasItem;
  if (ListCtrlTools::CheckHitList (list, hasItem, event.GetPosition ()))
  {
    wxMenu contextMenu;
    contextMenu.Append(ID_Add, wxT ("&Add"));
    if (hasItem)
    {
      if (model->IsEditAllowed ())
        contextMenu.Append(ID_Edit, wxT ("&Edit"));
      contextMenu.Append(ID_Delete, wxT ("&Delete"));
    }
    list->PopupMenu (&contextMenu);
  }
}


