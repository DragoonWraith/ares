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

#ifndef __appares_treeview_h
#define __appares_treeview_h

#include <csutil/stringarray.h>

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/treectrl.h>
#include <wx/xrc/xmlres.h>

#include "rowmodel.h"

class UIDialog;


/**
 * A view based on a tree control on top of a RowModel.
 */
class TreeCtrlView : public wxEvtHandler
{
private:
  wxTreeCtrl* tree;
  csRef<RowModel> model;
  csStringArray columns;
  UIDialog* forcedDialog;
  bool ownForcedDialog;
  csString rootName;

  void UnbindModel ();
  void BindModel (RowModel* model);

  void OnContextMenu (wxContextMenuEvent& event);
  void OnAdd (wxCommandEvent& event);
  void OnEdit (wxCommandEvent& event);
  void OnDelete (wxCommandEvent& event);

  /// This is used in case the model has a dialog to use.
  csStringArray DialogEditRow (const csStringArray& origRow);

  /// Get the right dialog.
  csStringArray DoDialog (const csStringArray& origRow);

public:
  TreeCtrlView (wxTreeCtrl* tree) : tree (tree), forcedDialog (0), ownForcedDialog (false) { }
  TreeCtrlView (wxTreeCtrl* tree, RowModel* model) : tree (tree), forcedDialog (0), ownForcedDialog (false)
  {
    BindModel (model);
  }
  ~TreeCtrlView ();

  void SetModel (RowModel* model) { BindModel (model); Refresh (); }

  void SetRootName (const char* rootName) { TreeCtrlView::rootName = rootName; }

  /**
   * Manually force an editor dialog to use. If 'own' is true
   * this view will assume ownership of the dialog and remove the
   * dialog in the destructor.
   */
  void SetEditorDialog (UIDialog* dialog, bool own = false);

  /**
   * Refresh the tree from the data in the model.
   */
  void Refresh ();
};

#endif // __appares_treeview_h

