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

  csStringArray columns = model->GetColumns ();
  columnCount = columns.GetSize ();
  for (size_t i = 0 ; i < columnCount ; i++)
    ListCtrlTools::SetColumn (list, i, columns[i], 100);
}

ListCtrlView::~ListCtrlView ()
{
  UnbindModel ();
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

void ListCtrlView::Update ()
{
  model->StartUpdate ();
  for (int r = 0 ; r < list->GetItemCount () ; r++)
  {
    csStringArray row = ListCtrlTools::ReadRow (list, r);
    if (!model->UpdateRow (row)) break;
  }
  model->FinishUpdate ();
}

csStringArray ListCtrlView::DialogEditRow (const csStringArray& origRow)
{
  UIDialog* dialog = model->GetEditorDialog ();
  dialog->Clear ();
  csStringArray columns = model->GetColumns ();
  if (origRow.GetSize () >= columnCount)
    for (size_t i = 0 ; i < columnCount ; i++)
      dialog->SetText (columns[i], origRow[i]);
  csStringArray ar;
  if (dialog->Show (0))
  {
    const csHash<csString,csString>& fields = dialog->GetFieldContents ();
    for (size_t i = 0 ; i < columnCount ; i++)
      ar.Push (fields.Get (columns[i], ""));
  }
  return ar;
}

void ListCtrlView::OnAdd (wxCommandEvent& event)
{
  csStringArray empty;
  csStringArray row;
  if (model->GetEditorDialog ())
    row = DialogEditRow (empty);
  else
    row = model->EditRow (empty);
  if (row.GetSize () > 0)
  {
    ListCtrlTools::AddRow (list, row);
    Update ();
  }
}

void ListCtrlView::OnEdit (wxCommandEvent& event)
{
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  csStringArray oldRow = ListCtrlTools::ReadRow (list, idx);
  csStringArray row;
  if (model->GetEditorDialog ())
    row = DialogEditRow (oldRow);
  else
    row = model->EditRow (oldRow);
  if (row.GetSize () > 0)
  {
    ListCtrlTools::ReplaceRow (list, idx, row);
    Update ();
  }
}

void ListCtrlView::OnDelete (wxCommandEvent& event)
{
  long idx = ListCtrlTools::GetFirstSelectedRow (list);
  if (idx == -1) return;
  list->DeleteItem (idx);
  for (size_t i = 0 ; i < columnCount ; i++)
    list->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);
  Update ();
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

//-----------------------------------------------------------------------------

csStringArray ListCtrlTools::ReadRow (wxListCtrl* list, int row)
{
  wxListItem rowInfo;
  csStringArray rc;

  rowInfo.m_itemId = row;
  rowInfo.m_mask = wxLIST_MASK_TEXT;
  for (int i = 0 ; i < list->GetColumnCount () ; i++)
  {
    rowInfo.m_col = i;
    list->GetItem (rowInfo);
    csString col = (const char*)(rowInfo.m_text.mb_str (wxConvUTF8)); 
    rc.Push (col);
  }
  return rc;
}

long ListCtrlTools::FindRow (wxListCtrl* list, int col, const char* value)
{
  wxListItem rowInfo;
  for (int row = 0 ; row < list->GetItemCount () ; row++)
  {
    rowInfo.m_itemId = row;
    rowInfo.m_col = col;
    rowInfo.m_mask = wxLIST_MASK_TEXT;
    list->GetItem (rowInfo);
    csString data = (const char*)(rowInfo.m_text.mb_str (wxConvUTF8)); 
    if (data == value) return row;
  }
  return -1;
}

long ListCtrlTools::AddRow (wxListCtrl* list, const char* value, ...)
{
  long idx = list->InsertItem (list->GetItemCount (), wxString::FromUTF8 (value));
  int col = 1;
  va_list args;
  va_start (args, value);
  const char* value2 = va_arg (args, char*);
  while (value2 != 0)
  {
    list->SetItem (idx, col++, wxString::FromUTF8 (value2));
    value2 = va_arg (args, char*);
  }
  va_end (args);
  for (int i = 0 ; i < col ; i++)
    list->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);
  return idx;
}

