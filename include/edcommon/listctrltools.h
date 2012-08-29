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

#ifndef __appares_listctrltools_h
#define __appares_listctrltools_h

#include <csutil/stringarray.h>
#include "aresextern.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/listbox.h>
#include <wx/xrc/xmlres.h>

/**
 * Various tools for lists.
 */
class ARES_EDCOMMON_EXPORT ListCtrlTools
{
public:
  /**
   * Read a row from the list control.
   */
  static csStringArray ReadRow (wxListCtrl* list, int row);

  /**
   * Add a row to the list control (at the end).
   */
  static long AddRow (wxListCtrl* list, const char* value, ...);

  /**
   * Add a row to the list control (at the end).
   */
  static long AddRow (wxListCtrl* list, const csStringArray& values);

  /**
   * Insert a row in the list control.
   */
  static long InsertRow (wxListCtrl* list, int idx, const char* value, ...);

  /**
   * Insert a row in the list control.
   */
  static long InsertRow (wxListCtrl* list, int idx, const csStringArray& values);

  /**
   * Replace a row in the list control.
   */
  static void ReplaceRow (wxListCtrl* list, int idx, const char* value, ...);

  /**
   * Replace a row in the list control.
   */
  static void ReplaceRow (wxListCtrl* list, int idx, const csStringArray& values);

  /**
   * Set the color of some row.
   */
  static void ColorRow (wxListCtrl* list, int idx,
    unsigned char red, unsigned char green, unsigned char blue);

  /**
   * Set the background color of some row.
   */
  static void BackgroundColorRow (wxListCtrl* list, int idx,
    unsigned char red, unsigned char green, unsigned char blue);

  /**
   * Add a column.
   */
  static void SetColumn (wxListCtrl* list, int idx, const char* name, int width);

  /**
   * Find a row index based on a given value in a column.
   * Returns -1 if not found.
   */
  static long FindRow (wxListCtrl* list, int col, const char* value);

  /**
   * Return the index of the first selected row or -1 if nothing is selected.
   */
  static long GetFirstSelectedRow (wxListCtrl* list);

  /**
   * Return the indices of all selected rows.
   */
  static csArray<long> GetSelectedRowIndices (wxListCtrl* list);

  /**
   * Select a row in the list.
   * If 'addSelection' is true then the selection is extended with this row.
   */
  static void SelectRow (wxListCtrl* list, int row, bool sendEvent = false,
      bool addSelection = false);

  /**
   * Clear selection in the list.
   * If sendEvent is true then a wxEVT_COMMAND_LIST_ITEM_DESELECTED is sent.
   */
  static void ClearSelection (wxListCtrl* list, bool sendEvent = false);

  /**
   * Check if a list control is hit with the given point (in screen space).
   * This is mostly used in combination with context menus.
   * 'hasItem' will be set to true if the point points to an item in the list
   * control.
   */
  static bool CheckHitList (wxListCtrl* list, bool& hasItem, const wxPoint& pos);

  /**
   * Check if a list box is hit with the given point (in screen space).
   * This is mostly used in combination with context menus.
   * 'hasItem' will be set to true if the point points to an item in the list
   * control.
   */
  static bool CheckHitList (wxListBox* list, bool& hasItem, const wxPoint& pos);
};

#endif // __appares_listctrltools_h

