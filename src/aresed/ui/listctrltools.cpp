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

void ListCtrlTools::SelectRow (wxListCtrl* list, int row, bool sendEvent)
{
  if (row >= list->GetItemCount ())
  {
    // If we don't have sufficient rows we also clear the selection but
    // in that case we have to send an event in case we wanted an event
    // when a new selection was made.
    ClearSelection (list, sendEvent);
    return;
  }
  ClearSelection (list);
  wxListItem rowInfo;
  rowInfo.m_itemId = row;
  rowInfo.m_col = 0;
  list->GetItem (rowInfo);
  list->SetItemState (rowInfo, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
  if (sendEvent)
  {
    wxCommandEvent event (wxEVT_COMMAND_LIST_ITEM_SELECTED);
    list->AddPendingEvent (event);
  }
}

void ListCtrlTools::ClearSelection (wxListCtrl* list, bool sendEvent)
{
  wxListItem rowInfo;
  for (int row = 0 ; row < list->GetItemCount () ; row++)
  {
    rowInfo.m_itemId = row;
    rowInfo.m_col = 0;
    list->GetItem (rowInfo);
    list->SetItemState (rowInfo, 0, wxLIST_STATE_SELECTED);
  }
  if (sendEvent)
  {
    wxCommandEvent event (wxEVT_COMMAND_LIST_ITEM_DESELECTED);
    list->AddPendingEvent (event);
  }
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