long ListCtrlTools::AddRow (wxListCtrl* list, const csStringArray& values)
{
  long idx = list->InsertItem (list->GetItemCount (), wxString::FromUTF8 (values[0]));
  for (size_t col = 1 ; col < values.GetSize () ; col++)
    list->SetItem (idx, col, wxString::FromUTF8 (values[col]));
  for (size_t i = 0 ; i < values.GetSize () ; i++)
    list->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);
  return idx;
}

void ListCtrlTools::ReplaceRow (wxListCtrl* list, int idx, const char* value, ...)
{
  list->DeleteItem (idx);
  list->InsertItem (idx, wxString::FromUTF8 (value));
  int col = 1;
  va_list args;
  va_start (args, value);
  const char* value2 = va_arg (args, char*);
  while (value2 != 0)
  {
    list->SetItem (idx, col++, wxString::FromUTF8 (value2));
    value2 = va_arg (args, char*);
  }
  va_end (args);
  for (int i = 0 ; i < col ; i++)
    list->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);
}

void ListCtrlTools::ReplaceRow (wxListCtrl* list, int idx, const csStringArray& values)
{
  list->DeleteItem (idx);
  list->InsertItem (idx, wxString::FromUTF8 (values[0]));
  for (size_t col = 1 ; col < values.GetSize () ; col++)
    list->SetItem (idx, col, wxString::FromUTF8 (values[col]));
  for (size_t i = 0 ; i < values.GetSize () ; i++)
    list->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);
}

void ListCtrlTools::ColorRow (wxListCtrl* list, int idx,
    unsigned char red, unsigned char green, unsigned char blue)
{
  wxListItem rowInfo;

  rowInfo.m_itemId = idx;
  rowInfo.m_mask = wxLIST_MASK_TEXT;
  for (int i = 0 ; i < list->GetColumnCount () ; i++)
  {
    rowInfo.m_col = i;
    list->GetItem (rowInfo);
    rowInfo.SetTextColour (wxColour (red, green, blue));
    list->SetItem (rowInfo);
  }
}

void ListCtrlTools::BackgroundColorRow (wxListCtrl* list, int idx,
    unsigned char red, unsigned char green, unsigned char blue)
{
  wxListItem rowInfo;

  rowInfo.m_itemId = idx;
  rowInfo.m_mask = wxLIST_MASK_TEXT;
  for (int i = 0 ; i < list->GetColumnCount () ; i++)
  {
    rowInfo.m_col = i;
    list->GetItem (rowInfo);
    rowInfo.SetBackgroundColour (wxColour (red, green, blue));
    list->SetItem (rowInfo);
  }
}

void ListCtrlTools::SetColumn (wxListCtrl* list, int idx, const char* name, int width)
{
  wxListItem colPath;
  colPath.SetId (idx);
  csString name2 = name;
  name2 += "      ";
  colPath.SetText (wxString::FromUTF8 (name2));
  colPath.SetWidth (width);
  list->InsertColumn (idx, colPath);
}

long ListCtrlTools::GetFirstSelectedRow (wxListCtrl* list)
{
  return list->GetNextItem (-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
}

bool ListCtrlTools::CheckHitList (wxListCtrl* list, bool& hasItem,
    const wxPoint& pos)
{
  int flags = 0;
  if (!list->IsShownOnScreen ()) return false;
  long idx = list->HitTest (list->ScreenToClient (pos), flags, 0);
  if (idx != wxNOT_FOUND) { hasItem = true; return true; }
  //else if (list->GetRect ().Contains (list->ScreenToClient (pos)))
  else if (list->GetScreenRect ().Contains (pos))
  { hasItem = false; return true; }
  return false;
}

bool ListCtrlTools::CheckHitList (wxListBox* list, bool& hasItem,
    const wxPoint& pos)
{
  if (!list->IsShownOnScreen ()) return false;
  long idx = list->HitTest (list->ScreenToClient (pos));
  if (idx != wxNOT_FOUND) { hasItem = true; return true; }
  //else if (list->GetRect ().Contains (list->ScreenToClient (pos)))
  else if (list->GetScreenRect ().Contains (pos))
  { hasItem = false; return true; }
  return false;
}

