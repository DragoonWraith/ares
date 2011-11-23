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

long ListCtrlTools::AddRow (wxListCtrl* list, const char* value, ...)
{
  long idx = list->InsertItem (list->GetItemCount (), wxString (value, wxConvUTF8));
  int col = 1;
  va_list args;
  va_start (args, value);
  const char* value2 = va_arg (args, char*);
  while (value2 != 0)
  {
    list->SetItem (idx, col++, wxString (value2, wxConvUTF8));
    value2 = va_arg (args, char*);
  }
  for (int i = 0 ; i < col ; i++)
    list->SetColumnWidth (i, wxLIST_AUTOSIZE_USEHEADER);
  return idx;
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
  colPath.SetText (wxString (name, wxConvUTF8));
  colPath.SetWidth (width);
  list->InsertColumn (idx, colPath);
}

