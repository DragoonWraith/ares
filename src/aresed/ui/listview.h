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

#ifndef __appares_listview_h
#define __appares_listview_h

#include <csutil/stringarray.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/listctrl.h>
#include <wx/listbox.h>
#include <wx/xrc/xmlres.h>

#include "../models/rowmodel.h"

class UIDialog;


/**
 * A simple view based on a list control on top of a RowModel.
 * This version doesn't allow any editing.
 */
class SimpleListCtrlView : public wxEvtHandler
{
protected:
  wxListCtrl* list;
  csRef<RowModel> model;
  csStringArray columns;

  void UnbindModel ();
  void BindModel (RowModel* model);

public:
  SimpleListCtrlView (wxListCtrl* list) : list (list) { }
  SimpleListCtrlView (wxListCtrl* list, RowModel* model) : list (list)
  {
    BindModel (model);
  }
  ~SimpleListCtrlView ();

  void SetModel (RowModel* model) { BindModel (model); Refresh (); }

  /**
   * Refresh the list from the data in the model.
   */
  void Refresh ();

  /**
   * Get the current selected row (or empty row in case nothing is
   * selected).
   */
  csStringArray GetSelectedRow ();
};

/**
 * A view based on a list control on top of a RowModel.
 * This version supports a context menu to add/edit/delete items from the model.
 */
class ListCtrlView : public SimpleListCtrlView
{
private:
  UIDialog* forcedDialog;
  bool ownForcedDialog;

  void OnContextMenu (wxContextMenuEvent& event);
  void OnAdd (wxCommandEvent& event);
  void OnEdit (wxCommandEvent& event);
  void OnDelete (wxCommandEvent& event);

  /// This is used in case the model has a dialog to use.
  csStringArray DialogEditRow (const csStringArray& origRow);

  /// Get the right dialog.
  csStringArray DoDialog (const csStringArray& origRow);

  void SetupContextMenu ();
  void ClearContextMenu ();

public:
  ListCtrlView (wxListCtrl* list) : SimpleListCtrlView (list), forcedDialog (0),
    ownForcedDialog (false)
  {
    SetupContextMenu ();
  }
  ListCtrlView (wxListCtrl* list, RowModel* model) :
    SimpleListCtrlView (list,  model),
    forcedDialog (0), ownForcedDialog (false)
  {
    SetupContextMenu ();
  }
  ~ListCtrlView ();

  /**
   * Manually force an editor dialog to use. If 'own' is true
   * this view will assume ownership of the dialog and remove the
   * dialog in the destructor.
   */
  void SetEditorDialog (UIDialog* dialog, bool own = false);
};

#endif // __appares_listview_h
